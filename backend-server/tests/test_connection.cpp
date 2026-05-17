#include <gtest/gtest.h>
#include "net/connection.h"

using namespace chatroom;

class ConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ConnectionTest, CreateConnection) {
    net::Connection conn(12345);
    
    EXPECT_EQ(conn.getFd(), 12345);
    EXPECT_EQ(conn.getState(), net::ConnState::UNAUTHED);
    EXPECT_EQ(conn.getUserId(), uint64_t{0});
    EXPECT_EQ(conn.getCurrentRoomId(), uint64_t{0});
}

TEST_F(ConnectionTest, BindUser) {
    net::Connection conn(100);
    
    conn.bindUser(42, "test_token_123", 1);
    
    EXPECT_EQ(conn.getState(), net::ConnState::AUTHED);
    EXPECT_EQ(conn.getUserId(), uint64_t{42});
    EXPECT_STREQ(conn.getToken().c_str(), "test_token_123");
    EXPECT_EQ(conn.getUserRole(), 1u);
}

TEST_F(ConnectionTest, UnbindUser) {
    net::Connection conn(101);
    
    conn.bindUser(42, "token", 1);
    conn.setCurrentRoom(999);
    
    conn.unbindUser();
    
    EXPECT_EQ(conn.getState(), net::ConnState::UNAUTHED);
    EXPECT_EQ(conn.getUserId(), uint64_t{0});
    EXPECT_EQ(conn.getCurrentRoomId(), uint64_t{0});
}

TEST_F(ConnectionTest, RoomBinding) {
    net::Connection conn(102);
    
    conn.setCurrentRoom(8888);
    EXPECT_EQ(conn.getCurrentRoomId(), uint64_t{8888});
    
    conn.setCurrentRoom(7777);
    EXPECT_EQ(conn.getCurrentRoomId(), uint64_t{7777});
}

TEST_F(ConnectionTest, ReadBuffer) {
    net::Connection conn(103);
    
    const char* data1 = "Hello";
    const char* data2 = " World";
    
    conn.appendReadBuffer(data1, 5);
    conn.appendReadBuffer(data2, 6);
    
    EXPECT_EQ(conn.getReadBufferSize(), size_t{11});
    EXPECT_EQ(conn.getReadBuffer(), "Hello World");
    
    conn.clearReadBuffer();
    EXPECT_EQ(conn.getReadBufferSize(), size_t{0});
    EXPECT_TRUE(conn.getReadBuffer().empty());
}

TEST(ConnectionManagerTest, Singleton) {
    auto& mgr1 = net::ConnectionManager::getInstance();
    auto& mgr2 = net::ConnectionManager::getInstance();
    
    EXPECT_EQ(&mgr1, &mgr2);
}

TEST(ConnectionManagerTest, AddRemove) {
    auto& mgr = net::ConnectionManager::getInstance();
    
    auto conn1 = std::make_shared<net::Connection>(2001);
    auto result = mgr.addConnection(2001, conn1);
    
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(mgr.getActiveConnectionCount(), size_t{1});
    
    auto retrieved = mgr.getConnection(2001);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getFd(), 2001);
    
    mgr.removeConnection(2001);
    EXPECT_EQ(mgr.getActiveConnectionCount(), size_t{0});
    
    auto notFound = mgr.getConnection(2001);
    EXPECT_EQ(notFound, nullptr);
}
