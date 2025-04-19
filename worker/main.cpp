#include <iostream>
#include <memory>
#include <stdexcept>

#include "main.hpp" 

int main() {
    try {
        Config cfg = LoadConfig();

        try {
            RegisterWithManager(cfg.WorkerURL, defaultMaxWorkers);
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to register with the manager: " << e.what() << std::endl;
            return 1;
        }

        StartServer(cfg);

    }
    catch (const std::exception& e) {
        std::cerr << "Error of init: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
