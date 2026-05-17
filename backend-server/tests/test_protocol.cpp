#include <gtest/gtest.h>
#include "protocol/message_header.h"
#include "protocol/message_codec.h"
#include "protocol/message_dispatcher.h"
#include "util/utils.h"

using namespace chatroom;
using namespace chatroom::protocol;

class ProtocolTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ProtocolTest, HeaderSerialization) {
    MessageHeader header;
    header.magic = MAGIC_NUMBER;
    header.version = PROTOCOL_VERSION;
    header.msgType = static_cast<uint16_t>(MessageType::MSG_DANMAKU);
    header.seq = 12345;
    header.roomId = 1001;
    header.userId = 2001;
    header.timestamp = 9876543210ULL;
    header.bodyLen = 50;
    
    EXPECT_TRUE(header.isValid());
    
    auto data = header.serialize();
    EXPECT_EQ(data.size(), HEADER_SIZE);
    
    MessageHeader decoded;
    ASSERT_TRUE(MessageHeader::deserialize(data.data(), data.size(), decoded));
    
    EXPECT_EQ(decoded.magic, header.magic);
    EXPECT_EQ(decoded.version, header.version);
    EXPECT_EQ(decoded.msgType, header.msgType);
    EXPECT_EQ(decoded.seq, header.seq);
    EXPECT_EQ(decoded.roomId, header.roomId);
    EXPECT_EQ(decoded.userId, header.userId);
    EXPECT_EQ(decoded.timestamp, header.timestamp);
    EXPECT_EQ(decoded.bodyLen, header.bodyLen);
}

TEST_F(ProtocolTest, EncodeDecodeRoundTrip) {
    MessageHeader header;
    header.msgType = static_cast<uint16_t>(MessageType::MSG_LOGIN);
    header.seq = 999;
    header.roomId = 0;
    header.userId = 0;
    header.timestamp = utils::getCurrentTimestampMs();
    
    std::string bodyData = "test protobuf body data here";
    header.bodyLen = bodyData.size();
    
    auto encodeResult = MessageCodec::encode(header, bodyData);
    ASSERT_TRUE(encodeResult.isOk());
    
    auto decodeResult = MessageCodec::decode(encodeResult.value());
    ASSERT_TRUE(decodeResult.isOk());
    
    const auto& msg = decodeResult.value();
    EXPECT_EQ(msg.header.msgType, header.msgType);
    EXPECT_EQ(msg.header.seq, header.seq);
    EXPECT_EQ(msg.header.bodyLen, bodyData.size());
    EXPECT_EQ(msg.body, bodyData);
}

TEST_F(ProtocolTest, InvalidHeader) {
    MessageHeader badHeader;
    badHeader.magic = 0xDEADBEEF;
    badHeader.bodyLen = MAX_PACKET_SIZE + 1;
    
    EXPECT_FALSE(badHeader.isValid());
}

TEST_F(ProtocolTest, MessageTypeNames) {
    EXPECT_STRCASEEQ(getMessageTypeName(MessageType::MSG_LOGIN), "MSG_LOGIN");
    EXPECT_STRCASEEQ(getMessageTypeName(MessageType::MSG_DANMAKU), "MSG_DANMAKU");
    EXPECT_STRCASEEQ(getMessageTypeName(MessageType::MSG_HEARTBEAT), "MSG_HEARTBEAT");
    EXPECT_STRCASEEQ(getMessageTypeName(static_cast<MessageType>(9999)), "UNKNOWN");
}

TEST_F(ProtocolTest, DecodeHeaderOnly) {
    MessageHeader header;
    header.msgType = static_cast<uint16_t>(MessageType::MSG_JOIN_ROOM);
    header.bodyLen = 100;
    
    std::string bodyData(100, 'A');
    auto encodeResult = MessageCodec::encode(header, bodyData);
    ASSERT_TRUE(encodeResult.isOk());
    
    auto decodeResult = MessageCodec::decodeHeaderOnly(encodeResult.value());
    ASSERT_TRUE(decodeResult.isOk());
    
    EXPECT_EQ(decodeResult.value().msgType, header.msgType);
    EXPECT_EQ(decodeResult.value().bodyLen, header.bodyLen);
}

TEST_F(ProtocolTest, PacketTooShort) {
    std::string shortPacket("abc");
    auto result = MessageCodec::decode(shortPacket);
    
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.code(), ErrorCode::ProtocolError);
}

TEST_F(ProtocolTest, DispatcherRegisterAndDispatch) {
    auto& dispatcher = MessageDispatcher::getInstance();
    
    int calledFd = -1;
    dispatcher.registerHandler(
        static_cast<uint16_t>(MessageType::MSG_LOGIN),
        [&calledFd](int fd, Message& msg) -> Result<void> {
            calledFd = fd;
            return VoidResult::ok();
        }
    );
    
    EXPECT_TRUE(dispatcher.hasHandler(static_cast<uint16_t>(MessageType::MSG_LOGIN)));
    EXPECT_FALSE(dispatcher.hasHandler(static_cast<uint16_t>(MessageType::MSG_GIFT)));
    
    Message testMsg;
    testMsg.header.msgType = static_cast<uint16_t>(MessageType::MSG_LOGIN);
    auto dispatchResult = dispatcher.dispatch(42, testMsg);
    
    ASSERT_TRUE(dispatchResult.isOk());
    EXPECT_EQ(calledFd, 42);
}
