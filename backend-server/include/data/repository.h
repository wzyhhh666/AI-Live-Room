#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "common/result.h"

namespace chatroom {
namespace service {

enum class RoomState : uint32_t {
    CREATED = 0,
    READY = 1,
    LIVE = 2,
    OFFLINE = 3,
    CLOSED = 4
};

struct UserInfo {
    uint64_t id = 0;
    std::string userName;
    std::string passwordHash;
    std::string avatarUrl;
    uint32_t role = 0;
    uint8_t status = 1;
};

struct RoomInfo {
    uint64_t roomId = 0;
    std::string roomName;
    uint64_t hostUserId = 0;
    std::string coverUrl;
    RoomState state = RoomState::CREATED;
    int onlineCount = 0;
    uint64_t danmakuCount = 0;
};

struct RoomMemberInfo {
    uint64_t id = 0;
    uint64_t roomId = 0;
    uint64_t userId = 0;
    uint32_t memberRole = 0;
    std::string joinTime;
    std::string leaveTime;
};

struct DanmakuRecord {
    uint64_t id = 0;
    uint64_t roomId = 0;
    uint64_t userId = 0;
    std::string userName;
    std::string content;
    uint8_t contentStatus = 0;
    std::string createdAt;
};

class DataRepository {
public:
    virtual ~DataRepository() = default;
    
    virtual Result<UserInfo> queryUserById(uint64_t userId) = 0;
    virtual Result<UserInfo> queryUserByName(const std::string& userName) = 0;
    virtual Result<void> updateUserToken(uint64_t userId, const std::string& token) = 0;
    
    virtual Result<uint64_t> createRoom(uint64_t hostUserId, const std::string& name) = 0;
    virtual Result<void> updateRoomState(uint64_t roomId, uint32_t state) = 0;
    virtual Result<void> updateRoomOnlineCount(uint64_t roomId, int count) = 0;
    virtual Result<std::vector<RoomInfo>> queryRoomList(int page, int pageSize, 
                                                       uint32_t stateFilter = 999) = 0;
    virtual Result<RoomInfo> queryRoomById(uint64_t roomId) = 0;
    
    virtual Result<void> addRoomMember(uint64_t roomId, uint64_t userId) = 0;
    virtual Result<void> updateRoomMemberLeaveTime(uint64_t roomId, uint64_t userId) = 0;
    virtual Result<int> queryRoomOnlineCount(uint64_t roomId) = 0;
    
    virtual Result<uint64_t> saveDanmaku(uint64_t roomId, uint64_t userId, 
                                         const std::string& content) = 0;
    virtual Result<std::vector<DanmakuRecord>> queryRecentDanmaku(uint64_t roomId, int count) = 0;
};

} // namespace service
} // namespace chatroom
