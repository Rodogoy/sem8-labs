#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

#include "main.hpp"


TaskQueue::TaskQueue() = default;

void TaskQueue::Push(const CrackTaskRequest& task) {
    std::unique_lock<std::mutex> lock(mu);
    tasks.push(task);
    notEmpty.notify_one(); 
}

std::shared_ptr<CrackTaskRequest> TaskQueue::Pop() {
    std::unique_lock<std::mutex> lock(mu);
    notEmpty.wait(lock, [this]() { return !tasks.empty(); });
    auto task = std::make_shared<CrackTaskRequest>(tasks.front());
    tasks.pop();
    return task;
}