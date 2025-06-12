#ifndef PUB_HPP
#define PUB_HPP

#include <string>
#include <zmq.hpp>
#include <nlohmann/json.hpp>

namespace zmq_network {

class Publisher {
public:
    Publisher();
    ~Publisher();

    bool connectPublisher(const std::string& nodeName, std::optional<std::string> std_topic = std::nullopt);
    bool sendMessage(const std::string& message, std::optional<std::string> topicOverride = std::nullopt);

private:
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::string topic_;
};

} // namespace zmq_network

#endif // PUB_HPP
