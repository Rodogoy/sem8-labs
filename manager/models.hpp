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
    std::vector<std::string> Data;
    
    nlohmann::json toJson() const;
};

class TaskStorage {
private:
    mongocxx::collection RequestToHashCollection;
    mongocxx::collection HashToStatusCollection;
    mongocxx::collection PartResultsCollection;
    mongocxx::collection PartCountsCollection;
    std::mutex mu;
    mongocxx::collection Status;

public:
    TaskStorage(mongocxx::database db);
    bool AddTask(std::string requestId, const std::string& hash);
    std::pair<StatusResponse, bool> GetStatus(const std::string& requestId);
    void SetPartCount(const std::string& hash, int count);
    void AddPartResult(const std::string& hash, int partNumber, const std::string& result);
}; 

extern std::shared_ptr<TaskStorage> GlobalTaskStorage;

extern mongocxx::collection globals;
extern mongocxx::options::update options;
const int partCoefficient = 5;
extern int max_id;
extern int current_id;
extern int complite_id;

#endif