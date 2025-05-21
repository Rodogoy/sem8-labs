#include "json.hpp"
#include "httplib.h"

#include "models.hpp"
nlohmann::json CrackTaskRequest::toJson() const {
    return {
        {"hash", Hash},
        {"maxLength", MaxLength},
        {"partNumber", PartNumber},
        {"partCount", PartCount}
    };
}

nlohmann::json StatusResponse::toJson() const {
    return {
        {"status", Status},
        {"data", Data}
    };
}

TaskStorage::TaskStorage(mongocxx::database db):
    HashToStatusCollection(db["hash_to_status"]){
}
bool TaskStorage::AddTask(std::string requestId, const std::string& hash) {
    std::unique_lock<std::mutex> lock(mu);

    std::cout << "[TaskStorage] Adding new task. RequestID: " << requestId << ", Hash: " << hash << std::endl;
    requestToHash[requestId] = hash;

    auto result = HashToStatusCollection.find_one(bsoncxx::builder::stream::document{} 
            << "hash" << hash 
            << bsoncxx::builder::stream::finalize);

    if (result->view()["status"]) {
        std::string status{result->view()["status"].get_string().value};
        if (status == "READY") {
            std::cout << "[TaskStorage] Hash " << hash << " already processed, returning result" << std::endl;
            return false;
        }
        if (status == "PART_READY") {
            std::cout << "[TaskStorage] Hash " << hash << " already part processed, returning uncomplite tasks" << std::endl;
            return false;
        }
        if (status == "IN_PROGRESS") {
            std::cout << "[TaskStorage] Hash " << hash << " already in progress" << std::endl;
            return false;
        }
        if (status == "FAIL") {
            std::cout << "[TaskStorage] Hash " << hash << " already processed, hash has no result" << std::endl;
            return false;
        }
    }
    options.upsert(true);
    HashToStatusCollection.update_one(
        bsoncxx::builder::stream::document{} 
            << "hash" << hash 
            << bsoncxx::builder::stream::finalize,
        bsoncxx::builder::stream::document{} 
            << "$set" << bsoncxx::builder::stream::open_document 
            << "status" << "IN_PROGRESS" 
            << "data" << "0%"  
            << "badtasks" << "" 
            << "countbadtasks" << 0 
            << bsoncxx::builder::stream::close_document 
            << bsoncxx::builder::stream::finalize,
        options);
    std::cout << "[TaskStorage] Started processing for hash " << hash << std::endl;
    return true;
}

std::pair<StatusResponse, bool> TaskStorage::GetStatus(const std::string& requestId) {
    std::unique_lock<std::mutex> lock(mu);

    if (requestToHash.find(requestId) == requestToHash.end()) {
        std::cout << "[TaskStorage] Request ID " << requestId << " not found" << std::endl;
        return { StatusResponse{}, false };
    }

    auto hash = requestToHash[requestId];

    auto result = HashToStatusCollection.find_one(bsoncxx::builder::stream::document{} 
        << "hash" << hash 
        << bsoncxx::builder::stream::finalize);

    std::string status{result->view()["status"].get_string().value}; 
    std::string data{result->view()["data"].get_string().value}; 
    std::string badtasks{result->view()["badtasks"].get_string().value}; 
    auto statusRes = StatusResponse{status, data, badtasks};
    std::cout << "[TaskStorage] Status for request " << requestId << " (hash " << hash << "): "
        << status << std::endl;
    return { statusRes, true };
}

void TaskStorage::CloseTask(std::shared_ptr<WorkerInfo> worker) {
    auto result = HashToStatusCollection.find_one(bsoncxx::builder::stream::document{} 
        << "hash" << worker->ActiveHash 
        << bsoncxx::builder::stream::finalize);
    std::string badtasks{result->view()["badtasks"].get_string().value}; 
    std::string actBadTasks = badtasks + std::to_string(worker->taskPartNumber) + " ";
    int countbadtasks = result->view()["countbadtasks"].get_int32().value + 1;
        
    HashToStatusCollection.update_one(
        bsoncxx::builder::stream::document{} 
            << "hash" << worker->ActiveHash  
            << bsoncxx::builder::stream::finalize,
        bsoncxx::builder::stream::document{} 
            << "$set" << bsoncxx::builder::stream::open_document 
            << "badtasks" << actBadTasks
            << "countbadtasks" << countbadtasks  
            << bsoncxx::builder::stream::close_document 
            << bsoncxx::builder::stream::finalize);
}

void TaskStorage::SetPartCount(const std::string& hash, int count) {
    std::unique_lock<std::mutex> lock(mu);

    if (partCounts.find(hash) == partCounts.end()) {
        partCounts[hash] = count;
        partResults[hash] = std::map<int, std::string>();
    }
}

void TaskStorage::AddPartResult(const std::string& hash, int partNumber, const std::string& result) {
    std::unique_lock<std::mutex> lock(mu);

    if (partResults.find(hash) == partResults.end()) {
        partResults[hash] = std::map<int, std::string>();
    }
    partResults[hash][partNumber] = result;

    double progress = (double)partResults[hash].size() / partCounts[hash] * 100;

    if (!result.empty()) {
        std::string successfulResults;
        for (const auto& [part, res] : partResults[hash]) {
            if (!res.empty()) {
                successfulResults += res + " ";
            }
        }
        if (!successfulResults.empty()) {        
            HashToStatusCollection.update_one(
                bsoncxx::builder::stream::document{} 
                    << "hash" << hash 
                    << bsoncxx::builder::stream::finalize,
                bsoncxx::builder::stream::document{} 
                    << "$set" << bsoncxx::builder::stream::open_document 
                    << "status" << "READY" 
                    << "data" << successfulResults 
                    << bsoncxx::builder::stream::close_document 
                    << bsoncxx::builder::stream::finalize);
            return;
        }
    }
    auto resultHTS = HashToStatusCollection.find_one(bsoncxx::builder::stream::document{} 
        << "hash" << hash 
        << bsoncxx::builder::stream::finalize); 
    std::string status{resultHTS->view()["status"].get_string().value}; 
    std::string badtasks{resultHTS->view()["badtasks"].get_string().value}; 
    int countbadtasks = resultHTS->view()["countbadtasks"].get_int32().value;

    if (status != "READY") {
        std::string progressStr =  std::to_string(progress) + "%";
        HashToStatusCollection.update_one(
            bsoncxx::builder::stream::document{} 
                << "hash" << hash 
                << bsoncxx::builder::stream::finalize,
            bsoncxx::builder::stream::document{} 
                << "$set" << bsoncxx::builder::stream::open_document 
                << "status" << "IN_PROGRESS" 
                << "data" << progressStr 
                << bsoncxx::builder::stream::close_document 
                << bsoncxx::builder::stream::finalize);
    }
    if (partResults[hash].size() == partCounts[hash] && status != "READY") {
        HashToStatusCollection.update_one(
            bsoncxx::builder::stream::document{} 
                << "hash" << hash 
                << bsoncxx::builder::stream::finalize,
            bsoncxx::builder::stream::document{} 
                << "$set" << bsoncxx::builder::stream::open_document 
                << "status" << "FAIL" 
                << "data" << "" 
                << bsoncxx::builder::stream::close_document 
                << bsoncxx::builder::stream::finalize);
    }
    if (!badtasks.empty() && partResults[hash].size() + countbadtasks == partCounts[hash] && status != "READY") {
        HashToStatusCollection.update_one(
            bsoncxx::builder::stream::document{} 
                << "hash" << hash 
                << bsoncxx::builder::stream::finalize,
            bsoncxx::builder::stream::document{} 
                << "$set" << bsoncxx::builder::stream::open_document 
                << "status" << "PART_READY" 
                << "data" << badtasks 
                << bsoncxx::builder::stream::close_document 
                << bsoncxx::builder::stream::finalize);
    }
}