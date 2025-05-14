#include "server.hpp"
#include "main.hpp"

RabbitMQManager manager = RabbitMQManager();

RabbitMQManager::RabbitMQManager() {
    
    loop = ev_loop_new(EVFLAG_AUTO);
    handler = new AMQP::LibEvHandler(loop);
    max_retries = 20;

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

    channel->declareQueue("complite_crack_task", AMQP::durable, arguments)
        .onSuccess([this]() {
            channel->consume("complite_crack_task")
                .onReceived([this](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
                    try {
                    if(redelivered) std::cout << "This worker get redelivered message" << std::endl;
                    CreateCrackTaskHandler(message, deliveryTag);
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

std::function<void()> RabbitMQManager::GetRetry_callback()
{
    return retry_callback;
}

void RabbitMQManager::run() {
    ev_run(loop, 0);
}

RabbitMQManager::~RabbitMQManager() {
    ev_timer_stop(loop, &retry_timer);
    ev_timer_stop(loop, &reconnect_timer);
    if (connection) connection->close();
    delete handler;
    ev_loop_destroy(loop);
}

void RabbitMQManager::publishWithRetry(const std::string& queue, CrackTaskResult message) {
    current_retry = 0;
    retry_callback = [this, queue, message]() {
        doPublish(queue, message);
    };
    retry_callback();
}

void onRetryTimer(struct ev_loop* loop, ev_timer* timer, int revents) {
    auto self = static_cast<RabbitMQManager*>(timer->data);
    manager.GetRetry_callback()();
}

void RabbitMQManager::doPublish(const std::string& queue, CrackTaskResult message) {
    if (current_retry >= max_retries) {
        std::cerr << "Max reconnection attempts reached (" << max_retries << ")" << std::endl;
        return;
    }
    auto payload = message.toJson().dump();
    current_retry++;
    std::cout << "Attempting to send " << current_retry << "/" << max_retries << std::endl;
    
    try {
        channel->confirmSelect().onSuccess([this, queue, payload]() {
            if(!channel->publish(
                "", 
                queue, 
                payload, 
                AMQP::mandatory
            ))
                handlePublishFailure("Failed to publish message");
        }).onAck([this](uint64_t deliveryTag, bool multiple) {
            std::cout << "Message successfully delivered to the queue" << std::endl;
            current_retry = 0;
        }).onNack([this](uint64_t deliveryTag, bool multiple, bool requeue) {
            handlePublishFailure("Broker refused to accept the message:");
        }).onError([this](const char *message) {
            handlePublishFailure("Broker refused to accept the message:");
        });
    } catch (const std::exception& e) {
        handlePublishFailure(std::string("Publication error: ") + e.what());
    }
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
