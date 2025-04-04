#include <iostream>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <algorithm>

#include "main.hpp"


RoundRobin::RoundRobin() = default;

void RoundRobin::RegisterWorker(const std::string& url, int port, int maxWorkers) {
    std::unique_lock<std::mutex> lock(mu);

    std::cout << "Registering new worker: " << url << " with max tasks: " << maxWorkers << std::endl;
    auto worker = std::make_shared<WorkerInfo>(WorkerInfo{ url, port, maxWorkers, 0 });
    workers.push_back(worker);
    std::cout << "Total registered workers: " << workers.size() << std::endl;

    for (int i = 0; i < maxWorkers; ++i) {
        workerReady.notify_one();
    }
}

std::vector<std::shared_ptr<WorkerInfo>> RoundRobin::GetWorkers() {
    return workers;
}

std::shared_ptr<WorkerInfo> RoundRobin::GetNextWorker() {
    std::unique_lock<std::mutex> lock(mu);

    workerReady.wait(lock, [this]() {
        for (const auto& worker : workers) {
            if (worker->ActiveTasks < worker->MaxWorkers) {
                return true;
            }
        }
        return false;
        });

    auto selectedWorker = *std::min_element(workers.begin(), workers.end(),
        [](const std::shared_ptr<WorkerInfo>& a, const std::shared_ptr<WorkerInfo>& b) {
            return a->ActiveTasks < b->ActiveTasks;
        });

    if (selectedWorker) {
        selectedWorker->ActiveTasks++;
    }
    return selectedWorker;
}
void RoundRobin::DeleteWorker(std::shared_ptr<WorkerInfo> worker) {
    std::unique_lock<std::mutex> lock(mu);
    for (std::vector<std::shared_ptr<WorkerInfo>>::iterator i = workers.begin(); i != workers.end(); ++i)
        if(worker == *i){
            workers.erase(i);
            break;
        }


}

void RoundRobin::TaskCompleted(const std::string& workerName) {
    std::unique_lock<std::mutex> lock(mu);

    for (const auto& worker : workers) {
        if (worker->name == workerName) {
            if (worker->ActiveTasks > 0) {
                worker->ActiveTasks--;
                workerReady.notify_one();
            }
            break;
        }
    }
}

