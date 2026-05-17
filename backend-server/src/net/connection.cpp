#include "net/connection.h"
#include "common/logging.h"
#include <cstring>
#include <unistd.h>

namespace chatroom {
namespace net {

Connection::Connection(int fd) 
    : m_fd(fd)
    , m_lastHeartbeat(std::chrono::steady_clock::now()) {
    LOG_DEBUG("Connection: created fd={}", fd);
}

Connection::~Connection() {
    LOG_DEBUG("Connection: destroyed fd={}", m_fd);
}

void Connection::bindUser(uint64_t userId, const std::string& token, uint32_t role) {
    m_userId.store(userId);
    m_token = token;
    m_role.store(role);
    m_state.store(ConnState::AUTHED);
    LOG_INFO("Connection: fd={} bound to user={} role={}", m_fd, userId, role);
}

void Connection::unbindUser() {
    LOG_INFO("Connection: fd={} unbound from user={}", m_fd, m_userId.load());
    m_userId.store(0);
    m_token.clear();
    m_role.store(0);
    m_roomId.store(0);
    m_state.store(ConnState::UNAUTHED);
}

std::string Connection::getToken() const {
    std::lock_guard<std::mutex> lock(m_heartbeatMutex);
    return m_token;
}

void Connection::updateHeartbeatTime() {
    std::lock_guard<std::mutex> lock(m_heartbeatMutex);
    m_lastHeartbeat = std::chrono::steady_clock::now();
}

std::chrono::steady_clock::time_point Connection::getLastHeartbeatTime() const {
    std::lock_guard<std::mutex> lock(m_heartbeatMutex);
    return m_lastHeartbeat;
}

void Connection::setCurrentRoom(uint64_t roomId) {
    m_roomId.store(roomId);
}

void Connection::appendReadBuffer(const char* data, size_t len) {
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    m_readBuffer.append(data, len);
}

std::string& Connection::getReadBuffer() {
    return m_readBuffer;
}

void Connection::clearReadBuffer() {
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    m_readBuffer.clear();
}

size_t Connection::getReadBufferSize() const {
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    return m_readBuffer.size();
}

ConnectionManager& ConnectionManager::getInstance() {
    static ConnectionManager instance;
    return instance;
}

Result<int> ConnectionManager::addConnection(int fd, std::shared_ptr<Connection> conn) {
    std::unique_lock<std::shared_mutex> lock(m_rwLock);
    
    auto result = m_connections.emplace(fd, std::move(conn));
    if (result.second) {
        LOG_INFO("ConnectionManager: added connection fd={}, total={}", fd, m_connections.size());
        lock.unlock();
        
        if (m_onConnect) {
            m_onConnect(fd);
        }
        
        return Result<int>::ok(fd);
    }
    
    return Result<int>::fail(ErrorCode::AlreadyExists, "Connection already exists");
}

Result<void> ConnectionManager::removeConnection(int fd) {
    std::shared_ptr<Connection> conn;
    {
        std::unique_lock<std::shared_mutex> lock(m_rwLock);
        auto it = m_connections.find(fd);
        if (it == m_connections.end()) {
            return VoidResult::fail(ErrorCode::NotFound, "Connection not found");
        }
        
        conn = it->second;
        m_connections.erase(it);
    }
    
    LOG_INFO("ConnectionManager: removed connection fd={}, user={}, remaining={}", 
             fd, conn->getUserId(), m_connections.size());
    
    if (m_onDisconnect) {
        m_onDisconnect(fd);
    }
    
    return VoidResult::ok();
}

std::shared_ptr<Connection> ConnectionManager::getConnection(int fd) {
    std::shared_lock<std::shared_mutex> lock(m_rwLock);
    auto it = m_connections.find(fd);
    if (it != m_connections.end()) {
        return it->second;
    }
    return nullptr;
}

Result<void> ConnectionManager::broadcast(uint64_t roomId, const std::string& message) {
    std::vector<std::shared_ptr<Connection>> targets;
    
    {
        std::shared_lock<std::shared_mutex> lock(m_rwLock);
        for (const auto& [fd, conn] : m_connections) {
            if (conn->getCurrentRoomId() == roomId && conn->getState() != ConnState::KICKED) {
                targets.push_back(conn);
            }
        }
    }
    
    int successCount = 0;
    int failCount = 0;
    
    for (const auto& conn : targets) {
        auto result = sendTo(conn->getFd(), message);
        if (result.isOk()) {
            successCount++;
        } else {
            failCount++;
        }
    }
    
    if (failCount > 0) {
        LOG_WARN("ConnectionManager: broadcast to room={} sent={} failed={}", 
                 roomId, successCount, failCount);
    }
    
    return VoidResult::ok();
}

Result<void> ConnectionManager::sendTo(int fd, const std::string& message) {
    auto conn = getConnection(fd);
    if (!conn) {
        return VoidResult::fail(ErrorCode::NotFound, "Connection not found");
    }
    
    ssize_t written = write(fd, message.data(), message.size());
    if (written < 0) {
        LOG_ERROR("ConnectionManager: send to fd={} failed: errno={}", fd, errno);
        return VoidResult::fail(ErrorCode::NetworkError, "Send failed");
    }
    
    if (static_cast<size_t>(written) != message.size()) {
        LOG_WARN("ConnectionManager: partial send to fd={} wrote/{}={}/{}", 
                 fd, written, message.size());
    }
    
    return VoidResult::ok();
}

size_t ConnectionManager::getActiveConnectionCount() const {
    std::shared_lock<std::shared_mutex> lock(m_rwLock);
    return m_connections.size();
}

void ConnectionManager::setConnectCallback(ConnectCallback cb) {
    m_onConnect = std::move(cb);
}

void ConnectionManager::setDisconnectCallback(DisconnectCallback cb) {
    m_onDisconnect = std::move(cb);
}

} // namespace net
} // namespace chatroom
