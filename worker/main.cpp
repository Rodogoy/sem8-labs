#include "server.hpp" 

int main() {
    try {
        manager.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error of init: " << e.what() << std::endl;
        return 1;
    }    
    return 0;
}
