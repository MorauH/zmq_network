#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <functional>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include "zmq_network/sub.hpp"
#include "zmq_network/helper.hpp"

using json = nlohmann::json;

namespace zmq_network {

// Constructor implementation
Subscriber::Subscriber(const std::string& configFilename)
    : configFilename_(configFilename), context_(1), socket_(nullptr), topicHeaderBytes_(0) {}

// Destructor implementation
Subscriber::~Subscriber() {
    if (socket_) {
        socket_->close();
    }
}

// Method implementations
bool Subscriber::connectSubscriber(const std::string& nodeName, const std::string& topic) {
    // Read configuration from file
    json config = Helper::readConfig(configFilename_);
    if (config.empty() || !config.contains("nodes")) {
        return false;
    }

    if (!config["nodes"].contains(nodeName)) {
        std::cerr << "Error: Node configuration not found for: " << nodeName << std::endl;
        return false;
    }

    const json& nodeConfig = config["nodes"][nodeName];

    if (!nodeConfig.contains("topics") || !nodeConfig.contains("is_ipc")) {
        std::cerr << "Error: Missing required configuration fields for: " << nodeName << std::endl;
        return false;
    }

    std::vector<std::string> valid_topics = nodeConfig["topics"];
    if (!Helper::contains_string(valid_topics, topic)) {
        std::cerr << "Error: Invalid topic for: " << nodeName << std::endl;
        return false;
    }

    bool is_ipc = nodeConfig["is_ipc"].get<bool>();

    // Set topic header bytes
    topicHeaderBytes_ = topic.length();
    if (topicHeaderBytes_ != 0)
        topicHeaderBytes_ += 1; // Add 1 for unit seperator if not subscribing to all topics

    // Construct full address from configuration
    std::string full_address;
    if (is_ipc) {
        full_address = "ipc:///tmp/" + nodeName;
    } else {
        if (!nodeConfig.contains("address") || !nodeConfig.contains("port")) {
            std::cerr << "Error: Missing required configuration field 'port' for: " << nodeName << std::endl;
            return false;
        }
        if (!nodeConfig["port"].is_number_integer()) {
            std::cerr << "Error: 'port' must be an integer for: " << nodeName << std::endl;
            return false;
        }

        std::string address = nodeConfig["address"];
        int port = nodeConfig["port"];
        full_address = "tcp://" + address + ":" + std::to_string(port);
    }

    // Create & connect socket
    socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::sub);

    try {
        socket_->connect(full_address);
    } catch (zmq::error_t& e) {
        std::cerr << "ZeroMQ error: " << e.what() << std::endl;
        return false;
    }

    // Set topic subscription filter
    socket_->set(zmq::sockopt::subscribe, topic);

    std::cout << topic << " connected on " << full_address << std::endl;
    return true;
}

/*
    * Receive a message from the publisher
    * Blocking call
    * @param message Filled with recieved message
        * Format: <message> if subscribed to specific topic, <topic>\x1F<message> if subscribed to all topics
    * @return True if message was received successfully, false if encountered an error
*/
bool Subscriber::receiveMessage(std::string& message) {
    zmq::message_t zmqMsg;
    
    // Attempt to receive message, blocking call
    try {
        if (!socket_->recv(zmqMsg)) {
            std::cerr << "Failed to receive message" << std::endl;
            return false;
        }
    } catch (const zmq::error_t& e) {
        if (e.num() == ETERM) {
            std::cerr << "ZMQ context was terminated" << std::endl;
        } else if (e.num() == ENOTSOCK) {
            std::cerr << "Invalid socket" << std::endl;
        } else {
            std::cerr << "ZMQ error: " << e.what() << std::endl;
        }
        return false;
    }

    // Copy message, without channel header
    message = std::string(static_cast<char*>(zmqMsg.data()) + topicHeaderBytes_, zmqMsg.size() - topicHeaderBytes_);

    std::cout << "Received message of size " << message.length() << std::endl;

    return true;
}
} // namespace zmq_network