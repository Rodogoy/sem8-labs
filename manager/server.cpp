#include "handlers.hpp"
#include "models.hpp"

void Start() {

    httplib::Server svr;

    svr.Post("/api/hash/crack", [&](const httplib::Request& req, httplib::Response& res) {
        CrackHashHandler(req, res);
        });

    svr.Post("/api/hash/status", [&](const httplib::Request& req, httplib::Response& res) {
        StatusHandler(req, res);
        });

    svr.Post("/internal/api/manager/hash/crack/result", [&](const httplib::Request& req, httplib::Response& res) {
        ResultHandler(req, res);
        });

    svr.Post("/internal/api/worker/register", [&](const httplib::Request& req, httplib::Response& res) {
        WorkerRegisterHandler(req, res);
        });

    std::cout << "Manager listening on port :8080" << std::endl;
    if (!svr.listen("manager", 8080)) {
        std::cerr << "Server error: Failed to start server" << std::endl;
    }
}