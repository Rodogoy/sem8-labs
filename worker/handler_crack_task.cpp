#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <semaphore>
#include "json.hpp"
#include <memory>
#include <stdexcept>
#include <atomic>

#include"main.hpp"

#include "httplib.h"

extern MD5Cracker md5Cracker = {};

void RetryingSend(const std::string& url, const nlohmann::json& payload, int maxRetries, std::chrono::seconds delay) {
    size_t protocol_end = url.find("://");
    if (protocol_end == std::string::npos) {
        throw std::runtime_error("Invalid URL format");
    }

    httplib::Client cli(url.substr(0, url.find('/', 8))); 
    std::string path = url.substr(url.find('/', 8));  

    cli.set_connection_timeout(30);
    cli.set_read_timeout(60);

    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        httplib::Headers headers = {
            {"Content-Type", "application/json"}
        };

        auto res = cli.Post(path.c_str(), headers, payload.dump(), "application/json");

        if (!res) {
            std::cerr << "HTTP request failed: " << res.error() << std::endl;
            std::this_thread::sleep_for(delay);
            continue;
        }

        if (res->status == 200) {
            std::cout << "Successfully sent result to manager" << std::endl;
            return;
        }

        std::cerr << "HTTP request failed with status: " << res->status << std::endl;
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < delay) {
            std::this_thread::yield();
        }
    }

    throw std::runtime_error("Max retries reached");
}

void SendResult(const CrackTaskResult& result) {
    nlohmann::json payload = {
        {"hash", result.hash},
        {"result", result.result},
        {"partNumber", result.partNumber}
    };

    try {
        RetryingSend(cfg.ManagerURL + "/internal/api/manager/hash/crack/result", payload, maxRetries, retryDelay);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to send result for hash " << result.hash << ": " << e.what() << std::endl;
    }
}

void CreateCrackTaskHandler(const httplib::Request& req, httplib::Response& res) {
    CrackTaskRequest task;
    try {
        auto jsonBody = nlohmann::json::parse(req.body);
        task = crackTaskRequestfromJson(jsonBody);
        }
    catch (const std::exception& e) {
        std::cerr << "Failed to process request: " << e.what() << std::endl;
        res.status = 400;
        res.set_content("Invalid request body", "text/plain");
    }

    res.status = 200;
    std::thread([task]() {
        try {
            std::cout << "Starting crack attempt for hash " << task.hash << " (part " << task.partNumber << "/" << task.partCount << ")" << std::endl;

            std::string result;
            try {
                result = md5Cracker.Crack(task.hash, task.maxLength, task.partNumber, task.partCount);
                std::cout << "Result for hash " << task.hash << ": " << result << " (part " << task.partNumber << "/" << task.partCount << ")" << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Crack failed for hash " << task.hash << " (part " << task.partNumber << "/" << task.partCount << "): " << e.what() << std::endl;
                SendResult({ task.hash, "", task.partNumber });
                return;
            }

            SendResult({ task.hash, result, task.partNumber });
        }
        catch (const std::exception& e) {
            std::cerr << "Crack failed on sending" << e.what() << std::endl;
            return;
        }
    }).detach();
}
