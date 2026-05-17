#pragma once

#include "message_header.h"
#include "common/result.h"
#include <string>

namespace chatroom {
namespace protocol {

struct Message {
    MessageHeader header;
    std::string body;
};

class MessageCodec {
public:
    static Result<std::string> encode(const MessageHeader& header, 
                                       const std::string& protobufBody);
    
    static Result<Message> decode(const std::string& packetData);
    
    static Result<MessageHeader> decodeHeaderOnly(const std::string& packetData);

private:
    static uint32_t hostToNetwork32(uint32_t val);
    static uint16_t hostToNetwork16(uint16_t val);
    static uint64_t hostToNetwork64(uint64_t val);
    
    static uint32_t networkToHost32(uint32_t val);
    static uint16_t networkToHost16(uint16_t val);
    static uint64_t networkToHost64(uint64_t val);
};

} // namespace protocol
} // namespace chatroom
