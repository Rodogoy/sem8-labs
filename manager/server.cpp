#include "server.hpp"
#include "handlers.hpp"

RabbitMQManager manager = RabbitMQManager();

RabbitMQManager::RabbitMQManager() {
    
    loop = ev_loop_new(EVFLAG_AUTO);
    handler = new AMQP::LibEvHandler(loop);
    max_retries = 1000;

    ev_timer_init(&retry_timer, &onRetryTimer, 0., 0.);
    retry_timer.data = this; 

    ev_timer_init(&reconnect_timer, &onReconnectTimer, 0., 0.);
    reconnect_timer.data = this;

    setupConnection();
}
void RabbitMQManager::setupConnection() {
    connection = std::make_shared<AMQP::TcpConnection>(
        handler, AMQP::Address(
            "amqp://guest:guest@rabbitmq/",
            AMQP::Login("guest", "guest")
    ));

    channel = std::make_shared<AMQP::TcpChannel>(connection.get());
    
    channel->onError([this](const char* message) {
        std::cerr << "AMQP Channel error: " << message << std::endl;
        scheduleReconnect();
    });
    channel->setQos(1);

    channel->confirmSelect();

    AMQP::Table arguments;
    arguments["x-message-ttl"] = 5000;
    arguments["x-dead-letter-exchange"] = "dead_letters";
    
    channel->declareQueue("hash_crack_results", AMQP::durable, arguments)
        .onSuccess([this]() {
            channel->consume("hash_crack_results")
                .onReceived([this](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
                    try {
                        if(redelivered) std::cout << "Manager get redelivered message" << std::endl;
                        ResultHandler(message, deliveryTag);
                        channel->ack(deliveryTag);
                    }
                    catch (const std::exception& e) {
                        std::cerr << e.what() << std::endl;
                        channel->reject(deliveryTag, true);
                    }
                });
        });
}

std::shared_ptr<AMQP::TcpChannel> RabbitMQManager::getChannel()
{
    return channel;
}

std::shared_ptr<AMQP::TcpConnection> RabbitMQManager::getConnection()
{
    return connection;
}

std::function<void()> RabbitMQManager::GetRetry_callback()
{
    return retry_callback;
}

void RabbitMQManager::run() {
    std::cout << "Manager connected to RabbitMQ and waiting for messages from workers" << std::endl;
    ev_run(loop, 0);
}

RabbitMQManager::~RabbitMQManager() {
    ev_timer_stop(loop, &retry_timer);
    ev_timer_stop(loop, &reconnect_timer);
    if (connection) connection->close();
    delete handler;
    ev_loop_destroy(loop);
}

void RabbitMQManager::publishWithRetry(const std::string& queue, CrackTaskRequest message) {
    current_retry = 0;
    retry_callback = [this, queue, message]() {
        doPublish(queue, message);
    };
    retry_callback();
}

void onRetryTimer(struct ev_loop* loop, ev_timer* timer, int revents) {
    auto self = static_cast<RabbitMQManager*>(timer->data);
    manager.retry_callback();
}

void RabbitMQManager::doPublish(const std::string& queue, CrackTaskRequest message) {
    if (current_retry >= max_retries) {
        std::cerr << "Max reconnection attempts reached (" << max_retries << ")" << std::endl;
        return;
    }

    auto payload = message.toJson().dump();
        std::cout<< payload <<std::endl;

    std::unique_lock<std::mutex> lock(mtx);
    message_acked = false;
    message_nacked = false;
    current_retry++;
    std::cout << "Attempting to send " << current_retry << "/" << max_retries << std::endl;
    
    try {
        channel->confirmSelect().onSuccess([this, queue, payload]() {
            channel->publish(
                "", 
                queue, 
                payload, 
                AMQP::mandatory
            );
        }).onAck([this](uint64_t deliveryTag, bool multiple) {
            std::unique_lock<std::mutex> lock(mtx);
            message_acked = true;
            cv.notify_one();
            std::cout << "Message successfully delivered to the queue" << std::endl;
            current_retry = 0;
        }).onNack([this](uint64_t deliveryTag, bool multiple, bool requeue) {
            std::unique_lock<std::mutex> lock(mtx);
            message_nacked = true;
            cv.notify_one();
        }).onError([this](const char *message) {
            std::unique_lock<std::mutex> lock(mtx);
            message_nacked = true;
            cv.notify_one();
        });
    } catch (const std::exception& e) {
        std::unique_lock<std::mutex> lock(mtx);
        message_nacked = true;
        cv.notify_one();
    }

    cv.wait(lock, [this]() { 
        return message_acked || message_nacked; 
    });
    
    if (message_nacked) 
        handlePublishFailure("Broker refused to accept the message:");
}

void RabbitMQManager::handlePublishFailure(const std::string& error) {
    std::cerr << error << ". Try " << current_retry << "/" << max_retries << std::endl;
    if (current_retry < max_retries) {
        double delay = std::pow(2, current_retry - 1);
        ev_timer_set(&retry_timer, delay, 0.);
        ev_timer_start(loop, &retry_timer);
    } else 
        std::cerr << "The message could not be delivered after " << max_retries << " attempts" << std::endl;
}

void RabbitMQManager::scheduleReconnect() {
    if (current_retry >= max_retries) {
        std::cerr << "Max reconnection attempts reached" << std::endl;
        return;
    }
    
    current_retry++;
    double delay = std::min(30.0, std::pow(2, current_retry));
    
    std::cout << "Scheduling reconnect in " << delay << " seconds (attempt " 
              << current_retry << "/" << max_retries << ")" << std::endl;
    
    ev_timer_set(&reconnect_timer, delay, 0.);
    ev_timer_start(loop, &reconnect_timer);
}

void onReconnectTimer(struct ev_loop* loop, ev_timer* timer, int revents) {
    auto self = static_cast<RabbitMQManager*>(timer->data);
    std::cout << "Attempting to reconnect..." << std::endl;
    manager.setupConnection();
    manager.current_retry = 0;
}

void Start() {
    std::thread rabbit_thread([&]() {
        manager.run();
    });

    httplib::Server svr;

    svr.Post("/api/hash/crack", [&](const httplib::Request& req, httplib::Response& res) {
        CrackHashHandler(req, res);
        });

    svr.Post("/api/hash/status", [&](const httplib::Request& req, httplib::Response& res) {
        StatusHandler(req, res);
        });

    std::cout << "Manager listening on port :8080" << std::endl;
    if (!svr.listen("manager", 8080)) {
        std::cerr << "Server error: Failed to start server" << std::endl;
    }
}