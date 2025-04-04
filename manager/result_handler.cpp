#include <iostream>
#include <string>
#include <memory>

#include "httplib.h"
#include "json.hpp"

#include "main.hpp"

void ResultHandler(const httplib::Request& req, httplib::Response& res) {

    try {
        auto jsonBody = nlohmann::json::parse(req.body);
        std::string hash = jsonBody["hash"];
        std::string result = jsonBody["result"];
        int partNumber = jsonBody["partNumber"];
        std::string workerName = dispatcher->GetWorkerByPart(hash, partNumber);
        if (!workerName.empty()) {
            dispatcher->DecrementWorkerTasks(workerName);
        }

        GlobalTaskStorage->AddPartResult(hash, partNumber, result);

        res.status = 200;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to process request: " << e.what() << std::endl;
        res.status = 400; 
        res.set_content("Invalid request body", "text/plain");
    }
}