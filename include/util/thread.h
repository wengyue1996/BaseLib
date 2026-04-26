#ifndef BASE_THREAD_H
#define BASE_THREAD_H

#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>
#include "result.h"

namespace base {
namespace util {

enum class ThreadPriority {
    Lowest = 0,
    BelowNormal = 1,
    Normal = 2,
    AboveNormal = 3,
    Highest = 4
};

enum class ThreadState {
    Idle,
    Starting,
    Running,
    Paused,
    Stopping,
    Stopped,
    Error
};

struct ThreadOptions {
    std::string name;
    ThreadPriority priority = ThreadPriority::Normal;
    size_t stack_size = 0;
    bool detached = false;
};

class Thread {
public:
    using RunCallback = std::function<void()>;
    using CleanupCallback = std::function<void()>;
    using ThreadId = uint64_t;

    Thread();
    Thread(RunCallback run);
    Thread(const ThreadOptions& options, RunCallback run, CleanupCallback cleanup = nullptr);
    ~Thread();

    Thread(Thread&& other) noexcept;
    Thread& operator=(Thread&& other) noexcept;

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    Result<void> start();
    Result<void> stop(int timeout_ms = 5000);
    Result<void> pause();
    Result<void> resume();
    Result<void> join();

    void setPriority(ThreadPriority priority);
    bool setPriority(int os_priority);
    ThreadPriority getPriority() const;

    void setName(const std::string& name);
    std::string getName() const;

    ThreadId getId() const;
    ThreadState getState() const;

    bool isRunning() const;
    bool isJoinable() const;
    uint64_t getUptime() const;

    static ThreadId getCurrentThreadId();
    static std::string getCurrentThreadName();
    static void sleep(int milliseconds);
    static void yield();

private:
    void setState(ThreadState state);
    void runLoop();
    bool waitForState(ThreadState state, int timeout_ms);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class ThreadGuard {
public:
    explicit ThreadGuard(Thread&& t);
    ~ThreadGuard();

    ThreadGuard(ThreadGuard&& other) noexcept;
    ThreadGuard& operator=(ThreadGuard&& other) noexcept;

    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;

    Thread* get() { return &m_thread; }
    Thread* operator->() { return &m_thread; }

private:
    Thread m_thread;
};

class AutoJoin {
public:
    explicit AutoJoin(std::thread& t) : m_thread(t) {}
    ~AutoJoin() { if (m_thread.joinable()) m_thread.join(); }

    AutoJoin(const AutoJoin&) = delete;
    AutoJoin& operator=(const AutoJoin&) = delete;

private:
    std::thread& m_thread;
};

} // namespace util
} // namespace base

#endif // BASE_THREAD_H
