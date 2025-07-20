#include <string>
#include <zmq.hpp>
#include <nlohmann/json.hpp>

#include "zmq_network/router.hpp"
#include "zmq_network/helper.hpp"

namespace zmq_network {

Router::Router(zmq::context_t& context) 
    : context_(context), socket_(nullptr), running_(false), message_handler_(nullptr) {}

Router::~Router() {
    if (socket_) {
        socket_->close();
    }

    if (running_.load()) {
        stop();
    }
}

bool Router::connectRouter(const std::string& nodeName, MessageHandler message_handler) {
    // Read configuration from file
    json config = Helper::readConfig();
    if (config.empty() || !config.contains("nodes")) {
        return false;
    }

    if (!config["nodes"].contains(nodeName)) {
        std::cerr << "Error: Node configuration not found for: " << nodeName << std::endl;
        return false;
    }

    if (!config["nodes"][nodeName].contains("router")) {
        std::cerr << "Error: Router configuration not found for: " << nodeName << std::endl;
        return false;
    }

    const json& nodeConfig = config["nodes"][nodeName]["router"];

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

    // Create socket
    socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::router);

    // Set socket options
    int linger = 0; // milliseconds to wait for pending messages to send at socket close
    socket_->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));

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
    
    std::cout << nodeName << " routing on " << full_address << std::endl;

    // set message handler
    message_handler_ = message_handler;

    return true;
}

bool Router::start() {
    if (running_.exchange(true)) {
        std::cerr << "Router already running" << std::endl;
        return false;
    }

    if (!message_handler_) {
        std::cerr << "Message handler not set" << std::endl;
        return false;
    }

    worker_thread_ = std::thread(&Router::workerLoop, this);
    //worker_thread_.detach(); needed?
    return true;
}

bool Router::stop() {
    if (!running_.exchange(false)) {
        std::cerr << "Router not running" << std::endl;
        return false;
    }

    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    return true;
}

void Router::workerLoop() {
    std::cout << "Router worker thread started" << std::endl;

    zmq::pollitem_t items[] = {
        { *socket_, 0, ZMQ_POLLIN, 0 }
    };

    while (running_.load()) {
        try {
            // Poll with timeout
            zmq::poll(items, 1, std::chrono::milliseconds(100));

            if (items[0].revents & ZMQ_POLLIN) {
                processMessage();
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
    }

    std::cout << "Router worker thread stopped" << std::endl;
}

void processMessage() {
    try {
        // Receive identity frame
        zmq::message_t identity;
        auto result = socket_.recv(identity, zmq::recv_flags::none);
        if (!result) {
            std::cerr << "Failed to receive identity frame" << std::endl;
            return;
        }

        std::string sender_id(static_cast<char*>(identity.data()), identity.size());

        // Receive message
        zmq::message_t message;
        result = socket_.recv(message, zmq::recv_flags::none);
        if (!result) {
            std::cerr << "Failed to receive message" << std::endl;
            return;
        }

        std::string msg_content(static_cast<char*>(message.data()), message.size());

        // Call message handler
        bool should_reply = false;
        std::string response = message_handler_(sender_id, msg_content, should_reply);

        // Send response if required
        if (should_reply && !response.empty()) {
            sendResponse(sender_id, response);
        }
    } catch (const zmq::error_t& e) {
        std::cerr << "ZMQ error: " << e.what() << std::endl;
    }
}

bool Router::sendResponse(const std::string& client_id, const std::string& response) {
    try {
        zmq::message_t id_msg(client_id.data(), client_id.size());
        zmq::message_t responseMsg(response.data(), response.size());
        socket_->send(id_msg, zmq::send_flags::sndmore);
        socket_->send(responseMsg, zmq::send_flags::none);
        return true;
    } catch (const zmq::error_t& e) {
        std::cerr << "Error sending response: " << e.what() << std::endl;
        return false;
    }
}

} // namespace zmq_network
