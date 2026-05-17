#include "protocol/message_codec.h"
#include "common/logging.h"
#include <cstring>
#include <arpa/inet.h>

namespace chatroom {
namespace protocol {

constexpr size_t LENGTH_FIELD_SIZE = 4;

uint32_t MessageCodec::hostToNetwork32(uint32_t val) {
    return htonl(val);
}

uint16_t MessageCodec::hostToNetwork16(uint16_t val) {
    return htons(val);
}

uint64_t MessageCodec::hostToNetwork64(uint64_t val) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return (((uint64_t)htonl(val & 0xFFFFFFFFUL)) << 32) | htonl((val >> 32) & 0xFFFFFFFFUL);
#else
    return val;
#endif
}

uint32_t MessageCodec::networkToHost32(uint32_t val) {
    return ntohl(val);
}

uint16_t MessageCodec::networkToHost16(uint16_t val) {
    return ntohs(val);
}

uint64_t MessageCodec::networkToHost64(uint64_t val) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return (((uint64_t)ntohl(val & 0xFFFFFFFFUL)) << 32) | ntohl((val >> 32) & 0xFFFFFFFFUL);
#else
    return val;
#endif
}

Result<std::string> MessageCodec::encode(const MessageHeader& header, 
                                          const std::string& protobufBody) {
    if (!header.isValid()) {
        return Result<std::string>::fail(ErrorCode::InvalidArgument, "Invalid header");
    }
    
    if (protobufBody.size() > MAX_PACKET_SIZE - HEADER_SIZE) {
        return Result<std::string>::fail(ErrorCode::InvalidArgument, "Body too large");
    }
    
    MessageHeader headerCopy = header;
    headerCopy.bodyLen = protobufBody.size();
    
    auto headerData = headerCopy.serialize();
    
    uint32_t packetLen = static_cast<uint32_t>(HEADER_SIZE + headerCopy.bodyLen);
    uint32_t networkLen = hostToNetwork32(packetLen);
    
    std::string packet;
    packet.reserve(LENGTH_FIELD_SIZE + packetLen);
    
    packet.append(reinterpret_cast<const char*>(&networkLen), sizeof(networkLen));
    packet.append(reinterpret_cast<const char*>(headerData.data()), headerData.size());
    packet.append(protobufBody);
    
    LOG_DEBUG("MessageCodec: encoded packet, type={}, len={}", 
              headerCopy.msgType, packet.size());
    
    return Result<std::string>::ok(std::move(packet));
}

Result<Message> MessageCodec::decode(const std::string& packetData) {
    if (packetData.size() < LENGTH_FIELD_SIZE) {
        return Result<Message>::fail(ErrorCode::ProtocolError, "Packet too short");
    }
    
    uint32_t networkLen = 0;
    std::memcpy(&networkLen, packetData.data(), sizeof(networkLen));
    uint32_t declaredLen = networkToHost32(networkLen);
    
    size_t actualContentLen = packetData.size() - LENGTH_FIELD_SIZE;
    
    if (actualContentLen < HEADER_SIZE) {
        return Result<Message>::fail(ErrorCode::ProtocolError, "Packet too short for header");
    }
    
    auto headerResult = decodeHeaderOnly(packetData);
    if (!headerResult.isOk()) {
        return Result<Message>::fail(headerResult.code(), headerResult.message());
    }
    
    MessageHeader& header = headerResult.value();
    
    size_t expectedTotal = LENGTH_FIELD_SIZE + HEADER_SIZE + header.bodyLen;
    if (packetData.size() < expectedTotal) {
        return Result<Message>::fail(ErrorCode::ProtocolError, "Incomplete body");
    }
    
    Message msg;
    msg.header = std::move(header);
    msg.body = packetData.substr(LENGTH_FIELD_SIZE + HEADER_SIZE, msg.header.bodyLen);
    
    LOG_DEBUG("MessageCodec: decoded packet, type={}, body_len={}", 
              msg.header.msgType, msg.body.size());
    
    return Result<Message>::ok(std::move(msg));
}

Result<MessageHeader> MessageCodec::decodeHeaderOnly(const std::string& packetData) {
    const uint8_t* data = nullptr;
    size_t dataLen = 0;
    
    if (packetData.size() >= LENGTH_FIELD_SIZE + HEADER_SIZE) {
        uint32_t networkLen = 0;
        std::memcpy(&networkLen, packetData.data(), sizeof(networkLen));
        uint32_t packetLen = networkToHost32(networkLen);
        
        if (packetData.size() >= LENGTH_FIELD_SIZE + packetLen && 
            packetLen >= HEADER_SIZE) {
            data = reinterpret_cast<const uint8_t*>(packetData.data()) + LENGTH_FIELD_SIZE;
            dataLen = HEADER_SIZE;
        } else {
            return Result<MessageHeader>::fail(ErrorCode::ProtocolError, "Invalid packet length");
        }
    } else if (packetData.size() >= HEADER_SIZE) {
        data = reinterpret_cast<const uint8_t*>(packetData.data());
        dataLen = HEADER_SIZE;
    } else {
        return Result<MessageHeader>::fail(ErrorCode::ProtocolError, "Packet too short for header");
    }
    
    MessageHeader header;
    
    if (!MessageHeader::deserialize(data, dataLen, header)) {
        return Result<MessageHeader>::fail(ErrorCode::ProtocolError, "Invalid header fields");
    }
    
    return Result<MessageHeader>::ok(std::move(header));
}

} // namespace protocol
} // namespace chatroom
