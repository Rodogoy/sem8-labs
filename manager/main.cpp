#include "handlers.hpp"
#include "server.hpp"
#include "dispatcher.hpp"

int main() {
    InitGlobalsAndDb();
    
    dispatcher->Start();

    Start();

    return 0;
}