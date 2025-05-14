#include <iostream>
#include <memory>
#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>
#include <amqpcpp/libev.h> 
#include <ev.h>

#include "models.hpp"
#include "handlers.hpp"

#include "httplib.h"

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

    int max_retries;

    std::mutex mtx;
    std::condition_variable cv;
    bool message_acked = false;
    bool message_nacked = false;
    
public:
    RabbitMQManager();
    void setupConnection();
    void run();
    std::shared_ptr<AMQP::TcpChannel> getChannel();
    std::shared_ptr<AMQP::TcpConnection> getConnection();
    std::function<void()> GetRetry_callback();
    ~RabbitMQManager();
    void publishWithRetry(const std::string& queue, CrackTaskRequest message);
    void scheduleReconnect();
    int current_retry;
    std::function<void()> retry_callback;
private:
    void doPublish(const std::string& queue, CrackTaskRequest message);
    void handlePublishFailure(const std::string& error);
};
void onRetryTimer(struct ev_loop* loop, ev_timer* timer, int revents);
void onReconnectTimer(struct ev_loop* loop, ev_timer* timer, int revents);

extern RabbitMQManager manager;

#endif