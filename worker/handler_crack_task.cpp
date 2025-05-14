#include "MD5cracker.hpp"
#include "crack_task.hpp"
#include "server.hpp"

#include "json.hpp"
#include "httplib.h"

MD5Cracker md5Cracker = {};

void CreateCrackTaskHandler(const AMQP::Message &message, uint64_t deliveryTag) {
    CrackTaskRequest task;
    try {        
            std::string body(message.body(), message.bodySize());

            if (body.empty() || body[0] != '{') {
                throw std::runtime_error("Invalid JSON format");
            }
                auto jsonBody = nlohmann::json::parse(body);
                task = crackTaskRequestfromJson(jsonBody);
            }

        catch (const std::exception& e) {
        std::cerr << "Failed to process request: " << e.what() << std::endl;
        manager.getChannel()->reject(deliveryTag, true);
        return;
    }

    std::thread([task, deliveryTag]() {
        try {
            std::cout << "Starting crack attempt for hash " << task.hash << " (part " << task.partNumber << "/" << task.partCount << ")" << std::endl;

            std::string result;
            try {
                result = md5Cracker.Crack(task.hash, task.maxLength, task.partNumber, task.partCount);
                std::cout << "Result for hash " << task.hash << ": " << result << " (part " << task.partNumber << "/" << task.partCount << ")" << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Crack failed for hash " << task.hash << " (part " << task.partNumber << "/" << task.partCount << "): " << e.what() << std::endl;
                manager.getChannel()->reject(deliveryTag, true);
                return;
            }

            manager.publishWithRetry("hash_crack_results", { task.hash, result, task.partNumber });
        }
        catch (const std::exception& e) {
            std::cerr << "Crack failed on sending" << e.what() << std::endl;
            return;
        }
    }).detach();
}
