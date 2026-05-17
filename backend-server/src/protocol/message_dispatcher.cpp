#include "protocol/message_dispatcher.h"
#include "common/logging.h"

namespace chatroom {
namespace protocol {

MessageDispatcher& MessageDispatcher::getInstance() {
    static MessageDispatcher instance;
    return instance;
}

void MessageDispatcher::registerHandler(uint16_t msgType, HandlerFunc handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_handlers.find(msgType);
    if (it != m_handlers.end()) {
        LOG_WARN("MessageDispatcher: overwriting handler for msgType={}", msgType);
    }
    
    m_handlers[msgType] = std::move(handler);
    LOG_DEBUG("MessageDispatcher: registered handler for msgType={}", msgType);
}

Result<void> MessageDispatcher::dispatch(int fd, Message& msg) {
    HandlerFunc handler;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_handlers.find(msg.header.msgType);
        if (it == m_handlers.end()) {
            LOG_WARN("MessageDispatcher: no handler for msgType={} from fd={}", 
                     msg.header.msgType, fd);
            return VoidResult::fail(ErrorCode::ProtocolError, "No handler for message type");
        }
        handler = it->second;
    }
    
    try {
        return handler(fd, msg);
    } catch (const std::exception& e) {
        LOG_ERROR("MessageDispatcher: handler exception for msgType={}: {}", 
                 msg.header.msgType, e.what());
        return VoidResult::fail(ErrorCode::InternalError, e.what());
    } catch (...) {
        LOG_ERROR("MessageDispatcher: unknown exception for msgType={}", msg.header.msgType);
        return VoidResult::fail(ErrorCode::InternalError, "Unknown exception");
    }
}

bool MessageDispatcher::hasHandler(uint16_t msgType) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_handlers.find(msgType) != m_handlers.end();
}

} // namespace protocol
} // namespace chatroom
