#pragma once

#include <memory>
#include "common/result.h"
#include "data/repository.h"
#include "net/connection.h"

namespace chatroom {
namespace service {

class FilterService;
class DataRepository;

class DanmakuService {
public:
    explicit DanmakuService(std::shared_ptr<DataRepository> dataRepo,
                           std::shared_ptr<FilterService> filterSvc = nullptr);
    
    Result<void> processDanmaku(int fd, uint64_t roomId, uint64_t userId,
                                const std::string& userName, const std::string& content);
    
    void setFilterService(std::shared_ptr<FilterService> filterSvc);

private:
    Result<void> validateInput(const std::string& content);
    Result<std::string> filterContent(uint64_t roomId, const std::string& content);
    
    std::shared_ptr<DataRepository> m_dataRepository;
    std::shared_ptr<FilterService> m_filterService;
};

class RoomService {
public:
    explicit RoomService(std::shared_ptr<DataRepository> dataRepo);
    
    Result<uint64_t> createRoom(uint64_t hostUserId, const std::string& roomName);
    Result<void> startLive(uint64_t roomId);
    Result<void> stopLive(uint64_t roomId);
    Result<void> closeRoom(uint64_t roomId);
    
    Result<void> joinRoom(uint64_t roomId, uint64_t userId);
    Result<void> leaveRoom(uint64_t roomId, uint64_t userId);
    
    Result<RoomInfo> getRoomInfo(uint64_t roomId);
    Result<std::vector<RoomInfo>> queryRoomList(int page, int pageSize, 
                                                  uint32_t stateFilter = 999);
    
private:
    bool validateStateTransition(RoomState from, RoomState to);
    
    std::shared_ptr<DataRepository> m_dataRepository;
};

} // namespace service
} // namespace chatroom
