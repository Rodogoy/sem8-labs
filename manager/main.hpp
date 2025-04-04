#include <map>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "json.hpp"
#include "httplib.h"

#ifndef CrackTask
#define CrackTask

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
    std::vector<std::string> badTasks;
    
    nlohmann::json toJson() const;
};

class TaskQueue {
private:
    std::queue<CrackTaskRequest> tasks;
    std::mutex mu;
    std::condition_variable notEmpty;

public:
    TaskQueue();
    void Push(const CrackTaskRequest& task);
    std::shared_ptr<CrackTaskRequest> Pop();
};

struct WorkerInfo {
    std::string name;
    int port;
    int MaxWorkers;
    int ActiveTasks;
    std::string ActiveHash;
    int taskPartNumber;
};

class TaskStorage {
private:
    std::map<std::string, std::string> requestToHash;
    std::map<std::string, StatusResponse> hashToStatus;
    std::map<std::string, std::map<int, std::string>> partResults;
    std::map<std::string, int> partCounts;
    std::mutex mu;

public:
    TaskStorage();
    bool AddTask(std::string requestId, const std::string& hash);
    std::pair<StatusResponse, bool> GetStatus(const std::string& requestId);
    void UpdateStatus(const std::string& hash, const std::string& status, const std::vector<std::string>& result);
    void SetPartCount(const std::string& hash, int count);
    void CloseTask(std::shared_ptr<WorkerInfo> worker);
    void AddPartResult(const std::string& hash, int partNumber, const std::string& result);
}; 

class TaskDispatcher {
private:
    std::shared_ptr<TaskQueue> taskQueue;
    std::map<std::string, std::map<int, std::string>> partToWorker;
    std::mutex mu;

public:
    TaskDispatcher(std::shared_ptr<TaskQueue> queue);
    void Start();
    void dispatchTasks();
    void dispatchChecks();
    bool isHttpServerAlive(std::shared_ptr<WorkerInfo> worker);
    void sendTaskToWorker(const std::string& workerName, const int workerPort, const CrackTaskRequest& task);
    void DecrementWorkerTasks(const std::string& workerName);
    std::string GetWorkerByPart(const std::string& hash, int partNumber);
};

class RoundRobin {
private:
    std::vector<std::shared_ptr<WorkerInfo>> workers;
    std::mutex mu;
    std::condition_variable workerReady;

public:
    RoundRobin();
    std::vector<std::shared_ptr<WorkerInfo>> GetWorkers();
    void RegisterWorker(const std::string& url, int port, int maxWorkers);
    std::shared_ptr<WorkerInfo> GetNextWorker();
    void DeleteWorker(std::shared_ptr<WorkerInfo> worker);
    void TaskCompleted(const std::string& workerName);
};

extern std::shared_ptr<TaskQueue> taskQueue;
extern std::shared_ptr<TaskDispatcher> dispatcher;
extern std::shared_ptr<TaskStorage> GlobalTaskStorage;
extern std::shared_ptr<RoundRobin> LoadBalancer;
const int partCoefficient = 50;

void CrackHashHandler(const httplib::Request& req, httplib::Response& res);
void ResultHandler(const httplib::Request& req, httplib::Response& res);
void StatusHandler(const httplib::Request& req, httplib::Response& res);
void WorkerRegisterHandler(const httplib::Request& req, httplib::Response& res);

void Start(std::shared_ptr<TaskQueue> taskQueue, std::shared_ptr<TaskDispatcher> taskDispatcher);
#endif