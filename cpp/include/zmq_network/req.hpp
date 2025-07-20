
#ifndef REP_HPP
#define REP_HPP

#include <string>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <memory>

namespace zmq_network {

class Requester {
public:
    Requester(zmq::context_t& context);
    ~Requester();
    
    bool sendRequest(const std::string& nodeName, const std::string& message, std::string& response);

private:
    std::unique_ptr<zmq::socket_t> socket_;
};

} // namespace zmq_network

#endif // REQ_HPP
