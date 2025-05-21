#include <iostream>
#include <memory>
#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>
#include <amqpcpp/libev.h> 
#include <ev.h>

#include "handlers.hpp"

#include "httplib.h"

#ifndef ROUNDROBIN
#define ROUNDROBIN

struct WorkerInfo {
    std::string name;
    int port;
    int MaxWorkers;
    int ActiveTasks;
    std::string ActiveHash;
    int taskPartNumber;
};

class RoundRobin {
private:
    std::vector<std::shared_ptr<WorkerInfo>> workers;
    std::mutex mu;
    std::condition_variable workerReady;

public:
    RoundRobin();
    std::vector<std::shared_ptr<WorkerInfo>> GetWorkers();
    void RegisterWorker(const std::string& url, int port, int maxWorkers);
    std::shared_ptr<WorkerInfo> GetNextWorker();
    void DeleteWorker(std::shared_ptr<WorkerInfo> worker);
    void TaskCompleted(const std::string& workerName);
};

extern std::shared_ptr<RoundRobin> LoadBalancer;
#endif