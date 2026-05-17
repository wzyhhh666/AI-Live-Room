#include "net/epoll_server.h"
#include "net/connection.h"
#include "protocol/message_header.h"
#include "common/logging.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

namespace chatroom {
namespace net {

EpollServer::EpollServer() : m_events(MAX_EVENTS) {}

EpollServer::~EpollServer() {
    stop();
}

Result<void> EpollServer::init(const std::string& host, uint16_t port, int backlog) {
    m_listenFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (m_listenFd < 0) {
        return VoidResult::fail(ErrorCode::NetworkError, "Failed to create listen socket");
    }
    
    int opt = 1;
    setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = (host == "0.0.0.0") ? INADDR_ANY : inet_addr(host.c_str());
    
    if (bind(m_listenFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(m_listenFd);
        m_listenFd = -1;
        return VoidResult::fail(ErrorCode::NetworkError, "Failed to bind to address");
    }
    
    if (listen(m_listenFd, backlog) < 0) {
        close(m_listenFd);
        m_listenFd = -1;
        return VoidResult::fail(ErrorCode::NetworkError, "Failed to listen on socket");
    }
    
    m_epollFd = epoll_create1(EPOLL_CLOEXEC);
    if (m_epollFd < 0) {
        close(m_listenFd);
        m_listenFd = -1;
        return VoidResult::fail(ErrorCode::InternalError, "Failed to create epoll instance");
    }
    
    struct epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = m_listenFd;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_listenFd, &ev) < 0) {
        close(m_epollFd);
        close(m_listenFd);
        m_epollFd = m_listenFd = -1;
        return VoidResult::fail(ErrorCode::InternalError, "Failed to add listen fd to epoll");
    }
    
    LOG_INFO("EpollServer: initialized on {}:{}", host, port);
    LOG_INFO("EpollServer: listen_fd={}, epoll_fd={}, backlog={}", m_listenFd, m_epollFd, backlog);
    
    return VoidResult::ok();
}

Result<void> EpollServer::start() {
    if (m_running.load()) {
        return VoidResult::fail(ErrorCode::InvalidArgument, "Server is already running");
    }
    
    if (m_listenFd < 0 || m_epollFd < 0) {
        return VoidResult::fail(ErrorCode::InternalError, "Server not initialized");
    }
    
    m_running.store(true);
    LOG_INFO("EpollServer: starting event loop...");
    
    try {
        eventLoop();
    } catch (const std::exception& e) {
        LOG_ERROR("EpollServer: event loop exception: {}", e.what());
        m_running.store(false);
        return VoidResult::fail(ErrorCode::InternalError, e.what());
    }
    
    m_running.store(false);
    LOG_INFO("EpollServer: event loop stopped");
    return VoidResult::ok();
}

void EpollServer::stop() {
    if (!m_running.exchange(false)) {
        return;
    }
    
    LOG_INFO("EpollServer: stopping...");
    
    if (m_listenFd >= 0) {
        shutdown(m_listenFd, SHUT_RDWR);
        close(m_listenFd);
        m_listenFd = -1;
    }
    
    if (m_epollFd >= 0) {
        close(m_epollFd);
        m_epollFd = -1;
    }
}

void EpollServer::eventLoop() {
    while (m_running.load()) {
        int nfds = epoll_wait(m_epollFd, m_events.data(), MAX_EVENTS, 100);
        
        if (nfds < 0) {
            if (errno == EINTR) continue;
            
            LOG_ERROR("EpollServer: epoll_wait error: {}", strerror(errno));
            break;
        }
        
        for (int i = 0; i < nfds; ++i) {
            int fd = m_events[i].data.fd;
            uint32_t events = m_events[i].events;
            
            if (fd == m_listenFd) {
                handleAccept();
                continue;
            }
            
            if (events & (EPOLLERR | EPOLLHUP)) {
                handleError(fd);
                continue;
            }
            
            if (events & EPOLLIN) {
                handleRead(fd);
            }
        }
    }
}

void EpollServer::handleAccept() {
    while (true) {
        struct sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        
        int clientFd = accept4(m_listenFd, 
                                reinterpret_cast<struct sockaddr*>(&clientAddr),
                                &addrLen,
                                SOCK_NONBLOCK | SOCK_CLOEXEC);
        
        if (clientFd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            
            LOG_ERROR("EpollServer: accept error: {}", strerror(errno));
            return;
        }
        
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
        uint16_t clientPort = ntohs(clientAddr.sin_port);
        
        auto conn = std::make_shared<Connection>(clientFd);
        
        auto& connMgr = ConnectionManager::getInstance();
        auto result = connMgr.addConnection(clientFd, conn);
        
        if (!result.isOk()) {
            LOG_WARN("EpollServer: failed to add connection fd={}, closing", clientFd);
            close(clientFd);
            continue;
        }
        
        struct epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = clientFd;
        
        if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, clientFd, &ev) < 0) {
            LOG_ERROR("EpollServer: failed to add client fd={} to epoll: {}", 
                     clientFd, strerror(errno));
            connMgr.removeConnection(clientFd);
            close(clientFd);
            continue;
        }
        
        LOG_INFO("EpollServer: new connection fd={} from {}:{}, total connections={}", 
                 clientFd, clientIP, clientPort, connMgr.getActiveConnectionCount());
    }
}

void EpollServer::handleRead(int fd) {
    auto& connMgr = ConnectionManager::getInstance();
    auto conn = connMgr.getConnection(fd);
    
    if (!conn) {
        LOG_WARN("EpollServer: read for unknown fd={}", fd);
        epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
        return;
    }
    
    constexpr size_t BUFFER_SIZE = 65536;
    char buffer[BUFFER_SIZE];
    
    while (true) {
        ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
        
        if (bytesRead > 0) {
            conn->appendReadBuffer(buffer, static_cast<size_t>(bytesRead));
            LOG_DEBUG("EpollServer: read {} bytes from fd={}", bytesRead, fd);
            continue;
        }
        
        if (bytesRead == 0) {
            LOG_INFO("EpollServer: connection closed by peer fd={}", fd);
            handleError(fd);
            return;
        }
        
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
        }
        
        if (errno == EINTR) {
            continue;
        }
        
        LOG_ERROR("EpollServer: read error on fd={}: {}", fd, strerror(errno));
        handleError(fd);
        return;
    }
    
    if (conn->getReadBufferSize() > 0) {
        LOG_DEBUG("EpollServer: fd={} has {} bytes in buffer, ready for processing", 
                 fd, conn->getReadBufferSize());
    }
}

void EpollServer::handleError(int fd) {
    LOG_WARN("EpollServer: handling error/disconnect for fd={}", fd);
    
    epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr);
    
    auto& connMgr = ConnectionManager::getInstance();
    connMgr.removeConnection(fd);
    
    close(fd);
}

} // namespace net
} // namespace chatroom
