#ifndef SUB_HPP
#define SUB_HPP

#include <string>
#include <zmq.hpp>
#include <nlohmann/json.hpp>

namespace zmq_network {

class Subscriber {
public:
    Subscriber(const std::string& configFilename = "config.json");
    ~Subscriber();

    bool connectSubscriber(const std::string& nodeName, const std::string& topic);
    bool receiveMessage(std::string& message);

private:
    std::string configFilename_;
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> socket_;
    uint16_t topicHeaderBytes_;
};

} // namespace zmq_network

#endif // SUB_HPP