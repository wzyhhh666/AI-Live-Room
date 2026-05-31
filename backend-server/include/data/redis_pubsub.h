#pragma once

#include <string>
#include <cstdint>
#include <functional>
#include <thread>
#include <atomic>
#include <hiredis/hiredis.h>

namespace chatroom {

struct DanmakuOutput {
    uint64_t id = 0;
    uint64_t room_id = 0;
    uint64_t user_id = 0;
    std::string username;
    std::string content;
    std::string color;
    std::string type = "normal";
    std::string created_at;
};

struct GiftOutput {
    uint64_t record_id = 0;
    uint64_t room_id = 0;
    uint64_t sender_id = 0;
    std::string sender_name;
    int gift_id = 0;
    std::string gift_name;
    int gift_count = 1;
    double total_price = 0.0;
    std::string effect_type = "normal";
    std::string created_at;
};

class RedisPublisher {
public:
    RedisPublisher() = default;
    ~RedisPublisher();

    bool init(const std::string& host, int port, const std::string& password = "");
    bool isConnected() const;

    bool publish(const std::string& channel, const std::string& message);

    bool publishDanmaku(const DanmakuOutput& record);
    bool publishGift(const GiftOutput& record);
    bool publishBlocked(uint64_t roomId, uint64_t userId,
                        const std::string& original, const std::string& matchedWords);
    bool publishPresence(uint64_t roomId, int onlineCount);

private:
    redisContext* m_context = nullptr;
};

using InputMessageCallback = std::function<void(const std::string& channel, const std::string& message)>;

class RedisSubscriber {
public:
    RedisSubscriber() = default;
    ~RedisSubscriber();

    bool init(const std::string& host, int port, const std::string& password = "");
    bool subscribe(const std::string& channel);
    bool unsubscribe(const std::string& channel);
    bool start();
    void stop();
    bool isRunning() const { return m_running.load(); }

    void setMessageCallback(InputMessageCallback cb) { m_callback = std::move(cb); }

private:
    void listenLoop();

    redisContext* m_context = nullptr;
    std::atomic<bool> m_running{false};
    std::thread m_listenThread;
    InputMessageCallback m_callback;
};

} // namespace chatroom
