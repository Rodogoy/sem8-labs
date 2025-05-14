#include "dispatcher.hpp"
    
void TaskQueue::saveTaskToDB(const CrackTaskRequest& task, int id) {
    tasksCollection.update_one(
        bsoncxx::builder::stream::document{} 
            << "task_id" << id 
            << "hash" << task.Hash
            << "part_number" << task.PartNumber
            << bsoncxx::builder::stream::finalize,
        bsoncxx::builder::stream::document{} 
            << "$set" << bsoncxx::builder::stream::open_document 
            << "max_length" << task.MaxLength
            << "part_count" << task.PartCount
            << bsoncxx::builder::stream::close_document 
            << bsoncxx::builder::stream::finalize,
        options);
}
TaskQueue::TaskQueue(mongocxx::database db)
{
    tasksCollection = db["tasks"];
    mongocxx::options::find opts;
    opts.sort( bsoncxx::builder::stream::document{} 
        << "task_id" << 1 
        << bsoncxx::builder::stream::finalize);
    
    auto cursor = tasksCollection.find({}, opts);
    
    for (auto&& doc : cursor) {
        CrackTaskRequest task;
        int t_id;
        if (doc["task_id"]) t_id = doc["task_id"].get_int32().value;
        if(t_id < current_id)
            continue;
        std::string hash{doc["hash"].get_string().value};
        task.Hash = hash;
        if (doc["max_length"]) task.MaxLength = doc["max_length"].get_int32().value;
        if (doc["part_number"]) task.PartNumber = doc["part_number"].get_int32().value;
        if (doc["part_count"]) task.PartCount = doc["part_count"].get_int32().value;
        //std::cout<< hash <<" "<< doc["max_length"].get_int32().value<<" "<< doc["part_number"].get_int32().value<<" "<< doc["part_count"].get_int32().value<<std::endl;
        tasks.push_back(task);
    }
} 

void TaskQueue::Push(const CrackTaskRequest& task, int id) {
    std::unique_lock<std::mutex> lock(mu);
    tasks.push_back(task);
    saveTaskToDB(task, id);
    notEmpty.notify_one();
}

std::shared_ptr<CrackTaskRequest> TaskQueue::Take() {
    std::unique_lock<std::mutex> lock(mu);
    notEmpty.wait(lock, [this]() { return !tasks.empty(); });
    auto task = std::make_shared<CrackTaskRequest>(tasks.front());
    tasks.pop_front();
    return task;
}

bool TaskQueue::FindCompletedTask(std::string Hash, int PartNumber) {
    mongocxx::options::find opts;
    opts.max_time(std::chrono::milliseconds(500)); 
    std::cout << "Processing Hash: " << Hash << " Part: " << PartNumber 
          << " Collection size: " << tasksCollection.count_documents({}) 
          << std::endl;
    return static_cast<bool>(tasksCollection.find_one(bsoncxx::builder::stream::document{} 
            << "hash" << Hash 
            << "part_number" << PartNumber 
            << bsoncxx::builder::stream::finalize, opts));
}

void TaskQueue::InitTaskDb() {
    tasksCollection.insert_one(bsoncxx::builder::stream::document{} 
        << "hash" << "hash" << "count" << 0 
        << bsoncxx::builder::stream::finalize);
}

void TaskQueue::PopCompletedTask(std::string Hash, int PartNumber) {
    try {
        if (static_cast<bool>(tasksCollection.delete_one(bsoncxx::builder::stream::document{} 
            << "hash" << Hash 
            << "part_number" << PartNumber 
            << bsoncxx::builder::stream::finalize)))
            std::cout << "task found with Hash, " << Hash <<", " << PartNumber << " and was deleted from MongoDB" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "MongoDB delete error: " << e.what() << std::endl;
    }
}

void TakeAndSendTaskToWorkers(int partCount)
{
    for (int i = 0; i < partCount; ++i)
    {
        std::shared_ptr<CrackTaskRequest> task = taskQueue->Take();
        dispatcher->sendTaskToWorker(*task);
    }
}
