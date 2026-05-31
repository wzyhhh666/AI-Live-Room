#pragma once

#include <sys/epoll.h>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>
#include "common/result.h"

namespace chatroom {
namespace protocol {
class MessageDispatcher;
}

namespace net{

class EpollServer {
public:
    EpollServer();
    ~EpollServer();
    
    EpollServer(const EpollServer&) = delete;
    EpollServer& operator=(const EpollServer&) = delete;
    
    Result<void> init(const std::string& host, uint16_t port, int backlog = 1024);
    Result<void> start();
    void stop();

    void setDispatcher(std::shared_ptr<protocol::MessageDispatcher> dispatcher);
    
    bool isRunning() const { return m_running.load(); }

private:
    void eventLoop();
    void handleAccept();
    void handleRead(int fd);
    void handleError(int fd);
    
    int m_epollFd = -1;
    int m_listenFd = -1;
    std::atomic<bool> m_running{false};
    std::vector<struct epoll_event> m_events;
    std::shared_ptr<protocol::MessageDispatcher> m_dispatcher;
    
    constexpr static int MAX_EVENTS = 1024;
};

} // namespace net
} // namespace chatroom
