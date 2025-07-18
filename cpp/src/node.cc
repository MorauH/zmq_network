
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
    : context_(1), publisher_(Publisher()) {}

// Destructor implementation
Node::~Node() {
    publisher_.~Publisher();
}

// Method implementations
bool Node::connectPublisher(const std::string& nodeName, std::optional<std::string> std_topic) {
    return publisher_.connectPublisher(context_, nodeName, std_topic);
}

bool Node::publishMessage(const std::string& message, std::optional<std::string> topicOverride) {
    return publisher_.sendMessage(message, topicOverride);
}

} // namespace zmq_network
