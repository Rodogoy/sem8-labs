#include <iostream>
#include <memory>
#include <thread>
#include "main.hpp"
#include "httplib.h"

extern std::shared_ptr<TaskQueue> taskQueue = std::make_shared<TaskQueue>();
extern std::shared_ptr<TaskStorage> GlobalTaskStorage = std::make_shared<TaskStorage>();
extern std::shared_ptr<RoundRobin> LoadBalancer = std::make_shared<RoundRobin>();

int main() {

    dispatcher = std::make_shared<TaskDispatcher>(taskQueue);
    dispatcher->Start();

    Start(taskQueue, dispatcher);

    return 0;
}