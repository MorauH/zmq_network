#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <functional>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include "zmq_network/pub.hpp"
#include "zmq_network/helper.hpp"

using json = nlohmann::json;

namespace zmq_network {

// Constructor implementation
Publisher::Publisher()
    : context_(1), socket_(nullptr) {}

// Destructor implementation
Publisher::~Publisher() {
    if (socket_) {
        socket_->close();
    }
}

// Method implementations
bool Publisher::connectPublisher(const std::string& nodeName, std::optional<std::string> std_topic) {
    // Read configuration from file
    json config = Helper::readConfig();
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
    bool is_ipc = nodeConfig["is_ipc"].get<bool>();

    // Set topic
    if (valid_topics.empty()) {
        std::cerr << "Error: No valid topics found for: " << nodeName << std::endl;
        return false;
    }

    topic_ = valid_topics[0];
    if (std_topic.has_value()) {
        if (Helper::contains_string(valid_topics, std_topic.value())) {
            topic_ = std_topic.value();
        }
        else {
            std::cerr << "Error: Invalid topic provided: " << std_topic.value()  << " => defaulting to first valid topic: " << topic_ << std::endl;
        }
    }
    
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
        int port = nodeConfig["port"].get<int>();
        full_address = "tcp://" + address + ":" + std::to_string(port);
    }
    
    // Create & bind socket
    socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::pub);
    
    try {
        socket_->bind(full_address);
    } catch (zmq::error_t& e) {
        if (e.num() == EADDRINUSE) {
            std::cerr << "Port occupied, exiting..." << std::endl;
        } else {
            std::cerr << "ZeroMQ error: " << e.what() << std::endl;
        }
        return false;
    }
    
    std::cout << topic_ << " connected on " << full_address << std::endl;
    return true;
}

bool Publisher::sendMessage(const std::string& message, std::optional<std::string> topicOverride) {
    if (!socket_) {
        std::cerr << "Error: Socket not initialized!" << std::endl;
        return false;
    }

    // Use topicOverride as header for message if provided, else default to topic_
    std::string fullMessage = (topicOverride.value_or(topic_)) + '\x1F' + message;
    
    try {
        zmq::message_t zmqMsg(fullMessage.data(), fullMessage.size());
        socket_->send(zmqMsg, zmq::send_flags::none);
        return true;
    } catch (zmq::error_t& e) {
        std::cerr << "ZeroMQ error sending message: " << e.what() << std::endl;
        return false;
    }
}
} // namespace zmq_network