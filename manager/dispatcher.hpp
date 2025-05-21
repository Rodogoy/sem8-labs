#define _CRT_SECURE_NO_WARNINGS
#include <mutex>

#include "queue.hpp"
#include "rr_balancer.hpp"

#ifndef DISPATCHER
#define DISPATCHER

class TaskDispatcher {
private:
    std::shared_ptr<TaskQueue> taskQueue;
    std::map<std::string, std::map<int, std::string>> partToWorker;
    std::mutex mu;

public:
    TaskDispatcher(std::shared_ptr<TaskQueue> queue);
    void Start();
    void dispatchTasks();
    void dispatchChecks();
    bool isHttpServerAlive(std::shared_ptr<WorkerInfo> worker);
    void sendTaskToWorker(const std::string& workerName, const int workerPort, const CrackTaskRequest& task);
    void DecrementWorkerTasks(const std::string& workerName);
    std::string GetWorkerByPart(const std::string& hash, int partNumber);
};

extern std::shared_ptr<TaskDispatcher> dispatcher;

#endif