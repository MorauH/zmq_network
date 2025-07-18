
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <functional>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include "zmq_network/node.hpp"
#include "zmq_network/pub.hpp"
#include "zmq_network/helper.hpp"

using json = nlohmann::json;

namespace zmq_network {

// Constructor implementation
Node::Node()
    : context_(1), publisher_(Publisher()), subscribers_(std::vector<std::unique_ptr<Subscriber>>()) {}

// Destructor implementation
Node::~Node() {
    publisher_.~Publisher();

    if (!subscribers_.empty()) {
        for (auto& subscriber : subscribers_) {
            subscriber->~Subscriber();
        }
    }
}

// Method implementations
bool Node::connectPublisher(const std::string& nodeName, std::optional<std::string> std_topic) {
    return publisher_.connectPublisher(context_, nodeName, std_topic);
}

bool Node::publishMessage(const std::string& message, std::optional<std::string> topicOverride) {
    return publisher_.sendMessage(message, topicOverride);
}

bool Node::connectSubscriber(const std::string& nodeName, const std::string& topic) {
    auto subscriber = std::make_unique<Subscriber>();
    if (!subscriber->connectSubscriber(context_, nodeName, topic)) {
        return false;
    }
    subscribers_.push_back(std::move(subscriber));
    return true;
}

} // namespace zmq_network
