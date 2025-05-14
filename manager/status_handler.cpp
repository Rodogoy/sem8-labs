#include "httplib.h"
#include "json.hpp"

#include "handlers.hpp"
#include "models.hpp"

void StatusHandler(const httplib::Request& req, httplib::Response& res) {
    auto jsonBody = nlohmann::json::parse(req.body);
    std::string requestId = jsonBody["requestId"];
    if (requestId.empty()) {
        res.status = 400;
        res.set_content("Missing requestId parameter", "text/plain");
        return;
    }
    std::pair<StatusResponse, bool> status = GlobalTaskStorage->GetStatus(requestId);
    if (!status.second) {
        res.status = 404; 
        res.set_content("Request not found", "text/plain");
        return;
    }
    res.set_content(status.first.toJson().dump(), "application/json");
}