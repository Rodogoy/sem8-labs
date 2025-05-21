#include <iostream>
#include <sstream> 
#include <map>
#include <mutex>
#include <vector>
#include <string>
#include <memory>
#include <iomanip> 

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "json.hpp"
#include "httplib.h"
#include "rr_balancer.hpp"

#ifndef MODELS
#define MODELS

struct CrackTaskRequest {
    std::string Hash;
    int MaxLength;
    int PartNumber;
    int PartCount;

    nlohmann::json toJson() const;
};

struct StatusResponse {
    std::string Status;
    std::string Data;
    std::string badTasks;
    
    nlohmann::json toJson() const;
};

class TaskStorage {
private:
    std::map<std::string, std::string> requestToHash;
    mongocxx::collection HashToStatusCollection;
    std::map<std::string, std::map<int, std::string>> partResults;
    std::map<std::string, int> partCounts;
    std::mutex mu;

public:
    TaskStorage(mongocxx::database db);
    bool AddTask(std::string requestId, const std::string& hash);
    std::pair<StatusResponse, bool> GetStatus(const std::string& requestId);
    void SetPartCount(const std::string& hash, int count);
    void CloseTask(std::shared_ptr<WorkerInfo> worker);
    void AddPartResult(const std::string& hash, int partNumber, const std::string& result);
}; 

extern std::shared_ptr<TaskStorage> GlobalTaskStorage;

extern mongocxx::options::update options;
const int partCoefficient = 5;
extern int max_id;
extern int current_id;
extern int complite_id;

#endif