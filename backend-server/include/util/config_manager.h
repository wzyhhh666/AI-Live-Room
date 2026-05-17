#pragma once

#include <string>
#include <cstdint>
#include "common/result.h"

namespace chatroom {

struct ServerConfig {
    std::string host = "0.0.0.0";
    uint16_t port = 8900;
    int backlog = 1024;
};

struct MysqlConfig {
    std::string host = "127.0.0.1";
    uint16_t port = 3306;
    std::string user = "chatroom";
    std::string password = "";
    std::string database = "chatroom_db";
    int poolMin = 4;
    int poolMax = 32;
    int idleTimeoutSec = 60;
    int connectTimeoutSec = 3;
};

struct RedisConfig {
    std::string host = "127.0.0.1";
    uint16_t port = 6379;
    std::string password = "";
    int db = 0;
    int poolMin = 2;
    int poolMax = 16;
    int connectTimeoutSec = 2;
    int retryCount = 3;
};

struct HeartbeatConfig {
    int clientIntervalSec = 10;
    int serverTimeoutSec = 30;
    int maxMissCount = 3;
};

struct RateLimitConfig {
    int userMaxPerSec = 1;
    int roomMaxPerSec = 1000;
};

struct RoomConfig {
    int maxOnline = 50000;
    int recentDanmakuCount = 50;
    int recentDanmakuTtlSec = 300;
};

struct FilterConfig {
    std::string wordDictPath = "config/sensitive_words.txt";
    int reloadIntervalSec = 300;
    int maxTextLength = 512;
};

struct LogConfig {
    std::string level = "INFO";
    std::string filePath = "logs/chatroom.log";
    int maxFileSizeMb = 100;
    int maxFiles = 10;
};

struct AppConfig {
    ServerConfig server;
    MysqlConfig mysql;
    RedisConfig redis;
    HeartbeatConfig heartbeat;
    RateLimitConfig rateLimit;
    RoomConfig room;
    FilterConfig filter;
    LogConfig log;
};

class ConfigManager {
public:
    static ConfigManager& getInstance();

    Result<void> loadFromFile(const std::string& configPath);
    
    const AppConfig& getConfig() const { return m_config; }
    AppConfig& getConfig() { return m_config; }

    const ServerConfig& getServer() const { return m_config.server; }
    const MysqlConfig& getMysql() const { return m_config.mysql; }
    const RedisConfig& getRedis() const { return m_config.redis; }
    const HeartbeatConfig& getHeartbeat() const { return m_config.heartbeat; }
    const LogConfig& getLog() const { return m_config.log; }

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    AppConfig m_config;
    std::string m_configPath;
};

} // namespace chatroom
