#include "protocol.h"
#include <iostream>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <atomic>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace chatroom {
namespace protocol {

std::atomic<uint64_t> ClientProtocol::s_seqCounter{1};

uint32_t ClientProtocol::hostToNetwork32(uint32_t val) {
#ifdef _WIN32
    return htonl(val);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap32(val);
#else
    return val;
#endif
}

uint16_t ClientProtocol::hostToNetwork16(uint16_t val) {
#ifdef _WIN32
    return htons(val);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap16(val);
#else
    return val;
#endif
}

uint64_t ClientProtocol::hostToNetwork64(uint64_t val) {
#if defined(_WIN32) || (__BYTE_ORDER == __LITTLE_ENDIAN)
    return (((uint64_t)hostToNetwork32(val & 0xFFFFFFFFUL)) << 32) | 
           hostToNetwork32((val >> 32) & 0xFFFFFFFFUL);
#else
    return val;
#endif
}

uint32_t ClientProtocol::networkToHost32(uint32_t val) {
#ifdef _WIN32
    return ntohl(val);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap32(val);
#else
    return val;
#endif
}

uint16_t ClientProtocol::networkToHost16(uint16_t val) {
#ifdef _WIN32
    return ntohs(val);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap16(val);
#else
    return val;
#endif
}

uint64_t ClientProtocol::networkToHost64(uint64_t val) {
#if defined(_WIN32) || (__BYTE_ORDER == __LITTLE_ENDIAN)
    return (((uint64_t)networkToHost32(val & 0xFFFFFFFFUL)) << 32) | 
           networkToHost32((val >> 32) & 0xFFFFFFFFUL);
#else
    return val;
#endif
}

std::vector<uint8_t> MessageHeader::serialize() const {
    std::vector<uint8_t> data(HEADER_SIZE);

    uint32_t magicNet = ClientProtocol::hostToNetwork32(magic);
    memcpy(data.data(), &magicNet, sizeof(magicNet));

    uint16_t versionNet = ClientProtocol::hostToNetwork16(version);
    memcpy(data.data() + 4, &versionNet, sizeof(versionNet));

    uint16_t headerLenNet = ClientProtocol::hostToNetwork16(headerLen);
    memcpy(data.data() + 6, &headerLenNet, sizeof(headerLenNet));

    uint16_t msgTypeNet = ClientProtocol::hostToNetwork16(msgType);
    memcpy(data.data() + 8, &msgTypeNet, sizeof(msgTypeNet));

    uint16_t flagsNet = ClientProtocol::hostToNetwork16(flags);
    memcpy(data.data() + 10, &flagsNet, sizeof(flagsNet));

    uint64_t seqNet = ClientProtocol::hostToNetwork64(seq);
    memcpy(data.data() + 12, &seqNet, sizeof(seqNet));

    uint64_t roomIdNet = ClientProtocol::hostToNetwork64(roomId);
    memcpy(data.data() + 20, &roomIdNet, sizeof(roomIdNet));

    uint64_t userIdNet = ClientProtocol::hostToNetwork64(userId);
    memcpy(data.data() + 28, &userIdNet, sizeof(userIdNet));

    uint64_t timestampNet = ClientProtocol::hostToNetwork64(timestamp);
    memcpy(data.data() + 36, &timestampNet, sizeof(timestampNet));

    uint32_t bodyLenNet = ClientProtocol::hostToNetwork32(bodyLen);
    memcpy(data.data() + 44, &bodyLenNet, sizeof(bodyLenNet));

    return data;
}

bool MessageHeader::deserialize(const uint8_t* data, size_t len, MessageHeader& out) {
    if (len < HEADER_SIZE) return false;

    memcpy(&out.magic, data, sizeof(out.magic));
    out.magic = ClientProtocol::networkToHost32(out.magic);

    memcpy(&out.version, data + 4, sizeof(out.version));
    out.version = ClientProtocol::networkToHost16(out.version);

    memcpy(&out.headerLen, data + 6, sizeof(out.headerLen));
    out.headerLen = ClientProtocol::networkToHost16(out.headerLen);

    memcpy(&out.msgType, data + 8, sizeof(out.msgType));
    out.msgType = ClientProtocol::networkToHost16(out.msgType);

    memcpy(&out.flags, data + 10, sizeof(out.flags));
    out.flags = ClientProtocol::networkToHost16(out.flags);

    memcpy(&out.seq, data + 12, sizeof(out.seq));
    out.seq = ClientProtocol::networkToHost64(out.seq);

    memcpy(&out.roomId, data + 20, sizeof(out.roomId));
    out.roomId = ClientProtocol::networkToHost64(out.roomId);

    memcpy(&out.userId, data + 28, sizeof(out.userId));
    out.userId = ClientProtocol::networkToHost64(out.userId);

    memcpy(&out.timestamp, data + 36, sizeof(out.timestamp));
    out.timestamp = ClientProtocol::networkToHost64(out.timestamp);

    memcpy(&out.bodyLen, data + 44, sizeof(out.bodyLen));
    out.bodyLen = ClientProtocol::networkToHost32(out.bodyLen);

    return true;
}

uint64_t ClientProtocol::getCurrentTimestampMs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

Result<std::string> ClientProtocol::encodeMessage(MessageType type,
                                                    uint64_t userId,
                                                    uint64_t roomId,
                                                    const std::string& body) {
    MessageHeader header;
    header.msgType = static_cast<uint16_t>(type);
    header.seq = s_seqCounter.fetch_add(1);
    header.userId = userId;
    header.roomId = roomId;
    header.timestamp = getCurrentTimestampMs();
    header.bodyLen = body.size();

    if (!header.isValid()) {
        return Result<std::string>::fail(-1, "Invalid header");
    }

    if (body.size() > MAX_PACKET_SIZE - HEADER_SIZE) {
        return Result<std::string>::fail(-1, "Body too large");
    }

    auto headerData = header.serialize();

    uint32_t packetLen = static_cast<uint32_t>(HEADER_SIZE + header.bodyLen);
    uint32_t networkLen = hostToNetwork32(packetLen);

    std::string packet;
    packet.reserve(LENGTH_FIELD_SIZE + packetLen);

    packet.append(reinterpret_cast<const char*>(&networkLen), sizeof(networkLen));
    packet.append(reinterpret_cast<const char*>(headerData.data()), headerData.size());
    packet.append(body);

    return Result<std::string>::ok(std::move(packet));
}

Result<Message> ClientProtocol::decodeMessage(const std::string& packet) {
    if (packet.size() < LENGTH_FIELD_SIZE + HEADER_SIZE) {
        return Result<Message>::fail(-1, "Packet too short");
    }

    uint32_t networkLen = 0;
    memcpy(&networkLen, packet.data(), sizeof(networkLen));
    uint32_t declaredLen = networkToHost32(networkLen);

    size_t actualContentLen = packet.size() - LENGTH_FIELD_SIZE;

    if (actualContentLen < HEADER_SIZE) {
        return Result<Message>::fail(-1, "Packet too short for header");
    }

    MessageHeader header;
    const uint8_t* headerData = reinterpret_cast<const uint8_t*>(packet.data()) + LENGTH_FIELD_SIZE;

    if (!MessageHeader::deserialize(headerData, HEADER_SIZE, header)) {
        return Result<Message>::fail(-1, "Invalid header");
    }

    size_t expectedTotal = LENGTH_FIELD_SIZE + HEADER_SIZE + header.bodyLen;
    if (packet.size() < expectedTotal) {
        return Result<Message>::fail(-1, "Incomplete body");
    }

    Message msg;
    msg.header = std::move(header);
    msg.body = packet.substr(LENGTH_FIELD_SIZE + HEADER_SIZE, msg.header.bodyLen);

    return Result<Message>::ok(std::move(msg));
}

} // namespace protocol
} // namespace chatroom
