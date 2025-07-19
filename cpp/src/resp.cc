#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <functional>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include "zmq_network/resp.hpp"
#include "zmq_network/helper.hpp"

using json = nlohmann::json;

namespace zmq_network {

Responder::Responder(zmq::context_t& context)
    : context_(context), socket_(nullptr) {}

Responder::~Responder() {
    if (socket_) {
        socket_->close();
    }
}

bool Responder::connectResponder(const std::string& nodeName) {
    // Read configuration from file
    json config = Helper::readConfig();
    if (config.empty() || !config.contains("nodes")) {
        return false;
    }
    
    if (!config["nodes"].contains(nodeName)) {
        std::cerr << "Error: Node configuration not found for: " << nodeName << std::endl;
        return false;
    }

    if (!config["nodes"][nodeName].contains("responder")) {
        std::cerr << "Error: responder configuration not found for: " << nodeName << std::endl;
        return false;
    }
    
    const json& nodeConfig = config["nodes"][nodeName]["responder"];
    
    if (!nodeConfig.contains("is_ipc")) {
        std::cerr << "Error: Missing required configuration fields for: " << nodeName << std::endl;
        return false;
    }
    
    bool is_ipc = nodeConfig["is_ipc"].get<bool>();
    
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
    socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::rep);
    
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

bool Responder::receiveRequest(std::string& request) {
    zmq::message_t requestMsg;
    
    // Attempt to receive message, blocking call
    try {
        if (!socket_->recv(requestMsg)) {
            std::cerr << "Failed to receive request" << std::endl;
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

    request = std::string(static_cast<char*>(requestMsg.data()), requestMsg.size());
    return true;

    // std::string response;
    // // Send response
    // try {
    //     zmq::message_t responseMsg(response.data(), response.size());
    //     socket_->send(responseMsg, zmq::send_flags::none);
    // } catch (const zmq::error_t& e) {
    //     std::cerr << "ZMQ error: " << e.what() << std::endl;
    //     return false;
    // }

    // return true;
}

bool Responder::sendResponse(const std::string& response) {
    try {
        zmq::message_t responseMsg(response.data(), response.size());
        socket_->send(responseMsg, zmq::send_flags::none);
    } catch (const zmq::error_t& e) {
        std::cerr << "ZMQ error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

} // namespace zmq_network