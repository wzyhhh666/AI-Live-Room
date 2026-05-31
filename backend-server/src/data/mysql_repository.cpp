#include "data/mysql_repository.h"
#include "common/logging.h"
#include <cstring>
#include <cstdio>

namespace chatroom {

MySqlRepository::MySqlRepository(std::shared_ptr<data::MySqlPool> pool)
    : m_pool(std::move(pool)) {}

std::string MySqlRepository::escape(const std::string& s, MYSQL* conn) {
    if (s.empty()) return s;
    std::string escaped(s.size() * 2 + 1, '\0');
    unsigned long len = mysql_real_escape_string(conn, &escaped[0], s.c_str(), s.size());
    escaped.resize(len);
    return escaped;
}

static std::string stateToDBString(uint32_t state) {
    switch (static_cast<service::RoomState>(state)) {
        case service::RoomState::CREATED: return "idle";
        case service::RoomState::LIVE:    return "living";
        case service::RoomState::CLOSED:  return "closed";
        default: return "idle";
    }
}

static uint32_t dbStringToState(const std::string& s) {
    if (s == "living") return static_cast<uint32_t>(service::RoomState::LIVE);
    if (s == "closed") return static_cast<uint32_t>(service::RoomState::CLOSED);
    return static_cast<uint32_t>(service::RoomState::CREATED);
}

Result<service::UserInfo> MySqlRepository::queryUserById(uint64_t userId) {
    auto connResult = m_pool->getConnection();
    if (!connResult.isOk()) return Result<service::UserInfo>::fail(ErrorCode::DatabaseError, "No DB connection");

    MYSQL* conn = connResult.value();
    char sql[512];
    snprintf(sql, sizeof(sql), "SELECT id, username, nickname, avatar_url, role FROM users WHERE id=%lu", userId);

    if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
        m_pool->returnConnection(conn);
        return Result<service::UserInfo>::fail(ErrorCode::DatabaseError, mysql_error(conn));
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        m_pool->returnConnection(conn);
        return Result<service::UserInfo>::fail(ErrorCode::DatabaseError, "No result");
    }

    service::UserInfo info;
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        info.id = row[0] ? std::stoull(row[0]) : 0;
        info.userName = row[1] ? row[1] : "";
        info.avatarUrl = row[3] ? row[3] : "";
        info.role = row[4] ? std::stoul(row[4]) : 0;
        info.status = 1;
    }

    mysql_free_result(result);
    m_pool->returnConnection(conn);

    if (row == nullptr) {
        return Result<service::UserInfo>::fail(ErrorCode::NotFound, "User not found");
    }
    return Result<service::UserInfo>::ok(info);
}

Result<service::UserInfo> MySqlRepository::queryUserByName(const std::string& userName) {
    auto connResult = m_pool->getConnection();
    if (!connResult.isOk()) return Result<service::UserInfo>::fail(ErrorCode::DatabaseError, "No DB connection");

    MYSQL* conn = connResult.value();
    std::string escaped = escape(userName, conn);
    char sql[512];
    snprintf(sql, sizeof(sql), "SELECT id, username, nickname, avatar_url, role FROM users WHERE username='%s'", escaped.c_str());

    if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
        m_pool->returnConnection(conn);
        return Result<service::UserInfo>::fail(ErrorCode::DatabaseError, mysql_error(conn));
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) { m_pool->returnConnection(conn); return Result<service::UserInfo>::fail(ErrorCode::DatabaseError, "No result"); }

    service::UserInfo info;
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        info.id = row[0] ? std::stoull(row[0]) : 0;
        info.userName = row[1] ? row[1] : "";
        info.avatarUrl = row[3] ? row[3] : "";
        info.role = row[4] ? std::stoul(row[4]) : 0;
        info.status = 1;
    }

    mysql_free_result(result);
    m_pool->returnConnection(conn);

    if (row == nullptr) {
        return Result<service::UserInfo>::fail(ErrorCode::NotFound, "User not found");
    }
    return Result<service::UserInfo>::ok(info);
}

Result<void> MySqlRepository::updateUserToken(uint64_t userId, const std::string& token) {
    auto connResult = m_pool->getConnection();
    if (!connResult.isOk()) return VoidResult::fail(ErrorCode::DatabaseError, "No DB connection");

    MYSQL* conn = connResult.value();
    std::string escaped = escape(token, conn);
    char sql[512];
    snprintf(sql, sizeof(sql), "UPDATE users SET token='%s' WHERE id=%lu", escaped.c_str(), userId);

    if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
        m_pool->returnConnection(conn);
        return VoidResult::fail(ErrorCode::DatabaseError, mysql_error(conn));
    }
    m_pool->returnConnection(conn);
    return VoidResult::ok();
}

Result<uint64_t> MySqlRepository::createRoom(uint64_t hostUserId, const std::string& name) {
    auto connResult = m_pool->getConnection();
    if (!connResult.isOk()) return Result<uint64_t>::fail(ErrorCode::DatabaseError, "No DB connection");

    MYSQL* conn = connResult.value();
    std::string escaped = escape(name, conn);
    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO rooms (room_name, host_id, host_name, state, online_count) VALUES ('%s', %lu, 'host', 'idle', 0)",
        escaped.c_str(), hostUserId);

    if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
        m_pool->returnConnection(conn);
        return Result<uint64_t>::fail(ErrorCode::DatabaseError, mysql_error(conn));
    }

    uint64_t id = mysql_insert_id(conn);
    m_pool->returnConnection(conn);
    return Result<uint64_t>::ok(id);
}

Result<void> MySqlRepository::updateRoomState(uint64_t roomId, uint32_t state) {
    auto connResult = m_pool->getConnection();
    if (!connResult.isOk()) return VoidResult::fail(ErrorCode::DatabaseError, "No DB connection");

    MYSQL* conn = connResult.value();
    std::string dbState = stateToDBString(state);
    char sql[256];
    snprintf(sql, sizeof(sql), "UPDATE rooms SET state='%s' WHERE id=%lu", dbState.c_str(), roomId);

    if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
        m_pool->returnConnection(conn);
        return VoidResult::fail(ErrorCode::DatabaseError, mysql_error(conn));
    }
    m_pool->returnConnection(conn);
    return VoidResult::ok();
}

Result<void> MySqlRepository::updateRoomOnlineCount(uint64_t roomId, int count) {
    auto connResult = m_pool->getConnection();
    if (!connResult.isOk()) return VoidResult::fail(ErrorCode::DatabaseError, "No DB connection");

    MYSQL* conn = connResult.value();
    char sql[256];
    snprintf(sql, sizeof(sql), "UPDATE rooms SET online_count=%d WHERE id=%lu", count, roomId);

    if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
        m_pool->returnConnection(conn);
        return VoidResult::fail(ErrorCode::DatabaseError, mysql_error(conn));
    }
    m_pool->returnConnection(conn);
    return VoidResult::ok();
}

Result<std::vector<service::RoomInfo>> MySqlRepository::queryRoomList(int page, int pageSize, uint32_t stateFilter) {
    auto connResult = m_pool->getConnection();
    if (!connResult.isOk()) return Result<std::vector<service::RoomInfo>>::fail(ErrorCode::DatabaseError, "No DB connection");

    MYSQL* conn = connResult.value();
    int offset = (page - 1) * pageSize;
    char sql[512];
    if (stateFilter == 999) {
        snprintf(sql, sizeof(sql),
            "SELECT id, room_name, host_id, cover_image, state, online_count FROM rooms ORDER BY online_count DESC LIMIT %d OFFSET %d",
            pageSize, offset);
    } else {
        std::string dbState = stateToDBString(stateFilter);
        snprintf(sql, sizeof(sql),
            "SELECT id, room_name, host_id, cover_image, state, online_count FROM rooms WHERE state='%s' ORDER BY online_count DESC LIMIT %d OFFSET %d",
            dbState.c_str(), pageSize, offset);
    }

    if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
        m_pool->returnConnection(conn);
        return Result<std::vector<service::RoomInfo>>::fail(ErrorCode::DatabaseError, mysql_error(conn));
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) { m_pool->returnConnection(conn); return Result<std::vector<service::RoomInfo>>::ok({}); }

    std::vector<service::RoomInfo> rooms;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        service::RoomInfo info;
        info.roomId = row[0] ? std::stoull(row[0]) : 0;
        info.roomName = row[1] ? row[1] : "";
        info.hostUserId = row[2] ? std::stoull(row[2]) : 0;
        info.coverUrl = row[3] ? row[3] : "";
        info.state = static_cast<service::RoomState>(dbStringToState(row[4] ? row[4] : "idle"));
        info.onlineCount = row[5] ? std::stoi(row[5]) : 0;
        rooms.push_back(info);
    }

    mysql_free_result(result);
    m_pool->returnConnection(conn);
    return Result<std::vector<service::RoomInfo>>::ok(std::move(rooms));
}

Result<service::RoomInfo> MySqlRepository::queryRoomById(uint64_t roomId) {
    auto connResult = m_pool->getConnection();
    if (!connResult.isOk()) return Result<service::RoomInfo>::fail(ErrorCode::DatabaseError, "No DB connection");

    MYSQL* conn = connResult.value();
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT id, room_name, host_id, cover_image, state, online_count FROM rooms WHERE id=%lu", roomId);

    if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
        m_pool->returnConnection(conn);
        return Result<service::RoomInfo>::fail(ErrorCode::DatabaseError, mysql_error(conn));
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) { m_pool->returnConnection(conn); return Result<service::RoomInfo>::fail(ErrorCode::DatabaseError, "No result"); }

    service::RoomInfo info;
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        info.roomId = row[0] ? std::stoull(row[0]) : 0;
        info.roomName = row[1] ? row[1] : "";
        info.hostUserId = row[2] ? std::stoull(row[2]) : 0;
        info.coverUrl = row[3] ? row[3] : "";
        info.state = static_cast<service::RoomState>(dbStringToState(row[4] ? row[4] : "idle"));
        info.onlineCount = row[5] ? std::stoi(row[5]) : 0;
    }

    mysql_free_result(result);
    m_pool->returnConnection(conn);

    if (row == nullptr) {
        return Result<service::RoomInfo>::fail(ErrorCode::NotFound, "Room not found");
    }
    return Result<service::RoomInfo>::ok(info);
}

Result<void> MySqlRepository::addRoomMember(uint64_t roomId, uint64_t userId) {
    auto connResult = m_pool->getConnection();
    if (!connResult.isOk()) return VoidResult::fail(ErrorCode::DatabaseError, "No DB connection");

    MYSQL* conn = connResult.value();
    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT IGNORE INTO room_members (room_id, user_id, username, join_time) VALUES (%lu, %lu, 'user', NOW())",
        roomId, userId);

    if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
        m_pool->returnConnection(conn);
        return VoidResult::fail(ErrorCode::DatabaseError, mysql_error(conn));
    }
    m_pool->returnConnection(conn);
    return VoidResult::ok();
}

Result<void> MySqlRepository::updateRoomMemberLeaveTime(uint64_t roomId, uint64_t userId) {
    auto connResult = m_pool->getConnection();
    if (!connResult.isOk()) return VoidResult::fail(ErrorCode::DatabaseError, "No DB connection");

    MYSQL* conn = connResult.value();
    char sql[512];
    snprintf(sql, sizeof(sql),
        "UPDATE room_members SET leave_time=NOW() WHERE room_id=%lu AND user_id=%lu AND leave_time IS NULL",
        roomId, userId);

    if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
        m_pool->returnConnection(conn);
        return VoidResult::fail(ErrorCode::DatabaseError, mysql_error(conn));
    }
    m_pool->returnConnection(conn);
    return VoidResult::ok();
}

Result<int> MySqlRepository::queryRoomOnlineCount(uint64_t roomId) {
    auto connResult = m_pool->getConnection();
    if (!connResult.isOk()) return Result<int>::fail(ErrorCode::DatabaseError, "No DB connection");

    MYSQL* conn = connResult.value();
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT COUNT(*) FROM room_members WHERE room_id=%lu AND leave_time IS NULL", roomId);

    if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
        m_pool->returnConnection(conn);
        return Result<int>::fail(ErrorCode::DatabaseError, mysql_error(conn));
    }

    MYSQL_RES* result = mysql_store_result(conn);
    int count = 0;
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row && row[0]) count = std::stoi(row[0]);
        mysql_free_result(result);
    }
    m_pool->returnConnection(conn);
    return Result<int>::ok(count);
}

Result<uint64_t> MySqlRepository::saveDanmaku(uint64_t roomId, uint64_t userId,
                                                const std::string& content) {
    auto connResult = m_pool->getConnection();
    if (!connResult.isOk()) return Result<uint64_t>::fail(ErrorCode::DatabaseError, "No DB connection");

    MYSQL* conn = connResult.value();
    std::string escaped = escape(content, conn);
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "INSERT INTO danmaku_messages (room_id, user_id, username, content, color, type) VALUES (%lu, %lu, 'user', '%s', '#00ff41', 'normal')",
        roomId, userId, escaped.c_str());

    if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
        m_pool->returnConnection(conn);
        return Result<uint64_t>::fail(ErrorCode::DatabaseError, mysql_error(conn));
    }

    uint64_t id = mysql_insert_id(conn);
    m_pool->returnConnection(conn);
    return Result<uint64_t>::ok(id);
}

Result<std::vector<service::DanmakuRecord>> MySqlRepository::queryRecentDanmaku(uint64_t roomId, int count) {
    auto connResult = m_pool->getConnection();
    if (!connResult.isOk()) return Result<std::vector<service::DanmakuRecord>>::fail(ErrorCode::DatabaseError, "No DB connection");

    MYSQL* conn = connResult.value();
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT id, room_id, user_id, username, content, type, created_at FROM danmaku_messages WHERE room_id=%lu ORDER BY created_at DESC LIMIT %d",
        roomId, count);

    if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
        m_pool->returnConnection(conn);
        return Result<std::vector<service::DanmakuRecord>>::fail(ErrorCode::DatabaseError, mysql_error(conn));
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) { m_pool->returnConnection(conn); return Result<std::vector<service::DanmakuRecord>>::ok({}); }

    std::vector<service::DanmakuRecord> records;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        service::DanmakuRecord r;
        r.id = row[0] ? std::stoull(row[0]) : 0;
        r.roomId = row[1] ? std::stoull(row[1]) : 0;
        r.userId = row[2] ? std::stoull(row[2]) : 0;
        r.userName = row[3] ? row[3] : "";
        r.content = row[4] ? row[4] : "";
        r.contentStatus = 0;
        r.createdAt = row[6] ? row[6] : "";
        records.push_back(r);
    }

    mysql_free_result(result);
    m_pool->returnConnection(conn);
    return Result<std::vector<service::DanmakuRecord>>::ok(std::move(records));
}

} // namespace chatroom
