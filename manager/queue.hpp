#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>

#include "models.hpp"

#ifndef QUEUE
#define QUEUE

class TaskQueue {
private:
    std::deque<CrackTaskRequest> tasks;
    std::mutex mu;
    std::condition_variable notEmpty;
    mongocxx::collection tasksCollection;
    void saveTaskToDB(const CrackTaskRequest& task, int id);

public:
    TaskQueue(mongocxx::database db);
    void InitTaskDb();
    void Push(const CrackTaskRequest& task, int id);
    void Return_into_queue(const CrackTaskRequest& task);
    std::shared_ptr<CrackTaskRequest> Take();
    void PopCompletedTask(std::string Hash, int PartNumber);
    bool FindCompletedTask(std::string Hash, int PartNumber);
};

void TakeAndSendTaskToWorkers(int partCount);

extern std::shared_ptr<TaskQueue> taskQueue;

#endif