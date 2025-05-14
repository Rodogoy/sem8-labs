#include "handlers.hpp"
#include "dispatcher.hpp"

#include "httplib.h"

mongocxx::instance inst{};
mongocxx::uri uri("mongodb://mongo1:27017,mongo2:27017,mongo3:27017/?replicaSet=rs0&readPreference=primary");
mongocxx::client client(uri);

auto db = client["testdb"];
mongocxx::collection globals = db["globals"];
mongocxx::options::update options;
std::shared_ptr<TaskDispatcher> dispatcher = std::make_shared<TaskDispatcher>(taskQueue, db);
std::shared_ptr<TaskStorage> GlobalTaskStorage = std::shared_ptr<TaskStorage>(new TaskStorage(db));
std::shared_ptr<TaskQueue> taskQueue = std::shared_ptr<TaskQueue>(new TaskQueue(db));
int max_id = 0;
int current_id = 0;
int complite_id = 0;

void InitGlobalsAndDb()
{
	try {
	    auto admin = client["admin"];
	    auto ping_cmd = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("ping", 1));
	    admin.run_command(ping_cmd.view());
	    
	    std::cout << "Successfully connected to MongoDB replica set" << std::endl;
	} catch (const std::exception& e) {
	    std::cerr << "MongoDB connection error: " << e.what() << std::endl;
	}
    options.upsert(true);

    auto max = globals.find_one(bsoncxx::builder::stream::document{} 
        << "name" << "max_id" 
        << bsoncxx::builder::stream::finalize);

    if(!max)
    {   
        auto doc = bsoncxx::builder::stream::document{} 
            << "name" << "max_id"
            << "value" << 0
            << bsoncxx::builder::stream::finalize;
        globals.insert_one(doc.view());
    }
    else
        max_id = max->view()["value"].get_int32().value;

    auto current = globals.find_one(bsoncxx::builder::stream::document{} 
        << "name" << "current_id" << 
        bsoncxx::builder::stream::finalize);
    if(!current)
    {   
        auto doc = bsoncxx::builder::stream::document{} 
            << "name" << "current_id"
            << "value" << 0
            << bsoncxx::builder::stream::finalize;
        globals.insert_one(doc.view());
    }
    else
        current_id = current->view()["value"].get_int32().value;

    auto complite = globals.find_one(bsoncxx::builder::stream::document{} 
        << "name" << "complite_id" << 
        bsoncxx::builder::stream::finalize);
    if(!complite)
    {   
        auto doc = bsoncxx::builder::stream::document{} 
            << "name" << "complite_id"
            << "value" << 0
            << bsoncxx::builder::stream::finalize;
        globals.insert_one(doc.view());
    }
    else
        complite_id = complite->view()["value"].get_int32().value;
}