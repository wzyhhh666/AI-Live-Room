#pragma once

#include <hiredis/hiredis.h>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "common/result.h"

namespace chatroom {
namespace data {

struct RedisConfig {
    std::string host = "127.0.0.1";
    uint16_t port = 6379;
    std::string password = "";
    int db = 0;
    size_t poolMin = 2;
    size_t poolMax = 16;
    int connectTimeoutSec = 2;
    int retryCount = 3;
};

class RedisPool {
public:
    static RedisPool& getInstance();
    
    Result<void> initialize(const RedisConfig& config);
    Result<redisContext*> getContext();
    void returnContext(redisContext* ctx);
    
    size_t getAvailableCount() const;
    void shutdown();

private:
    RedisPool() = default;
    ~RedisPool() { shutdown(); }
    
    Result<redisContext*> createContext();
    
    std::queue<redisContext*> m_pool;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    
    RedisConfig m_config;
    bool m_initialized = false;
};

} // namespace data
} // namespace chatroom
