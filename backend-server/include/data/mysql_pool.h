#pragma once

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "common/result.h"

namespace chatroom {
namespace data {

struct MySqlConfig {
    std::string host = "127.0.0.1";
    uint16_t port = 3306;
    std::string user = "chatroom";
    std::string password = "";
    std::string database = "chatroom_db";
    size_t poolMin = 4;
    size_t poolMax = 32;
    int idleTimeoutSec = 60;
    int connectTimeoutSec = 3;
};

class MySqlConnection {
public:
    explicit MySqlConnection(MYSQL* conn);
    ~MySqlConnection();
    
    MYSQL* get() { return m_conn; }
    bool isAlive();
    
private:
    MYSQL* m_conn;
    std::chrono::steady_clock::time_point m_lastUsedTime;
};

class MySqlPool {
public:
    static MySqlPool& getInstance();
    
    Result<void> initialize(const MySqlConfig& config);
    Result<MYSQL*> getConnection();
    void returnConnection(MYSQL* conn);
    
    size_t getAvailableCount() const;
    void shutdown();

private:
    MySqlPool() = default;
    ~MySqlPool() { shutdown(); }
    
    Result<MYSQL*> createConnection();
    bool checkConnection(MYSQL* conn);
    
    std::queue<MYSQL*> m_pool;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    
    MySqlConfig m_config;
    bool m_initialized = false;
};

} // namespace data
} // namespace chatroom
