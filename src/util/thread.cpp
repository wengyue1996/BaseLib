#include "util/thread.h"
#include <algorithm>
#include <sstream>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <processthreadsapi.h>
    typedef HANDLE ThreadHandle;
    typedef DWORD ThreadHandleId;
#else
    #include <pthread.h>
    #include <unistd.h>
    #include <sys/syscall.h>
    #include <sys/time.h>
    typedef pthread_t ThreadHandle;
    typedef pid_t ThreadHandleId;
#endif

namespace base {
namespace util {

static Thread::ThreadId generateThreadId() {
    static std::atomic<Thread::ThreadId> id_counter(0);
    return ++id_counter;
}

struct Thread::Impl {
    ThreadOptions options;
    RunCallback run_callback;
    CleanupCallback cleanup_callback;
    std::thread native_thread;
    ThreadState state;
    std::mutex state_mutex;
    std::condition_variable state_cv;
    ThreadId thread_id;
    std::atomic<bool> should_stop;
    std::atomic<bool> should_pause;
    std::atomic<uint64_t> start_time;
    std::atomic<uint64_t> uptime;

#if defined(_WIN32) || defined(_WIN64)
    HANDLE os_handle;
#endif

    Impl(const ThreadOptions& opts, RunCallback run, CleanupCallback cleanup)
        : options(opts)
        , run_callback(std::move(run))
        , cleanup_callback(std::move(cleanup))
        , state(ThreadState::Idle)
        , thread_id(generateThreadId())
        , should_stop(false)
        , should_pause(false)
        , start_time(0)
        , uptime(0)
#if defined(_WIN32) || defined(_WIN64)
        , os_handle(nullptr)
#endif
    {
        if (options.name.empty()) {
            std::ostringstream oss;
            oss << "Thread-" << thread_id;
            options.name = oss.str();
        }
    }

    ~Impl() {
        if (native_thread.joinable()) {
            should_stop = true;
            should_pause = false;
            state_cv.notify_one();
            native_thread.join();
        }
        cleanup();
    }

    void cleanup() {
        if (cleanup_callback) {
            try {
                cleanup_callback();
            } catch (...) {
            }
        }
    }

    void setOsPriority(int os_priority) {
#if defined(_WIN32) || defined(_WIN64)
        if (os_handle) {
            SetThreadPriority(os_handle, os_priority);
        }
#else
        // POSIX: pthread_setschedparam would be used here
        (void)os_priority;
#endif
    }

    int getOsPriority() const {
#if defined(_WIN32) || defined(_WIN64)
        if (os_handle) {
            return GetThreadPriority(os_handle);
        }
        return THREAD_PRIORITY_NORMAL;
#else
        return 0;
#endif
    }
};

Thread::Thread()
    : m_impl(std::make_unique<Impl>(ThreadOptions(), nullptr, nullptr)) {
}

Thread::Thread(RunCallback run)
    : m_impl(std::make_unique<Impl>(ThreadOptions(), std::move(run), nullptr)) {
}

Thread::Thread(const ThreadOptions& options, RunCallback run, CleanupCallback cleanup)
    : m_impl(std::make_unique<Impl>(options, std::move(run), std::move(cleanup))) {
}

Thread::~Thread() = default;

Thread::Thread(Thread&& other) noexcept
    : m_impl(std::move(other.m_impl)) {
}

Thread& Thread::operator=(Thread&& other) noexcept {
    if (this != &other) {
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

Result<void> Thread::start() {
    std::lock_guard<std::mutex> lock(m_impl->state_mutex);
    
    if (m_impl->state == ThreadState::Running) {
        return Result<void>::failure(ErrorCode::INVALID_STATE);
    }
    
    if (m_impl->state == ThreadState::Starting) {
        return Result<void>::failure(ErrorCode::INVALID_STATE);
    }

    m_impl->should_stop = false;
    m_impl->should_pause = false;
    setState(ThreadState::Starting);

    try {
        m_impl->native_thread = std::thread([this]() {
            this->runLoop();
        });
        
#if defined(_WIN32) || defined(_WIN64)
        m_impl->os_handle = m_impl->native_thread.native_handle();
#endif

        if (m_impl->options.priority != ThreadPriority::Normal) {
            setPriority(m_impl->options.priority);
        }

        setState(ThreadState::Running);
        m_impl->start_time = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        
        return Result<void>::success();
    } catch (...) {
        setState(ThreadState::Error);
        return Result<void>::failure(ErrorCode::UNKNOWN_ERROR);
    }
}

void Thread::runLoop() {
    if (m_impl->run_callback) {
        try {
            m_impl->run_callback();
        } catch (...) {
        }
    }
    
    std::lock_guard<std::mutex> lock(m_impl->state_mutex);
    if (m_impl->state != ThreadState::Stopping) {
        setState(ThreadState::Stopped);
    }
}

Result<void> Thread::stop(int timeout_ms) {
    std::unique_lock<std::mutex> lock(m_impl->state_mutex);
    
    if (m_impl->state != ThreadState::Running && 
        m_impl->state != ThreadState::Paused) {
        return Result<void>::failure(ErrorCode::INVALID_STATE);
    }

    setState(ThreadState::Stopping);
    m_impl->should_stop = true;
    m_impl->should_pause = false;
    m_impl->state_cv.notify_one();

    lock.unlock();

    if (m_impl->native_thread.joinable()) {
        if (m_impl->native_thread.get_id() == std::this_thread::get_id()) {
            return Result<void>::failure(ErrorCode::DEADLOCK);
        }
        
        try {
            m_impl->native_thread.join();
        } catch (...) {
            return Result<void>::failure(ErrorCode::UNKNOWN_ERROR);
        }
    }

    lock.lock();
    setState(ThreadState::Stopped);
    m_impl->uptime = 0;
    
    return Result<void>::success();
}

Result<void> Thread::pause() {
    std::lock_guard<std::mutex> lock(m_impl->state_mutex);
    
    if (m_impl->state != ThreadState::Running) {
        return Result<void>::failure(ErrorCode::INVALID_STATE);
    }

    m_impl->should_pause = true;
    setState(ThreadState::Paused);
    
    return Result<void>::success();
}

Result<void> Thread::resume() {
    std::lock_guard<std::mutex> lock(m_impl->state_mutex);
    
    if (m_impl->state != ThreadState::Paused) {
        return Result<void>::failure(ErrorCode::INVALID_STATE);
    }

    m_impl->should_pause = false;
    m_impl->state_cv.notify_one();
    setState(ThreadState::Running);
    
    return Result<void>::success();
}

Result<void> Thread::join() {
    if (m_impl->native_thread.get_id() == std::this_thread::get_id()) {
        return Result<void>::failure(ErrorCode::DEADLOCK);
    }

    try {
        if (m_impl->native_thread.joinable()) {
            m_impl->native_thread.join();
        }
        return Result<void>::success();
    } catch (...) {
        return Result<void>::failure(ErrorCode::UNKNOWN_ERROR);
    }
}

void Thread::setPriority(ThreadPriority priority) {
    m_impl->options.priority = priority;
    
#if defined(_WIN32) || defined(_WIN64)
    int os_priority = THREAD_PRIORITY_NORMAL;
    switch (priority) {
        case ThreadPriority::Lowest:
            os_priority = THREAD_PRIORITY_IDLE;
            break;
        case ThreadPriority::BelowNormal:
            os_priority = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        case ThreadPriority::Normal:
            os_priority = THREAD_PRIORITY_NORMAL;
            break;
        case ThreadPriority::AboveNormal:
            os_priority = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
        case ThreadPriority::Highest:
            os_priority = THREAD_PRIORITY_TIME_CRITICAL;
            break;
    }
    m_impl->setOsPriority(os_priority);
#endif
}

bool Thread::setPriority(int os_priority) {
#if defined(_WIN32) || defined(_WIN64)
    if (m_impl->os_handle) {
        return SetThreadPriority(m_impl->os_handle, os_priority) != 0;
    }
#endif
    return false;
}

ThreadPriority Thread::getPriority() const {
    return m_impl->options.priority;
}

void Thread::setName(const std::string& name) {
    m_impl->options.name = name;
#if defined(_WIN32) || defined(_WIN64)
    if (m_impl->os_handle) {
        using SetThreadDescription_t = HRESULT(WINAPI*)(HANDLE, PCWSTR);
        static auto SetThreadDescription = (SetThreadDescription_t)(
            GetProcAddress(GetModuleHandleA("kernel32.dll"), "SetThreadDescription"));
        if (SetThreadDescription) {
            std::wstring wname(name.begin(), name.end());
            SetThreadDescription(m_impl->os_handle, wname.c_str());
        }
    }
#else
    pthread_setname_np(m_impl->native_thread.native_handle(), name.c_str());
#endif
}

std::string Thread::getName() const {
    return m_impl->options.name;
}

Thread::ThreadId Thread::getId() const {
    return m_impl->thread_id;
}

ThreadState Thread::getState() const {
    std::lock_guard<std::mutex> lock(m_impl->state_mutex);
    return m_impl->state;
}

bool Thread::isRunning() const {
    std::lock_guard<std::mutex> lock(m_impl->state_mutex);
    return m_impl->state == ThreadState::Running;
}

bool Thread::isJoinable() const {
    return m_impl->native_thread.joinable();
}

uint64_t Thread::getUptime() const {
    if (m_impl->start_time == 0) {
        return 0;
    }
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    return static_cast<uint64_t>(now - m_impl->start_time);
}

void Thread::setState(ThreadState state) {
    m_impl->state = state;
    m_impl->state_cv.notify_all();
}

bool Thread::waitForState(ThreadState state, int timeout_ms) {
    std::unique_lock<std::mutex> lock(m_impl->state_mutex);
    return m_impl->state_cv.wait_for(lock,
        std::chrono::milliseconds(timeout_ms),
        [this, state]() { return m_impl->state == state; });
}

Thread::ThreadId Thread::getCurrentThreadId() {
#if defined(_WIN32) || defined(_WIN64)
    return GetCurrentThreadId();
#else
    return syscall(SYS_gettid);
#endif
}

std::string Thread::getCurrentThreadName() {
#if defined(_WIN32) || defined(_WIN64)
    return "MainThread";
#else
    char name[16] = {0};
    pthread_getname_np(pthread_self(), name, sizeof(name));
    return name;
#endif
}

void Thread::sleep(int milliseconds) {
#if defined(_WIN32) || defined(_WIN64)
    ::Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

void Thread::yield() {
#if defined(_WIN32) || defined(_WIN64)
    SwitchToThread();
#else
    sched_yield();
#endif
}

ThreadGuard::ThreadGuard(Thread&& t) : m_thread(std::move(t)) {}

ThreadGuard::~ThreadGuard() {
    if (m_thread.isRunning()) {
        m_thread.stop();
    }
}

ThreadGuard::ThreadGuard(ThreadGuard&& other) noexcept
    : m_thread(std::move(other.m_thread)) {
}

ThreadGuard& ThreadGuard::operator=(ThreadGuard&& other) noexcept {
    if (this != &other) {
        m_thread = std::move(other.m_thread);
    }
    return *this;
}

} // namespace util
} // namespace base
