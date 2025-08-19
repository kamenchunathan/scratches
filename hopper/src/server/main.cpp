#include "engine.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    engine::Engine engine;
    
    try {
        engine.initialize();
        engine.start_server(8080);
        
        // Simple server loop
        std::cout << "Server running... Press Ctrl+C to stop." << std::endl;
        
        // In a real implementation, you'd have a proper event loop here
        for (int i = 0; i < 100; ++i) {
            engine.update();
            // std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
        
        engine.stop_server();
        engine.shutdown();
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
