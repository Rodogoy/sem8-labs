#include "httplib.h"
#include "json.hpp"

#include "handlers.hpp"
#include "models.hpp"
#include "queue.hpp"

void CrackHashHandler(const httplib::Request& req, httplib::Response& res) {
    try {
        auto jsonBody = nlohmann::json::parse(req.body);
        std::string hash = jsonBody["hash"];
        int maxLength = jsonBody["maxLength"];
        std::string s_requestId = hash + std::to_string(maxLength);

        bool needWorker = GlobalTaskStorage->AddTask(s_requestId, hash);

        if (needWorker) {
            int partCount = maxLength * partCoefficient;
            GlobalTaskStorage->SetPartCount(hash, partCount);

            for (int i = 1; i <= partCount; ++i) {
                CrackTaskRequest task{ hash, maxLength, i, partCount };
                taskQueue->Push(task);
            }
        }

        nlohmann::json response = {
            {"requestId", s_requestId}
        };
        res.set_content(response.dump(), "application/json");
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to process request: " << e.what() << std::endl;
        res.status = 400;
        res.set_content("Invalid request body", "text/plain");
    }
 }