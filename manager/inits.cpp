#include "handlers.hpp"
#include "dispatcher.hpp"
#include "rr_balancer.hpp"

#include "httplib.h"

std::shared_ptr<TaskQueue> taskQueue = std::make_shared<TaskQueue>();
std::shared_ptr<RoundRobin> LoadBalancer = std::make_shared<RoundRobin>();
std::shared_ptr<TaskDispatcher> dispatcher = std::make_shared<TaskDispatcher>(taskQueue);