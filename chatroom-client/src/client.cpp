#include "client.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace chatroom {
namespace client {

ChatClient::ChatClient()
    : m_connection(std::make_unique<TcpConnection>()) {
    
    m_connection->setMessageCallback(
        [this](const std::string& data) {
            onMessageReceived(data);
        }
    );

    m_connection->setErrorCallback(
        [this](const std::string& error) {
            onErrorOccurred(error);
        }
    );
}

ChatClient::~ChatClient() {
    disconnect();
}

bool ChatClient::connect(const std::string& host, uint16_t port) {
    std::cout << formatLog("Initializing connection to " + host + ":" + std::to_string(port)) << std::endl;
    return m_connection->connect(host, port);
}

void ChatClient::disconnect() {
    stopHeartbeat();
    
    if (m_loggedIn.load()) {
        logout();
    }

    m_connection->disconnect();
}

bool ChatClient::isConnected() const {
    return m_connection->isConnected();
}

bool ChatClient::login(const std::string& username, const std::string& token) {
    if (!isConnected()) {
        std::cerr << formatLog("Cannot login: not connected") << std::endl;
        return false;
    }

    m_username = username;
    m_token = token.empty() ? "test_token_" + username : token;

    std::string loginBody = R"({"username":")" + username + R"(","token":")" + m_token + R"("})";

    auto result = protocol::ClientProtocol::encodeMessage(
        protocol::MessageType::MSG_LOGIN,
        0,
        0,
        loginBody
    );

    if (!result.isOk()) {
        std::cerr << formatLog("Failed to encode login message: " + result.message()) << std::endl;
        return false;
    }

    bool sent = m_connection->send(result.value());
    if (sent) {
        std::cout << formatLog("Login request sent for user: " + username) << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        m_userId.store(1);
        m_loggedIn.store(true);
        
        startHeartbeat(m_heartbeatInterval);
    }

    return sent;
}

bool ChatClient::logout() {
    if (!m_loggedIn.load()) return true;

    auto result = protocol::ClientProtocol::encodeMessage(
        protocol::MessageType::MSG_LOGOUT,
        m_userId.load(),
        0,
        ""
    );

    if (result.isOk()) {
        m_connection->send(result.value());
    }

    stopHeartbeat();
    m_loggedIn.store(false);
    m_userId.store(0);
    m_currentRoomId.store(0);

    std::cout << formatLog("Logged out successfully") << std::endl;
    return true;
}

bool ChatClient::sendDanmaku(uint64_t roomId, const std::string& content) {
    if (!m_loggedIn.load()) {
        std::cerr << formatLog("Cannot send danmaku: not logged in") << std::endl;
        return false;
    }

    std::string body = R"({"room_id":)" + std::to_string(roomId) +
                       R"(,"user_id":)" + std::to_string(m_userId.load()) +
                       R"(,"username":")" + m_username +
                       R"(","content":")" + content + R"("})";

    auto result = protocol::ClientProtocol::encodeMessage(
        protocol::MessageType::MSG_DANMAKU,
        m_userId.load(),
        roomId,
        body
    );

    if (!result.isOk()) {
        std::cerr << formatLog("Failed to encode danmaku message") << std::endl;
        return false;
    }

    bool sent = m_connection->send(result.value());
    if (sent) {
        std::cout << formatLog("Danmaku sent: " + content) << std::endl;
    }

    return sent;
}

bool ChatClient::sendGift(uint64_t roomId, int giftId, int count) {
    if (!m_loggedIn.load()) {
        std::cerr << formatLog("Cannot send gift: not logged in") << std::endl;
        return false;
    }

    std::string body = R"({"room_id":)" + std::to_string(roomId) +
                       R"(,"gift_id":)" + std::to_string(giftId) +
                       R"(,"count":)" + std::to_string(count) + R"(})";

    auto result = protocol::ClientProtocol::encodeMessage(
        protocol::MessageType::MSG_GIFT,
        m_userId.load(),
        roomId,
        body
    );

    if (!result.isOk()) return false;

    bool sent = m_connection->send(result.value());
    if (sent) {
        std::cout << formatLog("Gift sent: gift_id=" + std::to_string(giftId) + 
                     " count=" + std::to_string(count)) << std::endl;
    }

    return sent;
}

bool ChatClient::joinRoom(uint64_t roomId) {
    if (!m_loggedIn.load()) {
        std::cerr << formatLog("Cannot join room: not logged in") << std::endl;
        return false;
    }

    auto result = protocol::ClientProtocol::encodeMessage(
        protocol::MessageType::MSG_JOIN_ROOM,
        m_userId.load(),
        roomId,
        ""
    );

    if (!result.isOk()) return false;

    bool sent = m_connection->send(result.value());
    if (sent) {
        m_currentRoomId.store(roomId);
        std::cout << formatLog("Joined room: " + std::to_string(roomId)) << std::endl;
    }

    return sent;
}

bool ChatClient::leaveRoom(uint64_t roomId) {
    auto result = protocol::ClientProtocol::encodeMessage(
        protocol::MessageType::MSG_LEAVE_ROOM,
        m_userId.load(),
        roomId,
        ""
    );

    if (!result.isOk()) return false;

    bool sent = m_connection->send(result.value());
    if (sent) {
        if (m_currentRoomId.load() == roomId) {
            m_currentRoomId.store(0);
        }
        std::cout << formatLog("Left room: " + std::to_string(roomId)) << std::endl;
    }

    return sent;
}

bool ChatClient::createRoom(const std::string& roomName) {
    if (!m_loggedIn.load()) {
        std::cerr << formatLog("Cannot create room: not logged in") << std::endl;
        return false;
    }

    std::string body = R"({"room_name":")" + roomName + R"(","owner_id":)" + 
                      std::to_string(m_userId.load()) + R"(})";

    auto result = protocol::ClientProtocol::encodeMessage(
        protocol::MessageType::MSG_ROOM_CREATE,
        m_userId.load(),
        0,
        body
    );

    if (!result.isOk()) return false;

    bool sent = m_connection->send(result.value());
    if (sent) {
        std::cout << formatLog("Room creation requested: " + roomName) << std::endl;
    }

    return sent;
}

bool ChatClient::closeRoom(uint64_t roomId) {
    auto result = protocol::ClientProtocol::encodeMessage(
        protocol::MessageType::MSG_ROOM_CLOSE,
        m_userId.load(),
        roomId,
        ""
    );

    if (!result.isOk()) return false;

    bool sent = m_connection->send(result.value());
    if (sent) {
        std::cout << formatLog("Close room requested: " + std::to_string(roomId)) << std::endl;
    }

    return sent;
}

void ChatClient::startHeartbeat(int intervalSec) {
    stopHeartbeat();

    m_heartbeatInterval = intervalSec;
    m_heartbeatRunning.store(true);

    m_heartbeatThread = std::thread(&ChatClient::heartbeatLoop, this);
    std::cout << formatLog("Heartbeat started (interval=" + std::to_string(intervalSec) + "s)") << std::endl;
}

void ChatClient::stopHeartbeat() {
    if (!m_heartbeatRunning.load()) return;

    m_heartbeatRunning.store(false);

    if (m_heartbeatThread.joinable()) {
        m_heartbeatThread.join();
    }

    std::cout << formatLog("Heartbeat stopped") << std::endl;
}

void ChatClient::onMessageReceived(const std::string& data) {
    auto decodeResult = protocol::ClientProtocol::decodeMessage(data);
    if (!decodeResult.isOk()) {
        std::cerr << formatLog("Failed to decode server message") << std::endl;
        return;
    }

    handleServerMessage(decodeResult.value());
}

void ChatClient::onErrorOccurred(const std::string& error) {
    std::cerr << formatLog("Connection error: " + error) << std::endl;
    m_loggedIn.store(false);
}

void ChatClient::handleServerMessage(const protocol::Message& msg) {
    auto msgType = static_cast<protocol::MessageType>(msg.header.msgType);

    std::cout << "\n[Server Message] Type=" << protocol::getMessageTypeName(msgType)
              << " Seq=" << msg.header.seq
              << " RoomID=" << msg.header.roomId
              << " UserID=" << msg.header.userId
              << " BodyLen=" << msg.body.size();

    if (!msg.body.empty()) {
        std::cout << " Body=\"" << msg.body.substr(0, std::min(size_t(50), msg.body.size())) << "\"";
    }
    std::cout << std::endl;

    switch (msgType) {
        case protocol::MessageType::MSG_LOGIN_RESP:
            std::cout << formatLoginResponse(msg.body);
            break;

        case protocol::MessageType::MSG_KICK:
            std::cout << formatLog("⚠️  Kicked by server!") << std::endl;
            m_loggedIn.store(false);
            break;

        case protocol::MessageType::MSG_ACK:
            std::cout << formatLog("✓ ACK received for seq=" + std::to_string(msg.header.seq)) << std::endl;
            break;

        case protocol::MessageType::MSG_ERROR:
            std::cout << formatLog("✗ Error from server: " + msg.body) << std::endl;
            break;

        case protocol::MessageType::MSG_DANMAKU:
            std::cout << formatLog("📨 Received danmaku in room " + 
                         std::to_string(msg.header.roomId) + ": " + msg.body) << std::endl;
            break;

        case protocol::MessageType::MSG_ROOM_STATE_SYNC:
            std::cout << formatLog("🏠 Room state sync received") << std::endl;
            break;

        default:
            std::cout << formatLog("Unknown message type received") << std::endl;
            break;
    }
}

void ChatClient::heartbeatLoop() {
    while (m_heartbeatRunning.load() && isConnected()) {
        std::this_thread::sleep_for(std::chrono::seconds(m_heartbeatInterval));

        if (!m_heartbeatRunning.load() || !isConnected()) break;

        auto result = protocol::ClientProtocol::encodeMessage(
            protocol::MessageType::MSG_HEARTBEAT,
            m_userId.load(),
            m_currentRoomId.load(),
            ""
        );

        if (result.isOk()) {
            m_connection->send(result.value());
        }
    }
}

std::string ChatClient::formatLog(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&timeT), "%H:%M:%S") << "] " << message;
    return ss.str();
}

std::string ChatClient::formatLoginResponse(const std::string& body) {
    if (body.empty()) {
        return formatLog("✓ Login response received (empty body)");
    }

    std::string status = "✓ Login successful!";
    if (body.find("\"success\":false") != std::string::npos ||
        body.find("\"code\"") != std::string::npos) {
        status = "✗ Login failed!";
    }

    return formatLog(status + " Response: " + body.substr(0, 100));
}

} // namespace client
} // namespace chatroom
