#include <iostream>
#include <memory>

#include "httplib.h"
#include "main.hpp" 

void StartServer(const Config& cfg) {
    httplib::Server svr;

    svr.Post("/internal/api/worker/hash/crack/task", [&](const httplib::Request& req, httplib::Response& res) {
        CreateCrackTaskHandler(req, res);
        });

    std::cout << "Starting worker server on port " << cfg.Port
        << " with " << 1 << " max workers" << std::endl;

    if (!svr.listen(cfg.WorkerURL, std::stoi(cfg.Port))) {
        std::cerr << "Failed to start server on port " << cfg.Port << std::endl;
    }
}
