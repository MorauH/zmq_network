#ifndef NODE_HPP
#define NODE_HPP

#include <string>
#include <vector>
#include <memory>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include "zmq_network/pub.hpp"
#include "zmq_network/sub.hpp"
#include "zmq_network/req.hpp"
#include "zmq_network/resp.hpp"

namespace zmq_network {

class Node {
public:
    Node(bool singleThreaded = true);
    ~Node();

    bool connectPublisher(const std::string& nodeName, std::optional<std::string> std_topic = std::nullopt);
    bool publishMessage(const std::string& message, std::optional<std::string> topicOverride = std::nullopt);

    bool connectSubscriber(const std::string& nodeName, const std::string& topic, std::function<bool(std::string&)> callable);
    
    bool connectResponder(const std::string& nodeName, std::function<bool(std::string&, std::string&)> callable);

    bool sendRequest(const std::string& nodeName, const std::string& message, std::string& response);

private:
    zmq::context_t context_;
    Publisher publisher_;
    std::vector<std::shared_ptr<Subscriber>> subscribers_;
    Requester requester_;
    Responder responder_;

    std::mutex mutex_;
    bool singleThreadedResponses_;
};

} // namespace zmq_network

#endif // NODE_HPP

