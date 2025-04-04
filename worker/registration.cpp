#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <stdexcept>

#include "httplib.h"
#include "json.hpp"

#include"main.hpp"

void RetryingSend(const std::string& url, const nlohmann::json& payload, const SendConfig& config) {
    httplib::Client cli(url.substr(0, url.find('/', 8))); 
    std::string path = url.substr(url.find('/', 8));  

    for (int attempt = 0; attempt < config.maxRetries; ++attempt) {
        httplib::Headers headers = {
            {"Content-Type", "application/json"}
        };

        auto res = cli.Post(path.c_str(), headers, payload.dump(), "application/json");

        if (!res) {
            std::cerr << "HTTP error: " << res.error() << std::endl;
            continue;
        }

        if (res->status == config.successStatus) {
            std::cout << "Successfully registered with manager at " << url << std::endl;
            return;
        }

        bool shouldRetry = false;
        for (int status : config.retryOnStatuses) {
            if (res->status == status) {
                shouldRetry = true;
                break;
            }
        }

        if (!shouldRetry) {
            throw std::runtime_error("Unexpected HTTP status: " + std::to_string(res->status));
        }
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < config.delay) {
            std::this_thread::yield();
        }
    }

    throw std::runtime_error("Max retries reached");
}

void RegisterWithManager(const std::string& workerUrl, int maxWorkers) {
    Registration registration = { workerUrl, maxWorkers, stoi(cfg.Port)};
    SendConfig sendConfig = {
        .maxRetries = 10,
        .delay = std::chrono::seconds(5),
        .successStatus = 200,
        .retryOnStatuses = {500, 502, 503, 504}
    };

    std::string url = cfg.ManagerURL + "/internal/api/worker/register";
    nlohmann::json payload = {
        {"workerName", registration.workerUrl},
        {"maxWorkers", registration.maxWorkers},
        {"port", registration.port}
    };

    RetryingSend(url, payload, sendConfig);
}