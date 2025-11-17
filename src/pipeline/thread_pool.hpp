#pragma once

#include <atomic>
#include <thread>
#include <vector>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace pipeline {

class ThreadPool {
public:
    ThreadPool(uint32_t numThreads);
    ~ThreadPool();

    void start();
    void stop();
    
    template<typename Func>
    void enqueue(Func&& task);
    
    // Pin thread to specific CPU core
    bool pinThreadToCore(uint32_t threadId, uint32_t coreId);
    
    // Set thread priority (real-time)
    bool setThreadPriority(uint32_t threadId, int priority);

private:
    void workerThread(uint32_t threadId);
    
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_tasks;
    
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    
    std::atomic<bool> m_running{false};
    uint32_t m_numThreads;
};

template<typename Func>
void ThreadPool::enqueue(Func&& task) {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_tasks.emplace(std::forward<Func>(task));
    }
    m_condition.notify_one();
}

} // namespace pipeline
