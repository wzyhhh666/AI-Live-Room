#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include "connection.h"
#include "protocol.h"

namespace chatroom {
namespace client {

class ChatClient {
public:
    ChatClient();
    ~ChatClient();

    bool connect(const std::string& host, uint16_t port);
    void disconnect();
    
    bool isConnected() const;
    
    bool login(const std::string& username, const std::string& token = "");
    bool logout();
    
    bool sendDanmaku(uint64_t roomId, const std::string& content);
    bool sendGift(uint64_t roomId, int giftId, int count);
    bool joinRoom(uint64_t roomId);
    bool leaveRoom(uint64_t roomId);
    bool createRoom(const std::string& roomName);
    bool closeRoom(uint64_t roomId);
    
    void startHeartbeat(int intervalSec = 10);
    void stopHeartbeat();

private:
    void onMessageReceived(const std::string& data);
    void onErrorOccurred(const std::string& error);
    void heartbeatLoop();
    void handleServerMessage(const protocol::Message& msg);

    std::unique_ptr<TcpConnection> m_connection;
    
    std::atomic<bool> m_loggedIn{false};
    std::atomic<uint64_t> m_userId{0};
    std::atomic<uint64_t> m_currentRoomId{0};
    std::string m_username;
    std::string m_token;
    
    std::thread m_heartbeatThread;
    std::atomic<bool> m_heartbeatRunning{false};
    int m_heartbeatInterval{10};

public:
    static std::string formatLog(const std::string& message);
    static std::string formatLoginResponse(const std::string& body);
};

} // namespace client
} // namespace chatroom
