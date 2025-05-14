#include "dispatcher.hpp"
#include "handlers.hpp"
#include "server.hpp"

void CrackHashHandler(const httplib::Request& req, httplib::Response& res) {
    try {    
        if(!manager.getConnection()->usable()){
            std::cerr << "Failed to process request: " << std::endl;
            res.status = 400;
            res.set_content("Invalid queue", "text/plain");
            return;
        }
        auto jsonBody = nlohmann::json::parse(req.body);
        std::string hash = jsonBody["hash"];
        int maxLength = jsonBody["maxLength"];
        std::string s_requestId = hash + std::to_string(maxLength);
        if(auto doc = globals.find_one(bsoncxx::builder::stream::document{} 
            << "name" << "max_id" 
            << bsoncxx::builder::stream::finalize))      
            if (doc->view()["value"]) max_id = doc->view()["value"].get_int32().value;

        bool needWorker = GlobalTaskStorage->AddTask(s_requestId, hash);

        if (needWorker) {
            int partCount = maxLength * partCoefficient;
            GlobalTaskStorage->SetPartCount(hash, partCount);

            for (int i = 1; i <= partCount; ++i) {
                CrackTaskRequest task{ hash, maxLength, i, partCount };
                taskQueue->Push(task, max_id);
            }
            dispatcher->initIdToReqIdCollection.insert_one(bsoncxx::builder::stream::document{} 
                << "task_id" << max_id 
                << "part_count" << partCount 
                << bsoncxx::builder::stream::finalize);

            globals.update_one(
                bsoncxx::builder::stream::document{} 
                << "name" << "max_id" 
                << bsoncxx::builder::stream::finalize,
                bsoncxx::builder::stream::document{} 
                << "$set" << bsoncxx::builder::stream::open_document 
                << "value" << max_id + 1 
                << bsoncxx::builder::stream::close_document 
                << bsoncxx::builder::stream::finalize);
            max_id++;
        }
        
        nlohmann::json response = {
            {"requestId", s_requestId}
        };
        res.set_content(response.dump(), "application/json");
        res.status = 200;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to process request: " << e.what() << std::endl;
        res.status = 400;
        res.set_content("Invalid request body", "text/plain");
    }
 }