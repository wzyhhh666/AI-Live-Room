#pragma once

#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include <atomic>

namespace chatroom {
namespace util{

using TimerCallback = std::function<void()>;

struct TimerTask {
    uint64_t taskId;
    TimerCallback callback;
    int rotation;
    size_t slotIndex;
};

class TimerWheel {
public:
    explicit TimerWheel(size_t slotCount = 1024, int tickIntervalMs = 100);
    ~TimerWheel();
    
    TimerWheel(const TimerWheel&) = delete;
    TimerWheel& operator=(const TimerWheel&) = delete;
    
    uint64_t addTimer(int timeoutMs, TimerCallback callback);
    bool cancelTimer(uint64_t taskId);
    void tick();
    
    size_t getPendingTimerCount() const;

private:
    std::vector<std::vector<TimerTask>> m_slots;
    size_t m_currentSlot = 0;
    mutable std::mutex m_mutex;
    
    std::atomic<uint64_t> m_nextTaskId{1};
    int m_tickIntervalMs;
    size_t m_slotCount;
};

} // namespace util
} // namespace chatroom
