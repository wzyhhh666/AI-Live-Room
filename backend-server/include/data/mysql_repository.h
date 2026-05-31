#pragma once

#include "data/repository.h"
#include "data/mysql_pool.h"
#include <memory>

namespace chatroom {

class MySqlRepository : public service::DataRepository {
public:
    explicit MySqlRepository(std::shared_ptr<data::MySqlPool> pool);

    Result<service::UserInfo> queryUserById(uint64_t userId) override;
    Result<service::UserInfo> queryUserByName(const std::string& userName) override;
    Result<void> updateUserToken(uint64_t userId, const std::string& token) override;

    Result<uint64_t> createRoom(uint64_t hostUserId, const std::string& name) override;
    Result<void> updateRoomState(uint64_t roomId, uint32_t state) override;
    Result<void> updateRoomOnlineCount(uint64_t roomId, int count) override;
    Result<std::vector<service::RoomInfo>> queryRoomList(int page, int pageSize,
                                                         uint32_t stateFilter = 999) override;
    Result<service::RoomInfo> queryRoomById(uint64_t roomId) override;

    Result<void> addRoomMember(uint64_t roomId, uint64_t userId) override;
    Result<void> updateRoomMemberLeaveTime(uint64_t roomId, uint64_t userId) override;
    Result<int> queryRoomOnlineCount(uint64_t roomId) override;

    Result<uint64_t> saveDanmaku(uint64_t roomId, uint64_t userId,
                                 const std::string& content) override;
    Result<std::vector<service::DanmakuRecord>> queryRecentDanmaku(uint64_t roomId,
                                                                    int count) override;

private:
    std::shared_ptr<data::MySqlPool> m_pool;
    std::string escape(const std::string& s, MYSQL* conn);
};

} // namespace chatroom
