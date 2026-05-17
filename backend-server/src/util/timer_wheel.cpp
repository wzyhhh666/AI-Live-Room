#include "util/timer_wheel.h"
#include "common/logging.h"
#include <algorithm>

namespace chatroom {
namespace util {

TimerWheel::TimerWheel(size_t slotCount, int tickIntervalMs)
    : m_slotCount(slotCount)
    , m_tickIntervalMs(tickIntervalMs)
    , m_currentSlot(0) {
    
    m_slots.resize(slotCount);
    LOG_INFO("TimerWheel: initialized with {} slots, {}ms tick interval", 
             slotCount, tickIntervalMs);
}

TimerWheel::~TimerWheel() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& slot : m_slots) {
        slot.clear();
    }
    LOG_DEBUG("TimerWheel: destroyed");
}

uint64_t TimerWheel::addTimer(int timeoutMs, TimerCallback callback) {
    if (timeoutMs <= 0 || !callback) {
        return 0;
    }
    
    uint64_t taskId = m_nextTaskId.fetch_add(1);
    
    size_t ticksNeeded = static_cast<size_t>(timeoutMs / m_tickIntervalMs);
    if (ticksNeeded == 0) ticksNeeded = 1;
    
    size_t rotation = ticksNeeded / m_slotCount;
    size_t slotIndex = (m_currentSlot + ticksNeeded) % m_slotCount;
    
    TimerTask task;
    task.taskId = taskId;
    task.callback = std::move(callback);
    task.rotation = static_cast<int>(rotation);
    task.slotIndex = slotIndex;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_slots[slotIndex].push_back(task);
    }
    
    LOG_DEBUG("TimerWheel: added timer {} timeout={}ms ticks={} slot={}", 
             taskId, timeoutMs, ticksNeeded, slotIndex);
    
    return taskId;
}

bool TimerWheel::cancelTimer(uint64_t taskId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& slot : m_slots) {
        auto it = std::find_if(slot.begin(), slot.end(), 
            [taskId](const TimerTask& t) { return t.taskId == taskId; });
        
        if (it != slot.end()) {
            slot.erase(it);
            LOG_DEBUG("TimerWheel: cancelled timer {}", taskId);
            return true;
        }
    }
    
    return false;
}

void TimerWheel::tick() {
    std::vector<TimerTask> readyTasks;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto& currentSlot = m_slots[m_currentSlot];
        
        for (auto it = currentSlot.begin(); it != currentSlot.end(); ) {
            if (it->rotation > 0) {
                it->rotation--;
                ++it;
            } else {
                readyTasks.push_back(std::move(*it));
                it = currentSlot.erase(it);
            }
        }
        
        m_currentSlot = (m_currentSlot + 1) % m_slotCount;
    }
    
    for (auto& task : readyTasks) {
        try {
            if (task.callback) {
                task.callback();
            }
        } catch (const std::exception& e) {
            LOG_ERROR("TimerWheel: timer {} callback exception: {}", 
                     task.taskId, e.what());
        } catch (...) {
            LOG_ERROR("TimerWheel: timer {} unknown exception", task.taskId);
        }
    }
    
    if (!readyTasks.empty()) {
        LOG_TRACE("TimerWheel: tick processed {} timers", readyTasks.size());
    }
}

size_t TimerWheel::getPendingTimerCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& slot : m_slots) {
        count += slot.size();
    }
    return count;
}

} // namespace util
} // namespace chatroom
