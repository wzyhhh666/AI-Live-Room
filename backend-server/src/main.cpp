#include "common/logging.h"
#include "util/config_manager.h"
#include "net/epoll_server.h"
#include <iostream>
#include <csignal>
#include <thread>

using namespace chatroom;

static volatile sig_atomic_t g_running = 1;
static bool g_loggerInitialized = false;
static std::unique_ptr<net::EpollServer> g_server;

void signalHandler(int signum) {
    if (g_loggerInitialized) {
        LOG_INFO("Received signal {}, shutting down...", signum);
    } else {
        std::cout << "Received signal " << signum << ", shutting down..." << std::endl;
    }
    g_running = 0;
    
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    std::string configPath = "config/app.json";
    if (argc > 1) {
        configPath = argv[1];
    }

    try {
        auto& logger = Logger::getInstance();
        
        logger.initialize("/dev/null", spdlog::level::info, spdlog::level::off);
        g_loggerInitialized = true;

        auto& configMgr = ConfigManager::getInstance();

        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        auto loadResult = configMgr.loadFromFile(configPath);
        if (!loadResult.isOk()) {
            std::cerr << "Failed to load config: " << loadResult.message() << std::endl;
            return 1;
        }

        const auto& logConfig = configMgr.getLog();
        spdlog::level::level_enum consoleLevel = spdlog::level::info;
        if (logConfig.level == "DEBUG") consoleLevel = spdlog::level::debug;
        else if (logConfig.level == "TRACE") consoleLevel = spdlog::level::trace;

        logger.initialize(logConfig.filePath, consoleLevel);

        LOG_INFO("========================================");
        LOG_INFO("  Chatroom Server v1.0 Starting...");
        LOG_INFO("========================================");
        LOG_INFO("Server: {}:{}", configMgr.getServer().host, configMgr.getServer().port);
        LOG_INFO("MySQL:  {}:{}", configMgr.getMysql().host, configMgr.getMysql().port);
        LOG_INFO("Redis:  {}:{}", configMgr.getRedis().host, configMgr.getRedis().port);

        g_server = std::make_unique<net::EpollServer>();
        
        auto initResult = g_server->init(
            configMgr.getServer().host,
            configMgr.getServer().port,
            configMgr.getServer().backlog
        );
        
        if (!initResult.isOk()) {
            LOG_ERROR("Failed to initialize server: {}", initResult.message());
            return 1;
        }

        LOG_INFO("Server initialized successfully");
        LOG_INFO("Starting server on port {}...", configMgr.getServer().port);

        auto startResult = g_server->start();
        if (!startResult.isOk()) {
            LOG_ERROR("Failed to start server: {}", startResult.message());
            return 1;
        }

        LOG_INFO("Server is running. Press Ctrl+C to stop...");

        while (g_running && g_server->isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        LOG_INFO("Shutting down server...");
        
        if (g_server) {
            g_server->stop();
        }
        
        logger.flush();

        LOG_INFO("Server stopped gracefully");
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
