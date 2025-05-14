#include <iostream>
#include <cstdlib>
#include <string>
#include <semaphore>
#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>
#include <amqpcpp/libev.h> 
#include <ev.h>
#include <memory>
#include <stdexcept>

#include "json.hpp"
#include "httplib.h"
#ifndef MAIN
#define MAIN

void CreateCrackTaskHandler(const AMQP::Message &message, uint64_t deliveryTag);

#endif