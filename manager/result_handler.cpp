#include "httplib.h"
#include "json.hpp"

#include "handlers.hpp"
#include "queue.hpp"

void ResultHandler(const AMQP::Message &message, uint64_t deliveryTag) {
    std::string body(message.body(), message.bodySize());
    std::cout << "Raw message body: " << body << std::endl; 
    if (body.empty() || body[0] != '{') {
        throw std::runtime_error("Invalid JSON format");
    }
    auto jsonBody = nlohmann::json::parse(body);
    std::string hash = jsonBody["hash"];
    std::string result = jsonBody["result"];
    int partNumber = jsonBody["partNumber"];
    if(taskQueue->FindCompletedTask(hash, partNumber))
    {
        GlobalTaskStorage->AddPartResult(hash, partNumber, result);
        taskQueue->PopCompletedTask(hash, partNumber);
    }
    else
        std::cout << "task already complited or not found in MongoDB with Hash, " << hash <<", " << partNumber << std::endl;

}