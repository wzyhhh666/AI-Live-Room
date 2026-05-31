#include "util/config_manager.h"
#include "common/logging.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace chatroom {

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

Result<void> ConfigManager::loadFromFile(const std::string& configPath) {
    m_configPath = configPath;

    try {
        std::ifstream file(configPath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open config file: {}", configPath);
            return VoidResult::fail(ErrorCode::ConfigError, 
                "Cannot open config file: " + configPath);
        }

        nlohmann::json json;
        file >> json;

        if (json.contains("server")) {
            auto& s = json["server"];
            if (s.contains("host")) m_config.server.host = s["host"];
            if (s.contains("port")) m_config.server.port = s["port"];
            if (s.contains("backlog")) m_config.server.backlog = s["backlog"];
        }

        if (json.contains("mysql")) {
            auto& m = json["mysql"];
            if (m.contains("host")) m_config.mysql.host = m["host"];
            if (m.contains("port")) m_config.mysql.port = m["port"];
            if (m.contains("user")) m_config.mysql.user = m["user"];
            if (m.contains("password")) m_config.mysql.password = m["password"];
            if (m.contains("database")) m_config.mysql.database = m["database"];
            if (m.contains("pool_min")) m_config.mysql.poolMin = m["pool_min"];
            if (m.contains("pool_max")) m_config.mysql.poolMax = m["pool_max"];
        }

        if (json.contains("redis")) {
            auto& r = json["redis"];
            if (r.contains("host")) m_config.redis.host = r["host"];
            if (r.contains("port")) m_config.redis.port = r["port"];
            if (r.contains("password")) m_config.redis.password = r["password"];
            if (r.contains("db")) m_config.redis.db = r["db"];
            if (r.contains("pool_min")) m_config.redis.poolMin = r["pool_min"];
            if (r.contains("pool_max")) m_config.redis.poolMax = r["pool_max"];
        }

        if (json.contains("heartbeat")) {
            auto& h = json["heartbeat"];
            if (h.contains("client_interval_sec")) 
                m_config.heartbeat.clientIntervalSec = h["client_interval_sec"];
            if (h.contains("server_timeout_sec")) 
                m_config.heartbeat.serverTimeoutSec = h["server_timeout_sec"];
            if (h.contains("max_miss_count")) 
                m_config.heartbeat.maxMissCount = h["max_miss_count"];
        }

        if (json.contains("rate_limit")) {
            auto& rl = json["rate_limit"];
            if (rl.contains("user_max_per_sec")) 
                m_config.rateLimit.userMaxPerSec = rl["user_max_per_sec"];
            if (rl.contains("room_max_per_sec")) 
                m_config.rateLimit.roomMaxPerSec = rl["room_max_per_sec"];
        }

        if (json.contains("room")) {
            auto& rm = json["room"];
            if (rm.contains("max_online")) 
                m_config.room.maxOnline = rm["max_online"];
            if (rm.contains("recent_danmaku_count")) 
                m_config.room.recentDanmakuCount = rm["recent_danmaku_count"];
            if (rm.contains("recent_danmaku_ttl_sec")) 
                m_config.room.recentDanmakuTtlSec = rm["recent_danmaku_ttl_sec"];
        }

        if (json.contains("filter")) {
            auto& ft = json["filter"];
            if (ft.contains("word_dict_path")) 
                m_config.filter.wordDictPath = ft["word_dict_path"];
            if (ft.contains("reload_interval_sec")) 
                m_config.filter.reloadIntervalSec = ft["reload_interval_sec"];
            if (ft.contains("max_text_length")) 
                m_config.filter.maxTextLength = ft["max_text_length"];
        }

        if (json.contains("log")) {
            auto& l = json["log"];
            if (l.contains("level")) m_config.log.level = l["level"];
            if (l.contains("file_path")) m_config.log.filePath = l["file_path"];
            if (l.contains("max_file_size_mb")) m_config.log.maxFileSizeMb = l["max_file_size_mb"];
            if (l.contains("max_files")) m_config.log.maxFiles = l["max_files"];
        }

        LOG_INFO("Config loaded successfully from {}", configPath);

        return VoidResult::ok();

    } catch (const nlohmann::json::parse_error& e) {
        LOG_ERROR("JSON parse error in config file {}: {}", configPath, e.what());
        return VoidResult::fail(ErrorCode::ConfigError, 
            "JSON parse error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load config: {}", e.what());
        return VoidResult::fail(ErrorCode::ConfigError, e.what());
    }
}

} // namespace chatroom
