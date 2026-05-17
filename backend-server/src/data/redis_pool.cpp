#include "data/redis_pool.h"
#include "common/logging.h"

namespace chatroom {
namespace data {

RedisPool& RedisPool::getInstance() {
    static RedisPool instance;
    return instance;
}

Result<void> RedisPool::initialize(const RedisConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_config = config;
    
    for (size_t i = 0; i < config.poolMin; ++i) {
        auto result = createContext();
        if (!result.isOk()) {
            LOG_ERROR("RedisPool: failed to create initial context {}: {}", 
                     i, result.message());
            continue;
        }
        
        m_pool.push(result.value());
    }
    
    m_initialized = true;
    LOG_INFO("RedisPool: initialized with {} contexts, host={}:{}", 
             m_pool.size(), config.host, config.port);
    
    return VoidResult::ok();
}

Result<redisContext*> RedisPool::createContext() {
    struct timeval timeout = {m_config.connectTimeoutSec, 0};
    
    redisContext* ctx = redisConnectWithTimeout(
        m_config.host.c_str(),
        m_config.port,
        timeout
    );
    
    if (!ctx || ctx->err) {
        const char* error = ctx ? ctx->errstr : "Unknown error";
        if (ctx) redisFree(ctx);
        return Result<redisContext*>::fail(ErrorCode::InternalError, error);
    }
    
    if (!m_config.password.empty()) {
        redisReply* reply = static_cast<redisReply*>(
            redisCommand(ctx, "AUTH %s", m_config.password.c_str())
        );
        
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            const char* error = reply ? reply->str : "Auth failed";
            if (reply) freeReplyObject(reply);
            redisFree(ctx);
            return Result<redisContext*>::fail(ErrorCode::PermissionDenied, error);
        }
        
        if (reply) freeReplyObject(reply);
    }
    
    if (m_config.db > 0) {
        redisReply* reply = static_cast<redisReply*>(
            redisCommand(ctx, "SELECT %d", m_config.db)
        );
        if (reply) freeReplyObject(reply);
    }
    
    LOG_DEBUG("RedisPool: created new context {}", reinterpret_cast<uintptr_t>(ctx));
    return Result<redisContext*>::ok(ctx);
}

Result<redisContext*> RedisPool::getContext() {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    for (int retry = 0; retry < m_config.retryCount; ++retry) {
        if (!m_pool.empty()) {
            redisContext* ctx = m_pool.front();
            m_pool.pop();
            
            if (ctx && ctx->err == 0) {
                return Result<redisContext*>::ok(ctx);
            }
            
            if (ctx) redisFree(ctx);
            
            auto result = createContext();
            if (result.isOk()) {
                return result;
            }
            
            continue;
        }
        
        auto result = createContext();
        if (result.isOk()) {
            return result;
        }
        
        LOG_WARN("RedisPool: waiting for available context... ({}/{})", 
                 retry + 1, m_config.retryCount);
        m_condition.wait_for(lock, std::chrono::seconds(1));
    }
    
    return Result<redisContext*>::fail(ErrorCode::InternalError, "Failed to get Redis context");
}

void RedisPool::returnContext(redisContext* ctx) {
    if (!ctx) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_pool.size() < m_config.poolMax && ctx->err == 0) {
        m_pool.push(ctx);
        m_condition.notify_one();
    } else {
        redisFree(ctx);
    }
}

size_t RedisPool::getAvailableCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pool.size();
}

void RedisPool::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_initialized = false;
    
    while (!m_pool.empty()) {
        redisContext* ctx = m_pool.front();
        m_pool.pop();
        if (ctx) {
            redisFree(ctx);
        }
    }
    
    LOG_INFO("RedisPool: shutdown complete");
}

} // namespace data
} // namespace chatroom
