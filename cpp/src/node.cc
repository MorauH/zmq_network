
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include "zmq_network/node.hpp"

using json = nlohmann::json;

namespace zmq_network {

// Constructor implementation
Node::Node(bool singleThreadedResponses)
    : context_(1),
    publisher_(Publisher(context_)),
    subscribers_(std::vector<std::shared_ptr<Subscriber>>()),
    requester_(Requester(context_)),
    responder_(Responder(context_)),
    singleThreadedResponses_(singleThreadedResponses) {}

// Destructor implementation
Node::~Node() {
    publisher_.~Publisher();
    if (!subscribers_.empty()) {
        for (auto& subscriber : subscribers_) {
            subscriber->~Subscriber();
        }
    }
    requester_.~Requester();
    responder_.~Responder();

    // destroy context (closes all sockets and their blocked threads)
    context_.close();
}

// Method implementations
bool Node::connectPublisher(const std::string& nodeName, std::optional<std::string> std_topic) {
    return publisher_.connectPublisher(nodeName, std_topic);
}

bool Node::publishMessage(const std::string& message, std::optional<std::string> topicOverride) {
    return publisher_.sendMessage(message, topicOverride);
}

bool Node::connectSubscriber(const std::string& nodeName, const std::string& topic, std::function<bool(std::string&)> callable) {
    auto subscriber = std::make_shared<Subscriber>(context_);
    if (!subscriber->connectSubscriber(nodeName, topic)) {
        return false;
    }
    subscribers_.push_back(std::move(subscriber));

    // Start new thread, enters reciever loop
    std::thread t([this, &subscriber, &callable]() {
        std::string message;
        bool success;
        while (subscriber->receiveMessage(message)) {
            // lock mutex if singleThreadedResponses_ is true
            if (singleThreadedResponses_) {
                std::lock_guard<std::mutex> lock(mutex_);
                success = callable(message);
            } else {
                success = callable(message);
            }

            if (!success) {
                std::cerr << "Failed to process message" << std::endl;
                continue;
            }
        }
        std::cout << "Subscriber thread exited" << std::endl;
    });

    // Detach thread from local scope
    t.detach();
    return true;
}

bool Node::connectResponder(const std::string& nodeName, std::function<bool(std::string&, std::string&)> callable) {
    if (!responder_.connectResponder(nodeName)) {
        return false;
    }

    // Start new thread, enters reciever loop
    std::thread t([this, &callable]() {
        std::string request;
        std::string response;
        bool success;

        while (responder_.receiveRequest(request)) {
            // lock mutex if singleThreadedResponses_ is true
            if (singleThreadedResponses_) {
                std::lock_guard<std::mutex> lock(mutex_);
                success = callable(request, response);
            } else {
                success = callable(request, response);
            }

            if (!success) {
                std::cerr << "Failed to process request" << std::endl;
                responder_.sendResponse("");
                continue;
            }

            if (!responder_.sendResponse(response)) {
                std::cerr << "Failed to send response" << std::endl;
                continue;
            }
        }
        std::cout << "Responder thread exited" << std::endl;
    });

    return true;
}

bool Node::sendRequest(const std::string& nodeName, const std::string& message, std::string& response) {
    return requester_.sendRequest(nodeName, message, response);
}

} // namespace zmq_network
