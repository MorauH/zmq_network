#ifndef RESP_HPP
#define RESP_HPP

#include <string>
#include <zmq.hpp>
#include <nlohmann/json.hpp>

namespace zmq_network {

class Responder {
public:
    Responder(zmq::context_t& context);
    ~Responder();

    bool connectResponder(const std::string& nodeName);
    bool receiveRequest(std::string& request);
    bool sendResponse(const std::string& response);

private:
    zmq::context_t& context_;
    std::unique_ptr<zmq::socket_t> socket_;
};

} // namespace zmq_network

#endif // RESP_HPP
