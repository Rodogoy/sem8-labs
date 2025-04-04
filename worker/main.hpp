#include <iostream>
#include <cstdlib>
#include <string>
#include <semaphore>

#include "json.hpp"
#include "httplib.h"
#ifndef MAIN
#define MAIN

struct Config {
    int MaxWorkers;
    std::string WorkerURL;
    std::string ManagerURL;
    std::string Port;
};

void StartServer(const Config& cfg);

const int defaultMaxWorkers = 1;
const std::string defaultPort = "8080";
const std::string defaultManagerURL = "http://manager:8080";

struct CrackTaskRequest {
    std::string hash;
    int maxLength;
    int partNumber;
    int partCount;

    nlohmann::json toJson() const;
};
CrackTaskRequest crackTaskRequestfromJson(const nlohmann::json& j);

struct CrackTaskResult {
    std::string hash;
    std::string result;
    int partNumber;

    nlohmann::json toJson() const;;
};

CrackTaskResult crackTaskResultfromJson(const nlohmann::json& j);

class MD5Cracker {
private:
    std::string alphabet;
    std::string IntToCandidate(int num, int length);
    std::string CalculateMD5(const std::string& input);
public:
    MD5Cracker();
    std::string Crack(const std::string& targetHash, int maxLength, int partNumber, int partCount);

};

const int maxRetries = 5;
const std::chrono::seconds retryDelay(1);

void RetryingSend(const std::string& url, const nlohmann::json& payload, int maxRetries, std::chrono::seconds delay);
void SendResult(const CrackTaskResult& result);
void CreateCrackTaskHandler(const httplib::Request& req, httplib::Response& res);

struct SendConfig {
    int maxRetries;
    std::chrono::seconds delay;
    int successStatus;
    std::vector<int> retryOnStatuses;
};

struct Registration {
    std::string workerUrl;
    int maxWorkers;
    int port;
};

void RetryingSend(const std::string& url, const nlohmann::json& payload, const SendConfig& config);
void RegisterWithManager(const std::string& workerUrl, int maxWorkers);
Config LoadConfig();

extern MD5Cracker md5Cracker;

extern Config cfg;

#endif