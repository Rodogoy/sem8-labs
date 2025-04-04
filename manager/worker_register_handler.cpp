#include <iostream>
#include <string>

#include "httplib.h"
#include "json.hpp"

#include "main.hpp"

void WorkerRegisterHandler(const httplib::Request& req, httplib::Response& res) {
    try {
        auto jsonBody = nlohmann::json::parse(req.body);
        std::string workerName = jsonBody["workerName"];
        int maxWorkers = jsonBody["maxWorkers"];
        int port = jsonBody["port"];

        LoadBalancer->RegisterWorker(workerName, port, maxWorkers);

        res.status = 200;
    }
    catch (const std::exception& e) {
        res.status = 400;
        res.set_content(e.what(), "text/plain");
    }
}