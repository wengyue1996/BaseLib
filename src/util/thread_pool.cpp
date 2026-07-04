#include "../include/util/thread_pool.h"

namespace base {
namespace util {

ThreadPool::ThreadPool(size_t threadCount)
    : m_running(false), m_paused(false), m_thread_count(threadCount), m_active_tasks(0) {
    if (m_thread_count == 0) m_thread_count = 1;
    m_running = true;
    for (size_t i = 0; i < m_thread_count; ++i) {
        m_threads.emplace_back(&ThreadPool::workerThread, this);
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = false;
        m_paused = false;
    }
    m_condition.notify_all();
    for (auto& thread : m_threads) {
        if (thread.joinable()) thread.join();
    }
    m_threads.clear();
}

bool ThreadPool::isRunning() const {
    return m_running;
}

size_t ThreadPool::getThreadCount() const {
    return m_thread_count;
}

size_t ThreadPool::getTaskCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tasks.size();
}

void ThreadPool::pause() {
    m_paused = true;
}

void ThreadPool::resume() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_paused = false;
    }
    m_condition.notify_all();
}

bool ThreadPool::isPaused() const {
    return m_paused;
}

void ThreadPool::waitAll() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_done_condition.wait(lock, [this] {
        return m_tasks.empty() && m_active_tasks == 0;
    });
}

void ThreadPool::workerThread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this] {
                return !m_running || (!m_tasks.empty() && !m_paused);
            });
            if (!m_running && m_tasks.empty()) return;
            if (!m_tasks.empty()) {
                task = std::move(m_tasks.front());
                m_tasks.pop();
                m_active_tasks++;
            }
        }
        if (task) {
            task();
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_active_tasks--;
            }
            m_done_condition.notify_all();
        }
    }
}

} // namespace util
} // namespace base