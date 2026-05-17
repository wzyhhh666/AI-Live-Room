#include "protocol/message_header.h"
#include <cstring>
#include <arpa/inet.h>

namespace chatroom {
namespace protocol {

std::vector<uint8_t> MessageHeader::serialize() const {
    std::vector<uint8_t> data(HEADER_SIZE);
    
    uint32_t magicNet = htonl(magic);
    std::memcpy(data.data(), &magicNet, sizeof(magicNet));
    
    uint16_t versionNet = htons(version);
    std::memcpy(data.data() + 4, &versionNet, sizeof(versionNet));
    
    uint16_t headerLenNet = htons(headerLen);
    std::memcpy(data.data() + 6, &headerLenNet, sizeof(headerLenNet));
    
    uint16_t msgTypeNet = htons(msgType);
    std::memcpy(data.data() + 8, &msgTypeNet, sizeof(msgTypeNet));
    
    uint16_t flagsNet = htons(flags);
    std::memcpy(data.data() + 10, &flagsNet, sizeof(flagsNet));
    
    uint64_t seqNet = htobe64(seq);
    std::memcpy(data.data() + 12, &seqNet, sizeof(seqNet));
    
    uint64_t roomIdNet = htobe64(roomId);
    std::memcpy(data.data() + 20, &roomIdNet, sizeof(roomIdNet));
    
    uint64_t userIdNet = htobe64(userId);
    std::memcpy(data.data() + 28, &userIdNet, sizeof(userIdNet));
    
    uint64_t timestampNet = htobe64(timestamp);
    std::memcpy(data.data() + 36, &timestampNet, sizeof(timestampNet));
    
    uint32_t bodyLenNet = htonl(bodyLen);
    std::memcpy(data.data() + 44, &bodyLenNet, sizeof(bodyLenNet));
    
    return data;
}

bool MessageHeader::deserialize(const uint8_t* data, size_t len, MessageHeader& out) {
    if (len < HEADER_SIZE) {
        return false;
    }
    
    std::memcpy(&out.magic, data, sizeof(out.magic));
    out.magic = ntohl(out.magic);
    
    std::memcpy(&out.version, data + 4, sizeof(out.version));
    out.version = ntohs(out.version);
    
    std::memcpy(&out.headerLen, data + 6, sizeof(out.headerLen));
    out.headerLen = ntohs(out.headerLen);
    
    std::memcpy(&out.msgType, data + 8, sizeof(out.msgType));
    out.msgType = ntohs(out.msgType);
    
    std::memcpy(&out.flags, data + 10, sizeof(out.flags));
    out.flags = ntohs(out.flags);
    
    std::memcpy(&out.seq, data + 12, sizeof(out.seq));
    out.seq = be64toh(out.seq);
    
    std::memcpy(&out.roomId, data + 20, sizeof(out.roomId));
    out.roomId = be64toh(out.roomId);
    
    std::memcpy(&out.userId, data + 28, sizeof(out.userId));
    out.userId = be64toh(out.userId);
    
    std::memcpy(&out.timestamp, data + 36, sizeof(out.timestamp));
    out.timestamp = be64toh(out.timestamp);
    
    std::memcpy(&out.bodyLen, data + 44, sizeof(out.bodyLen));
    out.bodyLen = ntohl(out.bodyLen);
    
    return true;
}

} // namespace protocol
} // namespace chatroom
