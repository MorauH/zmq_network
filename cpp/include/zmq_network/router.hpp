#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <string>
#include <zmq.hpp>
#include <atomic>
#include <thread>
#include <functional>
#include <memory>

namespace zmq_network {
    
class Router {
public:
    using MessageHandler = std::function<std::string(const std::string& sender_id,
                                                        const std::string& message,
                                                        bool& should_reply)>;
    
                                                        Router(zmq::context_t& context);
    ~Router();

    bool connectRouter(const std::string& nodeName, MessageHandler message_handler);

    bool start();
    bool stop();

private:
    zmq::context_t& context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::atomic<bool> running_;
    std::thread worker_thread_;

    MessageHandler message_handler_;

    void workerLoop();
    void processMessage();
    bool sendResponse(const std::string& client_id, const std::string& response);

};

} // namespace zmq_network

#endif // ROUTER_HPP
