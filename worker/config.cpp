#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <cstdlib>
#include <string>

#include"main.hpp"

Config LoadConfig() {
    Config config;

    const char* workerURLEnv = std::getenv("WORKER_URL");
    config.WorkerURL = workerURLEnv ? workerURLEnv : "";

    const char* managerURLEnv = std::getenv("MANAGER_URL");
    config.ManagerURL = managerURLEnv ? managerURLEnv : defaultManagerURL;

    const char* portEnv = std::getenv("PORT");
    config.Port = portEnv ? portEnv : defaultPort;

    return config;
}