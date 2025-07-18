#ifndef SUB_HPP
#define SUB_HPP

#include <string>
#include <zmq.hpp>
#include <nlohmann/json.hpp>

namespace zmq_network {

class Subscriber {
public:
    Subscriber();
    ~Subscriber();

    bool connectSubscriber(zmq::context_t& context, const std::string& nodeName, const std::string& topic);
    bool receiveMessage(std::string& message);

private:
    std::unique_ptr<zmq::socket_t> socket_;
    uint16_t topicHeaderBytes_;
};

} // namespace zmq_network

#endif // SUB_HPP