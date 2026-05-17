#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <functional>
#include <atomic>
#include <string>
#include <chrono>
#include "common/error_code.h"
#include "common/callbacks.h"
#include "protocol/message_header.h"

namespace chatroom {
namespace net{

class Connection; 

enum class ConnState : uint8_t {
    UNAUTHED = 0,
    AUTHED = 1,
    KICKED = 2
};

class Connection {
public:
    explicit Connection(int fd);
    ~Connection();
    
    int getFd() const { return m_fd; }
    ConnState getState() const { return m_state.load(); }
    void setState(ConnState state) { m_state.store(state); }
    
    void bindUser(uint64_t userId, const std::string& token, uint32_t role);
    void unbindUser();
    uint64_t getUserId() const { return m_userId.load(); }
    std::string getToken() const;
    uint32_t getUserRole() const { return m_role.load(); }
    
    void updateHeartbeatTime();
    std::chrono::steady_clock::time_point getLastHeartbeatTime() const;
    
    void setCurrentRoom(uint64_t roomId);
    uint64_t getCurrentRoomId() const { return m_roomId.load(); }
    
    void appendReadBuffer(const char* data, size_t len);
    std::string& getReadBuffer();
    void clearReadBuffer();
    size_t getReadBufferSize() const;
    
private:
    int m_fd;
    std::atomic<ConnState> m_state{ConnState::UNAUTHED};
    
    std::atomic<uint64_t> m_userId{0};
    std::string m_token;
    std::atomic<uint32_t> m_role{0};
    
    std::atomic<uint64_t> m_roomId{0};
    
    std::chrono::steady_clock::time_point m_lastHeartbeat;
    mutable std::mutex m_heartbeatMutex;
    
    std::string m_readBuffer;
    mutable std::mutex m_bufferMutex;
};

class ConnectionManager {
public:
    static ConnectionManager& getInstance();
    
    Result<int> addConnection(int fd, std::shared_ptr<Connection> conn);
    Result<void> removeConnection(int fd);
    std::shared_ptr<Connection> getConnection(int fd);
    
    Result<void> broadcast(uint64_t roomId, const std::string& message);
    Result<void> sendTo(int fd, const std::string& message);
    
    size_t getActiveConnectionCount() const;
    
    void setConnectCallback(ConnectCallback cb);
    void setDisconnectCallback(DisconnectCallback cb);

private:
    ConnectionManager() = default;
    
    std::unordered_map<int, std::shared_ptr<Connection>> m_connections;
    mutable std::shared_mutex m_rwLock;
    
    ConnectCallback m_onConnect;
    DisconnectCallback m_onDisconnect;
};

} // namespace net
} // namespace chatroom
