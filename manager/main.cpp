#include <iostream>
#include <memory>
#include <thread>
#include "main.hpp"
#include "httplib.h"

std::shared_ptr<TaskQueue> taskQueue = std::make_shared<TaskQueue>();
std::shared_ptr<TaskStorage> GlobalTaskStorage = std::make_shared<TaskStorage>();
std::shared_ptr<RoundRobin> LoadBalancer = std::make_shared<RoundRobin>();
std::shared_ptr<TaskDispatcher> dispatcher = std::make_shared<TaskDispatcher>(taskQueue);

int main() {
    dispatcher->Start();

    Start(taskQueue, dispatcher);

    return 0;
}
