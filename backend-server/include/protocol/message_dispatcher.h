#pragma once

#include <unordered_map>
#include <functional>
#include <mutex>
#include "protocol/message_codec.h"
#include "common/result.h"

namespace chatroom {
namespace protocol{

class MessageDispatcher {
public:
    using HandlerFunc = std::function<Result<void>(int fd, Message& msg)>;
    
    static MessageDispatcher& getInstance();
    
    void registerHandler(uint16_t msgType, HandlerFunc handler);
    Result<void> dispatch(int fd, Message& msg);
    
    bool hasHandler(uint16_t msgType) const;

private:
    MessageDispatcher() = default;
    
    std::unordered_map<uint16_t, HandlerFunc> m_handlers;
    mutable std::mutex m_mutex;
};

} // namespace protocol
} // namespace chatroom
