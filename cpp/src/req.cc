
#include <iostream>
#include <nlohmann/json.hpp>
#include "zmq_network/req.hpp"
#include "zmq_network/helper.hpp"

using json = nlohmann::json;

namespace zmq_network {

// Constructor
Requester::Requester(zmq::context_t& context)
    : socket_(std::make_unique<zmq::socket_t>(context, zmq::socket_type::req)) {}

// Destructor
Requester::~Requester() {
    if (socket_) {
        socket_->close();
    }
}

bool Requester::sendRequest(const std::string& nodeName, const std::string& message, std::string& response) {
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
        std::cerr << "Error: Responder configuration not found for: " << nodeName << std::endl;
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
        int port = nodeConfig["port"];
        full_address = "tcp://" + address + ":" + std::to_string(port);
    }
    
    // Connect as client
    try {
        socket_->connect(full_address);
    } catch (zmq::error_t& e) {
        std::cerr << "ZeroMQ error: " << e.what() << std::endl;
        return false;
    }

    // Send request
    try {
        zmq::message_t request(message.data(), message.size());
        socket_->send(request, zmq::send_flags::none);
    } catch (zmq::error_t& e) {
        std::cerr << "ZeroMQ error sending message: " << e.what() << std::endl;
        return false;
    }

    // receive response, blocking call
    zmq::message_t responseMsg;
    try {
        if (!socket_->recv(responseMsg)) {
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

    // Disconnect
    try {
        socket_->disconnect(full_address);
    } catch (zmq::error_t& e) {
        std::cerr << "ZeroMQ error: " << e.what() << std::endl;
        return false;
    }

    // Copy message
    response = std::string(static_cast<char*>(responseMsg.data()), responseMsg.size());
    return true;
}
    
    
} // namespace zmq_network