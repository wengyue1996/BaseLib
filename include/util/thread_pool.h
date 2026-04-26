#include <functional>
#include <future>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace base {
namespace util {

class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();

    template <typename Func, typename... Args>
    auto submit(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>;

    void shutdown();
    bool isRunning() const;
    size_t getThreadCount() const;

private:
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_running;
    size_t m_thread_count;

    void workerThread();
};

template <typename Func, typename... Args>
auto ThreadPool::submit(Func&& func, Args&&... args) -> std::future<decltype(func(args...))> {
    auto task = std::make_shared<std::packaged_task<decltype(func(args...))()>>(
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
    );

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tasks.emplace([task]() { (*task)(); });
    }

    m_condition.notify_one();
    return task->get_future();
}

} // namespace util
} // namespace base