#include <iostream>
#include <memory>
#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>
#include <amqpcpp/libev.h> 
#include <ev.h>

#include "crack_task.hpp"

#ifndef SERVER
#define SERVER

class RabbitMQManager {
private:
    std::shared_ptr<AMQP::TcpConnection> connection;
    std::shared_ptr<AMQP::TcpChannel> channel;
    
    struct ev_loop* loop;
    AMQP::LibEvHandler* handler;

    ev_timer retry_timer;
    ev_timer reconnect_timer;

    std::function<void()> retry_callback;
    int max_retries;
    
    
public:
    RabbitMQManager();
    void setupConnection();
    void run();
    std::shared_ptr<AMQP::TcpChannel> getChannel();
    std::function<void()> GetRetry_callback();
    ~RabbitMQManager();
    void publishWithRetry(const std::string& queue, CrackTaskResult message);
    void scheduleReconnect();
    int current_retry;
private:
    void doPublish(const std::string& queue, CrackTaskResult message);
    void handlePublishFailure(const std::string& error);
};
void onRetryTimer(struct ev_loop* loop, ev_timer* timer, int revents);
void onReconnectTimer(struct ev_loop* loop, ev_timer* timer, int revents);

extern RabbitMQManager manager;

#endif