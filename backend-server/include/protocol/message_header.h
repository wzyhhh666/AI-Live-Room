#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include "common/result.h"

namespace chatroom {

namespace protocol {

constexpr uint32_t MAGIC_NUMBER = 0x48415443;
constexpr uint16_t PROTOCOL_VERSION = 1;
constexpr size_t HEADER_SIZE = 48;
constexpr size_t MAX_PACKET_SIZE = 1 * 1024 * 1024;

struct MessageHeader {
    uint32_t magic = MAGIC_NUMBER;
    uint16_t version = PROTOCOL_VERSION;
    uint16_t headerLen = HEADER_SIZE;
    uint16_t msgType = 0;
    uint16_t flags = 0;
    uint64_t seq = 0;
    uint64_t roomId = 0;
    uint64_t userId = 0;
    uint64_t timestamp = 0;
    uint32_t bodyLen = 0;

    std::vector<uint8_t> serialize() const;
    
    static bool deserialize(const uint8_t* data, size_t len, MessageHeader& out);
    
    bool isValid() const {
        return magic == MAGIC_NUMBER && 
               version == PROTOCOL_VERSION && 
               headerLen == HEADER_SIZE &&
               bodyLen <= MAX_PACKET_SIZE;
    }
};

enum class MessageType : uint16_t {
    MSG_LOGIN = 1001,
    MSG_LOGIN_RESP = 1002,
    MSG_LOGOUT = 1003,
    MSG_KICK = 1004,
    MSG_DANMAKU = 2001,
    MSG_GIFT = 2002,
    MSG_JOIN_ROOM = 3001,
    MSG_LEAVE_ROOM = 3002,
    MSG_ROOM_CREATE = 3003,
    MSG_ROOM_CLOSE = 3004,
    MSG_ROOM_STATE_SYNC = 3005,
    MSG_HEARTBEAT = 9001,
    MSG_ACK = 9002,
    MSG_ERROR = 5001
};

inline const char* getMessageTypeName(MessageType type) {
    switch (type) {
        case MessageType::MSG_LOGIN:          return "MSG_LOGIN";
        case MessageType::MSG_LOGIN_RESP:     return "MSG_LOGIN_RESP";
        case MessageType::MSG_LOGOUT:         return "MSG_LOGOUT";
        case MessageType::MSG_KICK:           return "MSG_KICK";
        case MessageType::MSG_DANMAKU:        return "MSG_DANMAKU";
        case MessageType::MSG_GIFT:           return "MSG_GIFT";
        case MessageType::MSG_JOIN_ROOM:      return "MSG_JOIN_ROOM";
        case MessageType::MSG_LEAVE_ROOM:     return "MSG_LEAVE_ROOM";
        case MessageType::MSG_ROOM_CREATE:    return "MSG_ROOM_CREATE";
        case MessageType::MSG_ROOM_CLOSE:     return "MSG_ROOM_CLOSE";
        case MessageType::MSG_ROOM_STATE_SYNC: return "MSG_ROOM_STATE_SYNC";
        case MessageType::MSG_HEARTBEAT:      return "MSG_HEARTBEAT";
        case MessageType::MSG_ACK:            return "MSG_ACK";
        case MessageType::MSG_ERROR:          return "MSG_ERROR";
        default:                              return "UNKNOWN";
    }
}

} // namespace protocol
} // namespace chatroom
