#include "dispatcher.hpp"
#include "server.hpp"

TaskDispatcher::TaskDispatcher(std::shared_ptr<TaskQueue> queue, mongocxx::database db): 
    taskQueue(queue),
    initIdToReqIdCollection(db["part_to_worker"]){
}

void TaskDispatcher::Start() {

    std::thread([this]() { dispatchTasks(); }).detach();
}

void TaskDispatcher::dispatchTasks() {
    while (true) {
        while(current_id >= max_id)
            std::this_thread::sleep_for(std::chrono::seconds(3));
        auto pc_value = initIdToReqIdCollection.find_one(bsoncxx::builder::stream::document{} 
            << "task_id" << current_id 
            << bsoncxx::builder::stream::finalize);
        TakeAndSendTaskToWorkers(pc_value->view()["part_count"].get_int32().value);
        current_id++;
    }
}

void TaskDispatcher::sendTaskToWorker(CrackTaskRequest task) {
    std::cout << "Dispatching task for hash " << task.Hash
      << " (part " << task.PartNumber << "/" << task.PartCount
      << ")" << std::endl;
    manager.publishWithRetry("complite_crack_task", task);
}