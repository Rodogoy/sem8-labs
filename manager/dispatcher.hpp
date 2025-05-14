#include <mutex>

#include "queue.hpp"

#ifndef DISPATCHER
#define DISPATCHER

class TaskDispatcher {
private:
    std::shared_ptr<TaskQueue> taskQueue;
    std::mutex mu;

public:
    TaskDispatcher(std::shared_ptr<TaskQueue> queue, mongocxx::database db);
    void Start();
    void dispatchTasks();
    void sendTaskToWorker(CrackTaskRequest task);
    void DecrementWorkerTasks(const std::string& workerName);
    std::string GetWorkerByPart(const std::string& hash, int partNumber);
    void DeleteWorkerByPart(const std::string& hash, int partNumber);
    mongocxx::collection initIdToReqIdCollection;
};

extern std::shared_ptr<TaskDispatcher> dispatcher;

#endif