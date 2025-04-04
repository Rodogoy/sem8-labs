#include <iostream>
#include <sstream> 
#include <map>
#include <mutex>
#include <vector>
#include <string>
#include <memory>
#include <iomanip> 

#include "json.hpp"
#include "httplib.h"

#include "main.hpp"

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

TaskStorage::TaskStorage() = default;

bool TaskStorage::AddTask(std::string requestId, const std::string& hash) {
    std::unique_lock<std::mutex> lock(mu);

    std::cout << "[TaskStorage] Adding new task. RequestID: " << requestId << ", Hash: " << hash << std::endl;
    requestToHash[requestId] = hash;

    if (hashToStatus.find(hash) != hashToStatus.end()) {
        auto& status = hashToStatus[hash];
        if (status.Status == "READY") {
            std::cout << "[TaskStorage] Hash " << hash << " already processed, returning result" << std::endl;
            return false;
        }
        if (status.Status == "PART_READY") {
            std::cout << "[TaskStorage] Hash " << hash << " already part processed, returning uncomplite tasks" << std::endl;
            return false;
        }
        if (status.Status == "IN_PROGRESS") {
            std::cout << "[TaskStorage] Hash " << hash << " already in progress" << std::endl;
            return false;
        }
    }

    hashToStatus[hash] = StatusResponse{ "IN_PROGRESS", {"0%"}, hashToStatus[hash].badTasks };
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
    auto status = hashToStatus[hash];
    std::cout << "[TaskStorage] Status for request " << requestId << " (hash " << hash << "): "
        << status.Status << " " << status.Data.size() << " results" << std::endl;
    return { status, true };
}

void TaskStorage::CloseTask(std::shared_ptr<WorkerInfo> worker) {
        hashToStatus[worker->ActiveHash].badTasks.push_back(std::to_string(worker->taskPartNumber));
}
void TaskStorage::UpdateStatus(const std::string& hash, const std::string& status, const std::vector<std::string>& result) {
    std::unique_lock<std::mutex> lock(mu);

    if (hashToStatus.find(hash) != hashToStatus.end()) {
        hashToStatus[hash] = StatusResponse{ status, result };
    }
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
        std::vector<std::string> successfulResults;
        for (const auto& [part, res] : partResults[hash]) {
            if (!res.empty()) {
                successfulResults.push_back(res);
            }
        }
        if (!successfulResults.empty()) {
            hashToStatus[hash] = StatusResponse{ "READY", successfulResults, hashToStatus[hash].badTasks };
            return;
        }
    }
        
    if (hashToStatus[hash].Status != "READY") {
        std::stringstream progressStr;
        progressStr << std::fixed << std::setprecision(1) << progress << "%";
        hashToStatus[hash] = StatusResponse{ "IN_PROGRESS", {progressStr.str()}, hashToStatus[hash].badTasks };
    }
    if (partResults[hash].size() == partCounts[hash] && hashToStatus[hash].Status != "READY") {
        hashToStatus[hash] = StatusResponse{ "FAIL", {}, hashToStatus[hash].badTasks };
    }
    if (!hashToStatus[hash].badTasks.empty() && partResults[hash].size() + hashToStatus[hash].badTasks.size() == partCounts[hash] && hashToStatus[hash].Status != "READY") {
        hashToStatus[hash] = StatusResponse{ "PART_READY", hashToStatus[hash].badTasks, hashToStatus[hash].badTasks };
    }
}