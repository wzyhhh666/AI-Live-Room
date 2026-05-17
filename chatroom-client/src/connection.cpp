#include "connection.h"
#include "protocol.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

namespace chatroom {
namespace client {

bool TcpConnection::initWinsock() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "[Client] WSAStartup failed: " << result << std::endl;
        return false;
    }
    return true;
#else
    return true;
#endif
}

void TcpConnection::cleanupWinsock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

TcpConnection::TcpConnection()
    : m_socket(INVALID_SOCKET) {
}

TcpConnection::~TcpConnection() {
    disconnect();
}

bool TcpConnection::connect(const std::string& host, uint16_t port) {
#ifdef _WIN32
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
#endif

    if (m_socket == INVALID_SOCKET) {
        std::cerr << "[Client] Failed to create socket" << std::endl;
        return false;
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

#ifdef _WIN32
    InetPtonA(AF_INET, host.c_str(), &serverAddr.sin_addr.s_addr);
#else
    inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);
#endif

    std::cout << "[Client] Connecting to " << host << ":" << port << "..." << std::endl;

#ifdef _WIN32
    int result = ::connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr));
#else
    int result = ::connect(m_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
#endif

    if (result == SOCKET_ERROR) {
        std::cerr << "[Client] Connection failed" << std::endl;
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET;
        return false;
    }

    m_connected.store(true);
    std::cout << "[Client] Connected to server successfully!" << std::endl;
    
    startRecvLoop();
    return true;
}

void TcpConnection::disconnect() {
    stopRecvLoop();

    if (m_socket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET;
    }
    
    m_connected.store(false);
    std::cout << "[Client] Disconnected from server" << std::endl;
}

bool TcpConnection::send(const std::string& data) {
    if (!m_connected.load()) {
        std::cerr << "[Client] Send failed: not connected" << std::endl;
        return false;
    }

    size_t totalSent = 0;
    while (totalSent < data.size()) {
#ifdef _WIN
        int sent = ::send(m_socket, data.data() + totalSent, 
                         static_cast<int>(data.size() - totalSent), 0);
#else
        ssize_t sent = ::send(m_socket, data.data() + totalSent, 
                              data.size() - totalSent, 0);
#endif

        if (sent <= 0) {
            std::cerr << "[Client] Send failed or connection lost" << std::endl;
            m_connected.store(false);
            return false;
        }

        totalSent += sent;
    }

    return true;
}

void TcpConnection::setMessageCallback(MessageCallback cb) {
    m_messageCallback = std::move(cb);
}

void TcpConnection::setErrorCallback(ErrorCallback cb) {
    m_errorCallback = std::move(cb);
}

void TcpConnection::startRecvLoop() {
    if (m_running.load()) return;

    m_running.store(true);
    m_recvThread = std::thread(&TcpConnection::recvLoop, this);
}

void TcpConnection::stopRecvLoop() {
    if (!m_running.load()) return;

    m_running.store(false);
    
    if (m_recvThread.joinable()) {
        m_recvThread.join();
    }
}

void TcpConnection::recvLoop() {
    std::string buffer;
    const size_t HEADER_SIZE_WITH_LENGTH = protocol::LENGTH_FIELD_SIZE + protocol::HEADER_SIZE;

    while (m_running.load() && m_connected.load()) {
        char temp[4096];
        
#ifdef _WIN32
        int received = recv(m_socket, temp, sizeof(temp), 0);
#else
        ssize_t received = recv(m_socket, temp, sizeof(temp), 0);
#endif

        if (received <= 0) {
            if (m_errorCallback) {
                m_errorCallback("Connection lost or receive error");
            }
            m_connected.store(false);
            break;
        }

        buffer.append(temp, received);

        while (buffer.size() >= HEADER_SIZE_WITH_LENGTH) {
            uint32_t packetLen = 0;
            memcpy(&packetLen, buffer.data(), sizeof(packetLen));
            
#ifdef _WIN32
            packetLen = ntohl(packetLen);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
            packetLen = __builtin_bswap32(packetLen);
#endif

            size_t totalSize = protocol::LENGTH_FIELD_SIZE + packetLen;
            
            if (buffer.size() < totalSize) break;

            std::string packet = buffer.substr(0, totalSize);
            buffer.erase(0, totalSize);

            if (m_messageCallback) {
                m_messageCallback(packet);
            }
        }
    }
}

} // namespace client
} // namespace chatroom
