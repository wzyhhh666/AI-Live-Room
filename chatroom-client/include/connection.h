#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>
#include <queue>
#include <condition_variable>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#endif

namespace chatroom {
namespace client {

class TcpConnection {
public:
    using MessageCallback = std::function<void(const std::string& data)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    TcpConnection();
    ~TcpConnection();

    bool connect(const std::string& host, uint16_t port);
    void disconnect();
    
    bool isConnected() const { return m_connected.load(); }
    
    bool send(const std::string& data);
    
    void setMessageCallback(MessageCallback cb);
    void setErrorCallback(ErrorCallback cb);
    
    void startRecvLoop();
    void stopRecvLoop();

private:
    void recvLoop();
    
    SOCKET m_socket;
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_running{false};
    
    MessageCallback m_messageCallback;
    ErrorCallback m_errorCallback;
    
    std::thread m_recvThread;
    mutable std::mutex m_mutex;

public:
    static bool initWinsock();
    static void cleanupWinsock();
};

} // namespace client
} // namespace chatroom
