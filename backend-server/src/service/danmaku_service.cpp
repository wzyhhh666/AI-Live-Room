#include "service/danmaku_service.h"
#include "service/filter_service.h"
#include "common/logging.h"
#include <map>
#include <algorithm>

namespace chatroom {
namespace service {

DanmakuService::DanmakuService(std::shared_ptr<DataRepository> dataRepo,
                               std::shared_ptr<FilterService> filterSvc)
    : m_dataRepository(std::move(dataRepo))
    , m_filterService(std::move(filterSvc)) {}

void DanmakuService::setFilterService(std::shared_ptr<FilterService> filterSvc) {
    m_filterService = std::move(filterSvc);
}

Result<void> DanmakuService::validateInput(const std::string& content) {
    if (content.empty()) {
        return VoidResult::fail(ErrorCode::InvalidArgument, "Danmaku content cannot be empty");
    }
    
    if (content.size() > 512) {
        return VoidResult::fail(ErrorCode::InvalidArgument, 
            "Danmaku content too long (max 512 characters)");
    }
    
    for (char c : content) {
        if (static_cast<unsigned char>(c) < 0x20 || c == 0x7F) {
            LOG_WARN("DanmakuService: invalid character in content");
        }
    }
    
    return VoidResult::ok();
}

Result<std::string> DanmakuService::filterContent(uint64_t roomId, const std::string& content) {
    if (!m_filterService) {
        return Result<std::string>::ok(content);
    }
    
    auto filterResult = m_filterService->filterText(roomId, content);
    if (!filterResult.isOk()) {
        LOG_WARN("DanmakuService: filter service error: {}", filterResult.message());
        return Result<std::string>::ok(content);
    }
    
    auto& filtered = filterResult.value();
    if (filtered.wasBlocked) {
        return Result<std::string>::fail(ErrorCode::PermissionDenied, 
            "Content blocked by sensitive word filter");
    }
    
    return Result<std::string>::ok(filtered.filteredText);
}

Result<void> DanmakuService::processDanmaku(int fd, uint64_t roomId, 
                                            uint64_t userId,
                                            const std::string& userName, 
                                            const std::string& content) {
    auto validateResult = validateInput(content);
    if (!validateResult.isOk()) {
        LOG_WARN("DanmakuService: validation failed for user={}: {}", 
                 userId, validateResult.message());
        return validateResult;
    }
    
    auto filterResult = filterContent(roomId, content);
    if (!filterResult.isOk()) {
        LOG_WARN("DanmakuService: content blocked for user={} in room={}: {}", 
                 userId, roomId, filterResult.message());
        return VoidResult::fail(filterResult.code(), filterResult.message());
    }
    
    std::string filteredContent = filterResult.value();
    
    if (!m_dataRepository) {
        LOG_ERROR("DanmakuService: data repository not initialized");
        return VoidResult::fail(ErrorCode::InternalError, "Data repository not initialized");
    }
    
    auto saveResult = m_dataRepository->saveDanmaku(roomId, userId, filteredContent);
    if (!saveResult.isOk()) {
        LOG_WARN("DanmakuService: failed to save danmaku: {}", saveResult.message());
    }
    
    LOG_INFO("DanmakuService: processed danmaku from user={} in room={}, content={}", 
             userName, roomId, filteredContent.substr(0, std::min(filteredContent.size(), size_t(50))));
    
    return VoidResult::ok();
}

RoomService::RoomService(std::shared_ptr<DataRepository> dataRepo)
    : m_dataRepository(std::move(dataRepo)) {}

bool RoomService::validateStateTransition(RoomState from, RoomState to) {
    static const std::map<std::pair<RoomState, RoomState>, bool> validTransitions = {
        {{RoomState::CREATED, RoomState::READY}, true},
        {{RoomState::READY, RoomState::LIVE}, true},
        {{RoomState::READY, RoomState::CLOSED}, true},
        {{RoomState::LIVE, RoomState::OFFLINE}, true},
        {{RoomState::OFFLINE, RoomState::LIVE}, true},
        {{RoomState::OFFLINE, RoomState::CLOSED}, true},
        {{RoomState::CREATED, RoomState::CLOSED}, true}
    };
    
    auto key = std::make_pair(from, to);
    auto it = validTransitions.find(key);
    return it != validTransitions.end() && it->second;
}

Result<uint64_t> RoomService::createRoom(uint64_t hostUserId, const std::string& roomName) {
    if (!m_dataRepository) {
        return Result<uint64_t>::fail(ErrorCode::InternalError, "Data repository not initialized");
    }
    
    if (roomName.empty() || roomName.size() > 100) {
        return Result<uint64_t>::fail(ErrorCode::InvalidArgument, "Invalid room name");
    }
    
    auto result = m_dataRepository->createRoom(hostUserId, roomName);
    if (result.isOk()) {
        LOG_INFO("RoomService: created room={} by user={}", result.value(), hostUserId);
    }
    
    return result;
}

Result<void> RoomService::startLive(uint64_t roomId) {
    if (!m_dataRepository) {
        return VoidResult::fail(ErrorCode::InternalError, "Data repository not initialized");
    }
    
    auto roomInfo = m_dataRepository->queryRoomById(roomId);
    if (!roomInfo.isOk()) {
        return VoidResult::fail(ErrorCode::NotFound, "Room not found");
    }
    
    if (!validateStateTransition(roomInfo.value().state, RoomState::LIVE)) {
        return VoidResult::fail(ErrorCode::InvalidArgument, 
            "Cannot transition to LIVE from current state");
    }
    
    auto result = m_dataRepository->updateRoomState(roomId, static_cast<uint32_t>(RoomState::LIVE));
    if (result.isOk()) {
        LOG_INFO("RoomService: room={} started live broadcast", roomId);
    }
    
    return result;
}

Result<void> RoomService::stopLive(uint64_t roomId) {
    if (!m_dataRepository) {
        return VoidResult::fail(ErrorCode::InternalError, "Data repository not initialized");
    }
    
    auto result = m_dataRepository->updateRoomState(roomId, static_cast<uint32_t>(RoomState::OFFLINE));
    if (result.isOk()) {
        LOG_INFO("RoomService: room={} stopped live broadcast", roomId);
    }
    
    return result;
}

Result<void> RoomService::closeRoom(uint64_t roomId) {
    if (!m_dataRepository) {
        return VoidResult::fail(ErrorCode::InternalError, "Data repository not initialized");
    }
    
    auto result = m_dataRepository->updateRoomState(roomId, static_cast<uint32_t>(RoomState::CLOSED));
    if (result.isOk()) {
        LOG_INFO("RoomService: room={} closed permanently", roomId);
    }
    
    return result;
}

Result<void> RoomService::joinRoom(uint64_t roomId, uint64_t userId) {
    if (!m_dataRepository) {
        return VoidResult::fail(ErrorCode::InternalError, "Data repository not initialized");
    }
    
    auto roomInfo = m_dataRepository->queryRoomById(roomId);
    if (!roomInfo.isOk()) {
        return VoidResult::fail(ErrorCode::NotFound, "Room not found");
    }
    
    if (roomInfo.value().state == RoomState::CLOSED) {
        return VoidResult::fail(ErrorCode::PermissionDenied, "Room is closed");
    }
    
    auto result = m_dataRepository->addRoomMember(roomId, userId);
    if (result.isOk()) {
        LOG_INFO("RoomService: user={} joined room={}", userId, roomId);
    }
    
    return result;
}

Result<void> RoomService::leaveRoom(uint64_t roomId, uint64_t userId) {
    if (!m_dataRepository) {
        return VoidResult::fail(ErrorCode::InternalError, "Data repository not initialized");
    }
    
    auto result = m_dataRepository->updateRoomMemberLeaveTime(roomId, userId);
    if (result.isOk()) {
        LOG_INFO("RoomService: user={} left room={}", userId, roomId);
    }
    
    return result;
}

Result<RoomInfo> RoomService::getRoomInfo(uint64_t roomId) {
    if (!m_dataRepository) {
        return Result<RoomInfo>::fail(ErrorCode::InternalError, "Data repository not initialized");
    }
    
    return m_dataRepository->queryRoomById(roomId);
}

Result<std::vector<RoomInfo>> RoomService::queryRoomList(int page, int pageSize, 
                                                        uint32_t stateFilter) {
    if (!m_dataRepository) {
        return Result<std::vector<RoomInfo>>::fail(ErrorCode::InternalError, "Data repository not initialized");
    }
    
    if (page < 1) page = 1;
    if (pageSize < 1) pageSize = 20;
    if (pageSize > 100) pageSize = 100;
    
    return m_dataRepository->queryRoomList(page, pageSize, stateFilter);
}

} // namespace service
} // namespace chatroom
