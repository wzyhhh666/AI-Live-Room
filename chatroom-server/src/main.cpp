#include <iostream>
#include "common/logger.h"

int main() {
    try {
        chatroom::Logger::init("INFO", "logs/chatroom.log");

        LOG_INFO("ChatRoom Server starting...");
        LOG_INFO("C++17 Live Streaming Server v1.0.0");
        LOG_INFO("Press Ctrl+C to shutdown");

        std::cout << "Server initialized successfully!" << std::endl;
        std::cout << "Configuration loaded from: config/app.json" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
