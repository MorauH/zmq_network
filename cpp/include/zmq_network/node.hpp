#ifndef NODE_HPP
#define NODE_HPP

#include <string>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include "zmq_network/pub.hpp"

namespace zmq_network {

class Node {
public:
    Node();
    ~Node();

    bool connectPublisher(const std::string& nodeName, std::optional<std::string> std_topic = std::nullopt);
    bool publishMessage(const std::string& message, std::optional<std::string> topicOverride = std::nullopt);

private:
    zmq::context_t context_;
    Publisher publisher_;
};

} // namespace zmq_network

#endif // NODE_HPP

