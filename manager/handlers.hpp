#include <memory>
#include <map>
#include <queue>
#include <iostream>
#include <string>

#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>
#include <amqpcpp/libev.h> 
#include <ev.h>

#include "json.hpp"
#include "httplib.h"

#ifndef HANDLERS
#define HANDLERS


void CrackHashHandler(const httplib::Request& req, httplib::Response& res);
void StatusHandler(const httplib::Request& req, httplib::Response& res);
void ResultHandler(const AMQP::Message &message, uint64_t deliveryTag);
void WorkerRegisterHandler(const AMQP::Message &message, uint64_t deliveryTag);

void Start();
void InitGlobalsAndDb();

#endif