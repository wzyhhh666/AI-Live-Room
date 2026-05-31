#include "data/redis_pubsub.h"
#include "common/logging.h"
#include <nlohmann/json.hpp>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace chatroom {

RedisPublisher::~RedisPublisher() {
    if (m_context) {
        redisFree(m_context);
        m_context = nullptr;
    }
}

bool RedisPublisher::init(const std::string& host, int port, const std::string& password) {
    struct timeval timeout = {1, 500000};
    m_context = redisConnectWithTimeout(host.c_str(), port, timeout);

    if (!m_context || m_context->err) {
        LOG_ERROR("RedisPublisher: connection failed: {}",
                  m_context ? m_context->errstr : "unknown");
        if (m_context) {
            redisFree(m_context);
            m_context = nullptr;
        }
        return false;
    }

    if (!password.empty()) {
        auto* reply = (redisReply*)redisCommand(m_context, "AUTH %s", password.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            LOG_ERROR("RedisPublisher: AUTH failed: {}",
                      reply ? reply->str : "unknown");
            if (reply) freeReplyObject(reply);
            redisFree(m_context);
            m_context = nullptr;
            return false;
        }
        freeReplyObject(reply);
    }

    LOG_INFO("RedisPublisher: connected to {}:{}", host, port);
    return true;
}

bool RedisPublisher::isConnected() const {
    return m_context != nullptr && m_context->err == 0;
}

bool RedisPublisher::publish(const std::string& channel, const std::string& message) {
    if (!isConnected()) {
        LOG_WARN("RedisPublisher::publish: not connected");
        return false;
    }

    auto* reply = (redisReply*)redisCommand(m_context, "PUBLISH %s %s",
                                            channel.c_str(), message.c_str());

    if (!reply) {
        LOG_ERROR("RedisPublisher::publish: redisCommand returned null, connection may be broken");
        return false;
    }

    bool ok = (reply->type == REDIS_REPLY_INTEGER);
    if (!ok) {
        LOG_ERROR("RedisPublisher::publish: unexpected reply type {}", (int)reply->type);
    }

    freeReplyObject(reply);
    return ok;
}

static std::string timeToISO8601() {
    auto now = std::time(nullptr);
    std::tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &now);
#else
    gmtime_r(&now, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

bool RedisPublisher::publishDanmaku(const DanmakuOutput& record) {
    nlohmann::json j;
    j["id"] = record.id;
    j["room_id"] = record.room_id;
    j["user_id"] = record.user_id;
    j["username"] = record.username;
    j["content"] = record.content;
    j["color"] = record.color;
    j["type"] = record.type;
    j["created_at"] = record.created_at.empty() ? timeToISO8601() : record.created_at;

    LOG_DEBUG("RedisPublisher: publishDanmaku room={} user={} content={}",
              record.room_id, record.user_id, record.content.substr(0, 30));

    return publish("danmaku:output", j.dump());
}

bool RedisPublisher::publishGift(const GiftOutput& record) {
    nlohmann::json j;
    j["record_id"] = record.record_id;
    j["room_id"] = record.room_id;
    j["sender_id"] = record.sender_id;
    j["sender_name"] = record.sender_name;
    j["gift_id"] = record.gift_id;
    j["gift_name"] = record.gift_name;
    j["gift_count"] = record.gift_count;
    j["total_price"] = record.total_price;
    j["effect_type"] = record.effect_type;
    j["created_at"] = record.created_at.empty() ? timeToISO8601() : record.created_at;

    LOG_DEBUG("RedisPublisher: publishGift room={} gift={} count={}",
              record.room_id, record.gift_name, record.gift_count);

    return publish("gift:output", j.dump());
}

bool RedisPublisher::publishBlocked(uint64_t roomId, uint64_t userId,
                                     const std::string& original,
                                     const std::string& matchedWords) {
    nlohmann::json j;
    j["room_id"] = roomId;
    j["user_id"] = userId;
    j["original"] = original;
    j["matched_words"] = matchedWords;
    j["reason"] = "content_filtered";

    LOG_INFO("RedisPublisher: publishBlocked room={} user={}", roomId, userId);

    return publish("danmaku:blocked", j.dump());
}

bool RedisPublisher::publishPresence(uint64_t roomId, int onlineCount) {
    nlohmann::json j;
    j["room_id"] = roomId;
    j["count"] = onlineCount;

    return publish("presence:online", j.dump());
}

// === RedisSubscriber ===

RedisSubscriber::~RedisSubscriber() {
    stop();
}

bool RedisSubscriber::init(const std::string& host, int port, const std::string& password) {
    m_context = redisConnectWithTimeout(
        host.c_str(), port,
        {.tv_sec = 3, .tv_usec = 0}
    );
    if (!m_context || m_context->err) {
        if (m_context) {
            LOG_ERROR("RedisSubscriber connect failed: {}", m_context->errstr);
            redisFree(m_context);
            m_context = nullptr;
        } else {
            LOG_ERROR("RedisSubscriber connect failed: cannot allocate context");
        }
        return false;
    }

    if (!password.empty()) {
        redisReply* reply = (redisReply*)redisCommand(m_context, "AUTH %s", password.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            LOG_ERROR("RedisSubscriber auth failed: {}", reply ? reply->str : "no reply");
            freeReplyObject(reply);
            redisFree(m_context);
            m_context = nullptr;
            return false;
        }
        freeReplyObject(reply);
    }

    LOG_INFO("RedisSubscriber connected to {}:{}", host, port);
    return true;
}

bool RedisSubscriber::subscribe(const std::string& channel) {
    if (!m_context) return false;
    redisReply* reply = (redisReply*)redisCommand(m_context, "SUBSCRIBE %s", channel.c_str());
    if (!reply) {
        LOG_ERROR("RedisSubscriber SUBSCRIBE {} failed: no reply", channel);
        return false;
    }
    freeReplyObject(reply);
    LOG_INFO("RedisSubscriber subscribed to {}", channel);
    return true;
}

bool RedisSubscriber::unsubscribe(const std::string& channel) {
    if (!m_context) return false;
    redisReply* reply = (redisReply*)redisCommand(m_context, "UNSUBSCRIBE %s", channel.c_str());
    if (!reply) return false;
    freeReplyObject(reply);
    return true;
}

bool RedisSubscriber::start() {
    if (m_running.load()) return true;
    if (!m_context) return false;

    m_running.store(true);
    m_listenThread = std::thread(&RedisSubscriber::listenLoop, this);
    return true;
}

void RedisSubscriber::stop() {
    m_running.store(false);

    if (m_context) {
        redisFree(m_context);
        m_context = nullptr;
    }

    if (m_listenThread.joinable()) {
        m_listenThread.join();
    }
}

void RedisSubscriber::listenLoop() {
    while (m_running.load()) {
        redisReply* reply = nullptr;
        int status = redisGetReply(m_context, (void**)&reply);

        if (status != REDIS_OK) {
            if (m_running.load()) {
                LOG_WARN("RedisSubscriber: redisGetReply failed (connection lost)");
            }
            break;
        }

        if (reply && reply->type == REDIS_REPLY_ARRAY && reply->elements >= 3) {
            if (reply->element[0]->type == REDIS_REPLY_STRING &&
                reply->element[0]->str &&
                strcmp(reply->element[0]->str, "message") == 0) {

                std::string channel = reply->element[1]->str ? reply->element[1]->str : "";
                std::string message = reply->element[2]->str ? reply->element[2]->str : "";

                if (m_callback) {
                    m_callback(channel, message);
                }
            }
        }

        if (reply) {
            freeReplyObject(reply);
        }
    }

    LOG_INFO("RedisSubscriber listen loop ended");
}

} // namespace chatroom
