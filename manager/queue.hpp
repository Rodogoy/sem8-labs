#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>

#include "models.hpp"

#ifndef QUEUE
#define QUEUE

class TaskQueue {
private:
    std::queue<CrackTaskRequest> tasks;
    std::mutex mu;
    std::condition_variable notEmpty;

public:
    TaskQueue();
    void Push(const CrackTaskRequest& task);
    std::shared_ptr<CrackTaskRequest> Pop();
};

void TakeAndSendTaskToWorkers(int partCount);

extern std::shared_ptr<TaskQueue> taskQueue;

#endif