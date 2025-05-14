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
    RequestToHashCollection(db["request_to_hash"]),
    HashToStatusCollection(db["hash_to_status"]),
    PartResultsCollection(db["part_results"]),
    PartCountsCollection(db["part_counts"]) {
}

bool TaskStorage::AddTask(std::string requestId, const std::string& hash) {
    std::unique_lock<std::mutex> lock(mu);

    std::cout << "[TaskStorage] Adding new task. RequestID: " << requestId << ", Hash: " << hash << std::endl;

    RequestToHashCollection.update_one(
        bsoncxx::builder::stream::document{} 
            << "request_id" << requestId 
            << bsoncxx::builder::stream::finalize,
        bsoncxx::builder::stream::document{} 
            << "$set" << bsoncxx::builder::stream::open_document 
            << "hash" << hash 
            << bsoncxx::builder::stream::close_document 
            << bsoncxx::builder::stream::finalize,
        options);
    auto result = HashToStatusCollection.find_one(bsoncxx::builder::stream::document{} 
            << "hash" << hash 
            << bsoncxx::builder::stream::finalize);
        if (result->view()["status"]) {
        std::string status{result->view()["status"].get_string().value};
        if (status == "READY") {
            std::cout << "[TaskStorage] Hash " << hash << " already processed, returning result" << std::endl;
            return false;
        }
        if (status == "IN_PROGRESS") {
            std::cout << "[TaskStorage] Hash " << hash << " already in progress" << std::endl;
            return false;
        }
        if (status == "FAIL") {
            std::cout << "[TaskStorage] Hash " << hash << " already processed, and it have no answer" << std::endl;
            return false;
        }
    }

    HashToStatusCollection.update_one(
        bsoncxx::builder::stream::document{} 
            << "hash" << hash 
            << bsoncxx::builder::stream::finalize,
        bsoncxx::builder::stream::document{} 
            << "$set" << bsoncxx::builder::stream::open_document 
            << "status" << "IN_PROGRESS" 
            << "data" << "0%" 
            << bsoncxx::builder::stream::close_document 
            << bsoncxx::builder::stream::finalize,
        options);
    std::cout << "[TaskStorage] Started processing for hash " << hash << std::endl;
    return true;
}

std::pair<StatusResponse, bool> TaskStorage::GetStatus(const std::string& requestId) {
    std::unique_lock<std::mutex> lock(mu);
    auto RTHresult = RequestToHashCollection.find_one(bsoncxx::builder::stream::document{} 
            << "request_id" << requestId 
            << bsoncxx::builder::stream::finalize);
    if (!RTHresult) {
        std::cout << "[TaskStorage] Request ID " << requestId << " not found" << std::endl;
        return { StatusResponse{}, false };
    }

    std::string hash{RTHresult->view()["hash"].get_string().value};

    auto HTSresult = HashToStatusCollection.find_one(bsoncxx::builder::stream::document{} 
        << "hash" << hash 
        << bsoncxx::builder::stream::finalize);
    std::string status{HTSresult->view()["status"].get_string().value};

    mongocxx::cursor cursor = HashToStatusCollection.find(
        bsoncxx::builder::stream::document{} 
            << "hash" << hash 
            << bsoncxx::builder::stream::finalize);

    std::vector<std::string> data;
    for (auto&& doc : cursor){
        std::string tmp{doc["data"].get_string().value};
        data.push_back(tmp);
    } 
        
    std::cout << "[TaskStorage] Status for request " << requestId << " (hash " << hash << "): "
        << status << " " << data.size() << " results" << std::endl;
    return { StatusResponse{ status, data }, true };
}

void TaskStorage::SetPartCount(const std::string& hash, int count) {
    std::unique_lock<std::mutex> lock(mu);

    auto result = PartCountsCollection.find_one(bsoncxx::builder::stream::document{} 
            << "hash" << hash 
            << bsoncxx::builder::stream::finalize);

    if (!result) {
        PartCountsCollection.insert_one(bsoncxx::builder::stream::document{} 
            << "hash" << hash << "count" << count 
            << bsoncxx::builder::stream::finalize);
    }
}

void TaskStorage::AddPartResult(const std::string& hash, int partNumber, const std::string& result) {
    {
        std::unique_lock<std::mutex> lock(mu);
        PartResultsCollection.update_one(
        bsoncxx::builder::stream::document{} 
            << "hash" << hash 
            << "partNumber" << partNumber 
            << bsoncxx::builder::stream::finalize,
        bsoncxx::builder::stream::document{} 
            << "$set" << bsoncxx::builder::stream::open_document 
            << "result" << result 
            << bsoncxx::builder::stream::close_document 
            << bsoncxx::builder::stream::finalize,
        options
        );
    }

    int64_t count_docs = PartResultsCollection.count_documents(
        bsoncxx::builder::stream::document{} 
            << "hash" << hash 
            << bsoncxx::builder::stream::finalize
    );

    auto res = PartCountsCollection.find_one(bsoncxx::builder::stream::document{} 
        << "hash" << hash 
        << bsoncxx::builder::stream::finalize);
    int pCount = res->view()["count"].get_int32().value;
    double progress = (double)count_docs / pCount * 100;

    auto hashStat = HashToStatusCollection.find_one(bsoncxx::builder::stream::document{} 
        << "hash" << hash 
        << bsoncxx::builder::stream::finalize);
    std::string status{hashStat->view()["status"].get_string().value};

    if (!result.empty())
    {
        std::unique_lock<std::mutex> lock(mu);
        if(status != "READY")
        {
            HashToStatusCollection.update_one(
                bsoncxx::builder::stream::document{} 
                    << "hash" << hash 
                    << bsoncxx::builder::stream::finalize,
                bsoncxx::builder::stream::document{} 
                    << "$set" << bsoncxx::builder::stream::open_document 
                    << "status" << "READY" 
                    << "data" << result 
                    << bsoncxx::builder::stream::close_document 
                    << bsoncxx::builder::stream::finalize);
            
            globals.update_one(
                bsoncxx::builder::stream::document{} 
                    << "name" << "complite_id" 
                    << bsoncxx::builder::stream::finalize,
                bsoncxx::builder::stream::document{} 
                    << "$set" << bsoncxx::builder::stream::open_document 
                    << "value" << complite_id + 1 
                    << bsoncxx::builder::stream::close_document 
                    << bsoncxx::builder::stream::finalize);
            complite_id++;
            return;
        }
        else
        {
            HashToStatusCollection.insert_one(
                bsoncxx::builder::stream::document{} 
                    << "hash" << hash 
                    << "status" << "READY" 
                    << "data" << result  
                    << bsoncxx::builder::stream::finalize);
            return;
        }
    }

    if (count_docs == pCount && status != "READY") {
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
                     
        globals.update_one(
            bsoncxx::builder::stream::document{} 
                << "name" << "complite_id" 
                << bsoncxx::builder::stream::finalize,
            bsoncxx::builder::stream::document{} 
                << "$set" << bsoncxx::builder::stream::open_document 
                << "value" << complite_id + 1 
                << bsoncxx::builder::stream::close_document 
                << bsoncxx::builder::stream::finalize);
        complite_id++;
        return;
    }
    if (status != "READY") {
        std::stringstream progressStr;
        progressStr << std::fixed << std::setprecision(1) << progress << "%";
        HashToStatusCollection.update_one(
            bsoncxx::builder::stream::document{} 
                << "hash" << hash 
                << bsoncxx::builder::stream::finalize,
            bsoncxx::builder::stream::document{} 
                << "$set" << bsoncxx::builder::stream::open_document 
                << "status" << "IN_PROGRESS" 
                << "data" << progressStr.str() 
                << bsoncxx::builder::stream::close_document 
                << bsoncxx::builder::stream::finalize);
        return;
    }
}