#include "common/result.h"
#include "common/logging.h"
#include "util/config_manager.h"
#include "data/mysql_pool.h"
#include "data/redis_pool.h"
#include "data/redis_pubsub.h"
#include "data/mysql_repository.h"
#include "service/filter_service.h"
#include "service/danmaku_service.h"
#include "net/epoll_server.h"
#include "net/connection.h"
#include "protocol/message_dispatcher.h"
#include "protocol/message_codec.h"
#include "protocol/message_header.h"
#include <nlohmann/json.hpp>
#include <csignal>
#include <memory>
#include <thread>
#include <iostream>

using namespace chatroom;

std::atomic<bool> g_running{true};

void signalHandler(int) {
    g_running = false;
}

static spdlog::level::level_enum parseLogLevel(const std::string& level) {
    if (level == "TRACE") return spdlog::level::trace;
    if (level == "DEBUG") return spdlog::level::debug;
    if (level == "WARN")  return spdlog::level::warn;
    if (level == "ERROR") return spdlog::level::err;
    if (level == "CRITICAL") return spdlog::level::critical;
    return spdlog::level::info;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    std::string configPath = (argc > 1) ? argv[1] : "config/app.json";

    auto& configMgr = ConfigManager::getInstance();
    auto loadResult = configMgr.loadFromFile(configPath);
    if (!loadResult.isOk()) {
        std::cerr << "Failed to load config: " << loadResult.message() << std::endl;
        return 1;
    }
    auto& cfg = configMgr.getConfig();

    Logger::getInstance().initialize(
        cfg.log.filePath,
        parseLogLevel(cfg.log.level),
        spdlog::level::debug,
        static_cast<size_t>(cfg.log.maxFileSizeMb) * 1024 * 1024,
        static_cast<size_t>(cfg.log.maxFiles)
    );

    LOG_INFO("Starting chatroom-server v1.0");
    LOG_INFO("Config: {}:{}", cfg.server.host, cfg.server.port);

    data::MySqlConfig mysqlCfg;
    mysqlCfg.host = cfg.mysql.host;
    mysqlCfg.port = cfg.mysql.port;
    mysqlCfg.user = cfg.mysql.user;
    mysqlCfg.password = cfg.mysql.password;
    mysqlCfg.database = cfg.mysql.database;
    mysqlCfg.poolMin = static_cast<size_t>(cfg.mysql.poolMin);
    mysqlCfg.poolMax = static_cast<size_t>(cfg.mysql.poolMax);

    auto& mysqlPool = data::MySqlPool::getInstance();
    auto mysqlInitResult = mysqlPool.initialize(mysqlCfg);
    if (!mysqlInitResult.isOk()) {
        LOG_CRITICAL("Failed to init MySQL pool: {}", mysqlInitResult.message());
        return 1;
    }
    LOG_INFO("MySQL pool initialized (min={} max={})", cfg.mysql.poolMin, cfg.mysql.poolMax);

    auto redisPublisher = std::make_shared<RedisPublisher>();
    if (!redisPublisher->init(cfg.redis.host, cfg.redis.port, cfg.redis.password)) {
        LOG_WARN("RedisPublisher init failed (non-fatal, continuing)");
    } else {
        LOG_INFO("RedisPublisher connected to {}:{}", cfg.redis.host, cfg.redis.port);
    }

    auto repository = std::make_shared<MySqlRepository>(
        std::shared_ptr<data::MySqlPool>(&mysqlPool, [](data::MySqlPool*) {})
    );

    auto filterService = std::make_shared<service::FilterService>();
    auto filterInitResult = filterService->initialize(cfg.filter.wordDictPath);
    if (!filterInitResult.isOk()) {
        LOG_WARN("FilterService init failed: {} (continuing without filter)", filterInitResult.message());
    } else {
        LOG_INFO("FilterService loaded ({} words)", filterService->getDictionarySize());
    }

    LOG_INFO("Creating DanmakuService...");
    auto danmakuService = std::make_shared<service::DanmakuService>(repository, filterService);
    LOG_INFO("Creating RoomService...");
    auto roomService = std::make_shared<service::RoomService>(repository);
    LOG_INFO("Services created.");

    auto& dispatcherRef = protocol::MessageDispatcher::getInstance();
    auto dispatcher = std::shared_ptr<protocol::MessageDispatcher>(
        &dispatcherRef, [](protocol::MessageDispatcher*) {});

    dispatcher->registerHandler(static_cast<uint16_t>(protocol::MessageType::MSG_DANMAKU),
        [danmakuService, redisPublisher](int fd, protocol::Message& msg) -> Result<void> {
            try {
                auto bodyJson = nlohmann::json::parse(msg.body);
                uint64_t roomId = bodyJson["room_id"];
                uint64_t userId = bodyJson["user_id"];
                std::string userName = bodyJson.value("username", "unknown");
                std::string content = bodyJson["content"];
                std::string color = bodyJson.value("color", "#00ff41");

                auto result = danmakuService->processDanmaku(fd, roomId, userId, userName, content);

                if (redisPublisher && redisPublisher->isConnected()) {
                    DanmakuOutput output;
                    output.id = 0;
                    output.room_id = roomId;
                    output.user_id = userId;
                    output.username = userName;
                    output.content = content;
                    output.color = color;
                    output.type = "normal";
                    redisPublisher->publishDanmaku(output);
                }
            } catch (const std::exception& e) {
                LOG_WARN("Danmaku handler error: {}", e.what());
            }
            return Result<void>::ok();
        }
    );

    dispatcher->registerHandler(static_cast<uint16_t>(protocol::MessageType::MSG_GIFT),
        [redisPublisher](int fd, protocol::Message& msg) -> Result<void> {
            try {
                auto bodyJson = nlohmann::json::parse(msg.body);
                GiftOutput output;
                output.room_id = bodyJson["room_id"];
                output.sender_id = bodyJson["sender_id"];
                output.sender_name = bodyJson.value("sender_name", "unknown");
                output.gift_id = bodyJson["gift_id"];
                output.gift_name = bodyJson.value("gift_name", "🎁");
                output.gift_count = bodyJson.value("gift_count", 1);
                output.total_price = bodyJson.value("total_price", 0.0);
                output.effect_type = bodyJson.value("effect_type", "normal");

                if (redisPublisher && redisPublisher->isConnected()) {
                    redisPublisher->publishGift(output);
                }
            } catch (const std::exception& e) {
                LOG_WARN("Gift handler error: {}", e.what());
            }
            return Result<void>::ok();
        }
    );

    dispatcher->registerHandler(static_cast<uint16_t>(protocol::MessageType::MSG_HEARTBEAT),
        [](int fd, protocol::Message&) -> Result<void> {
            auto conn = net::ConnectionManager::getInstance().getConnection(fd);
            if (conn) {
                conn->updateHeartbeatTime();
            }
            return Result<void>::ok();
        }
    );

    // === Phase 2 P1: Redis Subscriber ===
    auto redisSubscriber = std::make_unique<RedisSubscriber>();
    if (redisSubscriber->init(cfg.redis.host, cfg.redis.port, cfg.redis.password)) {
        redisSubscriber->setMessageCallback(
            [danmakuService, redisPublisher](const std::string& channel, const std::string& message) {
                try {
                    auto json = nlohmann::json::parse(message);

                    if (channel == "danmaku:input") {
                        uint64_t roomId = json["room_id"];
                        uint64_t userId = json["user_id"];
                        std::string userName = json.value("username", "");
                        std::string content = json["content"];
                        std::string color = json.value("color", "#00ff41");

                        auto result = danmakuService->processDanmaku(0, roomId, userId, userName, content);
                        if (result.isOk()) {
                            DanmakuOutput output;
                            output.room_id = roomId;
                            output.user_id = userId;
                            output.username = userName;
                            output.content = content;
                            output.color = color;
                            output.type = "normal";
                            if (redisPublisher && redisPublisher->isConnected()) {
                                redisPublisher->publishDanmaku(output);
                            }
                        } else {
                            if (redisPublisher && redisPublisher->isConnected()) {
                                redisPublisher->publishBlocked(roomId, userId, content, "");
                            }
                        }
                    }
                    else if (channel == "gift:input") {
                        uint64_t roomId = json["room_id"];
                        uint64_t senderId = json["sender_id"];
                        std::string senderName = json.value("sender_name", "");
                        int giftId = json["gift_id"];
                        std::string giftName = json.value("gift_name", "");
                        int count = json.value("gift_count", 1);
                        double price = json.value("total_price", 0.0);
                        std::string effectType = json.value("effect_type", "normal");

                        LOG_INFO("RedisSubscriber: processing gift from user={} in room={}", senderName, roomId);

                        auto connResult = data::MySqlPool::getInstance().getConnection();
                        if (connResult.isOk()) {
                            MYSQL* conn = connResult.value();
                            char sql[2048];
                            char escapedName[256], escapedEffect[64], escapedGiftName[128];
                            mysql_real_escape_string(conn, escapedName, senderName.c_str(), senderName.size());
                            mysql_real_escape_string(conn, escapedEffect, effectType.c_str(), effectType.size());
                            mysql_real_escape_string(conn, escapedGiftName, giftName.c_str(), giftName.size());
                            snprintf(sql, sizeof(sql),
                                "INSERT INTO gift_record (room_id, sender_id, sender_name, receiver_id, gift_id, gift_name, gift_count, total_price, effect_type) "
                                "VALUES (%lu, %lu, '%s', 1, %d, '%s', %d, %.2f, '%s')",
                                roomId, senderId, escapedName, giftId, escapedGiftName, count, price, escapedEffect);
                            if (mysql_real_query(conn, sql, strlen(sql)) == 0) {
                                int recordId = static_cast<int>(mysql_insert_id(conn));
                                if (redisPublisher && redisPublisher->isConnected()) {
                                    GiftOutput output;
                                    output.record_id = recordId;
                                    output.room_id = roomId;
                                    output.sender_id = senderId;
                                    output.sender_name = senderName;
                                    output.gift_id = giftId;
                                    output.gift_name = giftName;
                                    output.gift_count = count;
                                    output.total_price = price;
                                    output.effect_type = effectType;
                                    redisPublisher->publishGift(output);
                                }
                            }
                            data::MySqlPool::getInstance().returnConnection(conn);
                        }
                    }
                } catch (const std::exception& e) {
                    LOG_ERROR("RedisSubscriber callback error: {}", e.what());
                }
            }
        );
        redisSubscriber->subscribe("danmaku:input");
        redisSubscriber->subscribe("gift:input");
        redisSubscriber->start();
        LOG_INFO("RedisSubscriber started, subscribed to danmaku:input and gift:input");
    } else {
        LOG_WARN("RedisSubscriber init failed (non-fatal, continuing without input listener)");
    }

    auto server = std::make_shared<net::EpollServer>();
    server->setDispatcher(dispatcher);

    auto initResult = server->init(cfg.server.host, cfg.server.port, cfg.server.backlog);
    if (!initResult.isOk()) {
        LOG_CRITICAL("Failed to init server: {}", initResult.message());
        return 1;
    }

    LOG_INFO("Server listening on {}:{}", cfg.server.host, cfg.server.port);

    server->start();

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO("Shutting down...");
    redisSubscriber->stop();
    server->stop();
    LOG_INFO("Server stopped");

    return 0;
}
