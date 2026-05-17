#pragma once

#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include "common/result.h"

namespace chatroom {
namespace util {

struct Task {
    std::function<void()> callback;
    uint64_t taskId = 0;
    uint64_t roomId = 0;
    uint64_t userId = 0;
    
    Task() = default;
    Task(std::function<void()> cb, uint64_t tid = 0, 
          uint64_t rid = 0, uint64_t uid = 0)
        : callback(std::move(cb)), taskId(tid), 
          roomId(rid), userId(uid) {}
};

class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = 0);
    ~ThreadPool();
    
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    
    Result<uint64_t> submit(Task task);
    Result<uint64_t> submit(std::function<void()> callback, 
                            uint64_t roomId = 0, uint64_t userId = 0);
    
    size_t getPendingTaskCount() const;
    size_t getThreadCount() const;
    bool isRunning() const;
    
    void shutdown();
    void shutdownNow();

private:
    void workerLoop(uint32_t workerId);
    
    std::vector<std::thread> m_workers;
    std::queue<Task> m_taskQueue;
    
    mutable std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_stop{false};
    std::atomic<bool> m_stopNow{false};
    
    std::atomic<uint64_t> m_nextTaskId{1};
    size_t m_maxQueueSize = 65536;
};

} // namespace util
} // namespace chatroom
