#include "data/mysql_pool.h"
#include "common/logging.h"
#include <thread>

namespace chatroom {
namespace data {

MySqlConnection::MySqlConnection(MYSQL* conn) : m_conn(conn) {
    m_lastUsedTime = std::chrono::steady_clock::now();
}

MySqlConnection::~MySqlConnection() {
    if (m_conn) {
        mysql_close(m_conn);
    }
}

bool MySqlConnection::isAlive() {
    if (!m_conn) return false;
    
    if (mysql_ping(m_conn) != 0) {
        LOG_WARN("MySQL connection ping failed: {}", mysql_error(m_conn));
        return false;
    }
    
    m_lastUsedTime = std::chrono::steady_clock::now();
    return true;
}

MySqlPool& MySqlPool::getInstance() {
    static MySqlPool instance;
    return instance;
}

Result<void> MySqlPool::initialize(const MySqlConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_config = config;
    
    for (size_t i = 0; i < config.poolMin; ++i) {
        auto result = createConnection();
        if (!result.isOk()) {
            LOG_ERROR("MySQLPool: failed to create initial connection {}: {}", 
                     i, result.message());
            continue;
        }
        
        m_pool.push(result.value());
    }
    
    m_initialized = true;
    LOG_INFO("MySQLPool: initialized with {} connections, host={}:{}", 
             m_pool.size(), config.host, config.port);
    
    return VoidResult::ok();
}

Result<MYSQL*> MySqlPool::createConnection() {
    MYSQL* conn = mysql_init(nullptr);
    if (!conn) {
        return Result<MYSQL*>::fail(ErrorCode::DatabaseError, "mysql_init failed");
    }
    
    unsigned int timeout = m_config.connectTimeoutSec;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    
    MYSQL* result = mysql_real_connect(
        conn,
        m_config.host.c_str(),
        m_config.user.c_str(),
        m_config.password.c_str(),
        m_config.database.c_str(),
        m_config.port,
        nullptr,
        CLIENT_MULTI_STATEMENTS
    );
    
    if (!result) {
        const char* error = mysql_error(conn);
        mysql_close(conn);
        return Result<MYSQL*>::fail(ErrorCode::DatabaseError, error ? error : "Connection failed");
    }
    
    unsigned int reconnect = 1;
    mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);
    
    LOG_DEBUG("MySQLPool: created new connection {}", reinterpret_cast<uintptr_t>(conn));
    return Result<MYSQL*>::ok(conn);
}

bool MySqlPool::checkConnection(MYSQL* conn) {
    if (!conn) return false;
    return mysql_ping(conn) == 0;
}

Result<MYSQL*> MySqlPool::getConnection() {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    while (true) {
        if (!m_pool.empty()) {
            MYSQL* conn = m_pool.front();
            m_pool.pop();
            
            if (checkConnection(conn)) {
                return Result<MYSQL*>::ok(conn);
            }
            
            mysql_close(conn);
            continue;
        }
        
        auto result = createConnection();
        if (result.isOk()) {
            return result;
        }
        
        LOG_WARN("MySQLPool: waiting for available connection...");
        m_condition.wait_for(lock, std::chrono::seconds(1));
        
        if (!m_initialized) {
            return Result<MYSQL*>::fail(ErrorCode::InternalError, "Pool shutting down");
        }
    }
}

void MySqlPool::returnConnection(MYSQL* conn) {
    if (!conn) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_pool.size() < m_config.poolMax && checkConnection(conn)) {
        m_pool.push(conn);
        m_condition.notify_one();
    } else {
        mysql_close(conn);
    }
}

size_t MySqlPool::getAvailableCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pool.size();
}

void MySqlPool::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_initialized = false;
    
    while (!m_pool.empty()) {
        MYSQL* conn = m_pool.front();
        m_pool.pop();
        if (conn) {
            mysql_close(conn);
        }
    }
    
    LOG_INFO("MySQLPool: shutdown complete");
}

} // namespace data
} // namespace chatroom
