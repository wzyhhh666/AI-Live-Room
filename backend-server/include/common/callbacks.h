#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace chatroom {

struct MessageHeader;

using ConnectCallback = std::function<void(int fd)>;
using DisconnectCallback = std::function<void(int fd)>;
using MessageCallback = std::function<void(int fd, uint16_t msgType,
                                           uint64_t roomId, uint64_t userId,
                                           const std::string& data)>;
using AuthCallback = std::function<void(int fd, uint64_t userId, uint32_t role)>;
using KickCallback = std::function<void(int fd, uint64_t operatorId, const std::string& reason)>;
using RoomStateCallback = std::function<void(uint64_t roomId, int state)>;
using ErrorCallback = std::function<void(int errorCode, const std::string& message)>;

} // namespace chatroom
