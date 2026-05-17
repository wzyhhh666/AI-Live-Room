#include "util/thread_pool.h"
#include "common/logging.h"

namespace chatroom {
namespace util {

ThreadPool::ThreadPool(size_t threadCount) 
    : m_stop(false), m_stopNow(false) {
    if (threadCount == 0) {
        threadCount = std::thread::hardware_concurrency() * 2;
    }
    
    LOG_INFO("ThreadPool: creating {} worker threads", threadCount);
    
    m_workers.reserve(threadCount);
    for (size_t i = 0; i < threadCount; ++i) {
        m_workers.emplace_back(&ThreadPool::workerLoop, this, static_cast<uint32_t>(i));
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::workerLoop(uint32_t workerId) {
    LOG_DEBUG("ThreadPool: worker {} started", workerId);
    
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait(lock, [this] {
                return m_stop || m_stopNow || !m_taskQueue.empty();
            });
            
            if (m_stop && m_taskQueue.empty()) {
                LOG_DEBUG("ThreadPool: worker {} exiting (normal shutdown)", workerId);
                return;
            }
            
            if (m_stopNow && !m_taskQueue.empty()) {
                std::queue<Task> empty;
                std::swap(m_taskQueue, empty);
                lock.unlock();
                continue;
            }
            
            if (!m_taskQueue.empty()) {
                task = std::move(m_taskQueue.front());
                m_taskQueue.pop();
            }
        }
        
        if (task.callback) {
            try {
                task.callback();
            } catch (const std::exception& e) {
                LOG_ERROR("ThreadPool: task {} threw exception: {}", task.taskId, e.what());
            } catch (...) {
                LOG_ERROR("ThreadPool: task {} threw unknown exception", task.taskId);
            }
        }
    }
}

Result<uint64_t> ThreadPool::submit(Task task) {
    if (m_stop || m_stopNow) {
        return Result<uint64_t>::fail(ErrorCode::InternalError, "ThreadPool is shutting down");
    }
    
    uint64_t taskId = m_nextTaskId.fetch_add(1);
    task.taskId = taskId;
    
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        
        if (m_taskQueue.size() >= m_maxQueueSize) {
            LOG_WARN("ThreadPool: queue full, rejecting task {}", taskId);
            return Result<uint64_t>::fail(ErrorCode::InternalError, "Task queue full");
        }
        
        m_taskQueue.push(std::move(task));
    }
    
    m_condition.notify_one();
    return Result<uint64_t>::ok(taskId);
}

Result<uint64_t> ThreadPool::submit(std::function<void()> callback,
                                        uint64_t roomId, uint64_t userId) {
    Task task(std::move(callback), 0, roomId, userId);
    return submit(std::move(task));
}

size_t ThreadPool::getPendingTaskCount() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_taskQueue.size();
}

size_t ThreadPool::getThreadCount() const {
    return m_workers.size();
}

bool ThreadPool::isRunning() const {
    return !m_stop && !m_stopNow;
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_stop = true;
    }
    m_condition.notify_all();
    
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    LOG_INFO("ThreadPool: shutdown complete");
}

void ThreadPool::shutdownNow() {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_stopNow = true;
    }
    m_condition.notify_all();
    
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    LOG_INFO("ThreadPool: immediate shutdown complete");
}

} // namespace util
} // namespace chatroom
