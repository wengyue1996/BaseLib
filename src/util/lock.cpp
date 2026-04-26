#include "util/lock.h"
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <mutex>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <pthread.h>
    #include <sys/time.h>
#endif

namespace base {
namespace util {

struct RecursiveMutex::Impl {
    std::string name;
    std::mutex mutex;
    std::atomic<int> lock_count;
    std::thread::id owner_thread_id;
    std::atomic<int> contention_count;

    Impl(const std::string& n)
        : name(n), lock_count(0), contention_count(0) {}
};

RecursiveMutex::RecursiveMutex(const std::string& name) 
    : m_impl(std::make_unique<Impl>(name.empty() ? "RecursiveMutex" : name)) {
}

RecursiveMutex::~RecursiveMutex() = default;

void RecursiveMutex::lock() {
    std::thread::id current_thread = std::this_thread::get_id();

    if (m_impl->owner_thread_id == current_thread) {
        m_impl->lock_count++;
        return;
    }

    m_impl->mutex.lock();
    m_impl->owner_thread_id = current_thread;
    m_impl->lock_count = 1;
}

bool RecursiveMutex::tryLock() {
    return tryLock(0);
}

bool RecursiveMutex::tryLock(int timeout_ms) {
    std::thread::id current_thread = std::this_thread::get_id();

    if (m_impl->owner_thread_id == current_thread) {
        m_impl->lock_count++;
        return true;
    }

    bool acquired = m_impl->mutex.try_lock();
    if (acquired) {
        m_impl->owner_thread_id = current_thread;
        m_impl->lock_count = 1;
        return true;
    }

    m_impl->contention_count++;
    return false;
}

void RecursiveMutex::unlock() {
    std::thread::id current_thread = std::this_thread::get_id();

    if (m_impl->owner_thread_id != current_thread) {
        return;
    }

    m_impl->lock_count--;
    if (m_impl->lock_count == 0) {
        m_impl->owner_thread_id = std::thread::id();
        m_impl->mutex.unlock();
    }
}

bool RecursiveMutex::isLocked() const {
    return m_impl->lock_count > 0;
}

int RecursiveMutex::getLockCount() const {
    return m_impl->lock_count;
}

std::string RecursiveMutex::getName() const {
    return m_impl->name;
}

struct NonRecursiveMutex::Impl {
    std::string name;
    std::mutex mutex;
    std::thread::id owner_thread_id;
    std::atomic<int> contention_count;

    Impl(const std::string& n)
        : name(n), contention_count(0) {}
};

NonRecursiveMutex::NonRecursiveMutex(const std::string& name) 
    : m_impl(std::make_unique<Impl>(name.empty() ? "NonRecursiveMutex" : name)) {
}

NonRecursiveMutex::~NonRecursiveMutex() = default;

void NonRecursiveMutex::lock() {
    std::thread::id current_thread = std::this_thread::get_id();

    if (m_impl->owner_thread_id == current_thread) {
        return;
    }

    m_impl->mutex.lock();
    m_impl->owner_thread_id = current_thread;
}

bool NonRecursiveMutex::tryLock() {
    return tryLock(0);
}

bool NonRecursiveMutex::tryLock(int timeout_ms) {
    std::thread::id current_thread = std::this_thread::get_id();

    if (m_impl->owner_thread_id == current_thread) {
        return true;
    }

    bool acquired = m_impl->mutex.try_lock();
    if (acquired) {
        m_impl->owner_thread_id = current_thread;
        return true;
    }

    m_impl->contention_count++;
    return false;
}

void NonRecursiveMutex::unlock() {
    std::thread::id current_thread = std::this_thread::get_id();

    if (m_impl->owner_thread_id != current_thread) {
        return;
    }

    m_impl->owner_thread_id = std::thread::id();
    m_impl->mutex.unlock();
}

bool NonRecursiveMutex::isLocked() const {
    return m_impl->owner_thread_id != std::thread::id();
}

int NonRecursiveMutex::getLockCount() const {
    return m_impl->owner_thread_id != std::thread::id() ? 1 : 0;
}

std::string NonRecursiveMutex::getName() const {
    return m_impl->name;
}

LockScope::LockScope(ILock* lock, LockScopeType type)
    : m_lock(lock), m_acquired(false), m_type(type) {
    if (m_lock) {
        m_lock->lock();
        m_acquired = true;
        m_lockName = m_lock->getName();
        LockMonitor::getInstance().recordLock(m_lockName, m_lock->getLockCount());
    }
}

LockScope::LockScope(ILock& lock, LockScopeType type)
    : LockScope(&lock, type) {
}

LockScope::~LockScope() {
    if (m_acquired && m_lock) {
        m_lock->unlock();
        LockMonitor::getInstance().recordUnlock(m_lockName);
    }
}

LockScope::LockScope(LockScope&& other) noexcept
    : m_lock(other.m_lock)
    , m_acquired(other.m_acquired)
    , m_type(other.m_type)
    , m_lockName(std::move(other.m_lockName)) {
    other.m_acquired = false;
}

LockScope& LockScope::operator=(LockScope&& other) noexcept {
    if (this != &other) {
        if (m_acquired && m_lock) {
            m_lock->unlock();
        }
        m_lock = other.m_lock;
        m_acquired = other.m_acquired;
        m_type = other.m_type;
        m_lockName = std::move(other.m_lockName);
        other.m_acquired = false;
    }
    return *this;
}

void LockScope::release() {
    if (m_acquired && m_lock) {
        m_lock->unlock();
        m_acquired = false;
        LockMonitor::getInstance().recordUnlock(m_lockName);
    }
}

bool LockScope::isAcquired() const {
    return m_acquired;
}

struct ReadWriteLock::Impl {
    std::string name;
    std::mutex read_mutex;
    std::mutex write_mutex;
    std::atomic<int> read_count;
    std::thread::id write_owner;
    std::atomic<int> read_waiters;
    std::atomic<int> write_waiters;

    Impl(const std::string& n)
        : name(n), read_count(0), read_waiters(0), write_waiters(0) {}
};

ReadWriteLock::ReadWriteLock(const std::string& name) 
    : m_impl(std::make_unique<Impl>(name.empty() ? "ReadWriteLock" : name)) {
}

ReadWriteLock::~ReadWriteLock() = default;

void ReadWriteLock::readLock() {
    std::lock_guard<std::mutex> lock(m_impl->read_mutex);
    m_impl->read_count++;
}

bool ReadWriteLock::tryReadLock() {
    std::lock_guard<std::mutex> lock(m_impl->read_mutex);
    m_impl->read_count++;
    return true;
}

bool ReadWriteLock::tryReadLock(int timeout_ms) {
    std::lock_guard<std::mutex> lock(m_impl->read_mutex);
    m_impl->read_count++;
    return true;
}

void ReadWriteLock::readUnlock() {
    std::lock_guard<std::mutex> lock(m_impl->read_mutex);
    if (m_impl->read_count > 0) {
        m_impl->read_count--;
    }
}

void ReadWriteLock::writeLock() {
    std::thread::id current_thread = std::this_thread::get_id();
    if (m_impl->write_owner == current_thread) {
        return;
    }
    m_impl->write_mutex.lock();
    m_impl->write_owner = current_thread;
}

bool ReadWriteLock::tryWriteLock() {
    return tryWriteLock(0);
}

bool ReadWriteLock::tryWriteLock(int timeout_ms) {
    std::thread::id current_thread = std::this_thread::get_id();
    if (m_impl->write_owner == current_thread) {
        return true;
    }

    bool acquired = m_impl->write_mutex.try_lock();
    if (acquired) {
        m_impl->write_owner = current_thread;
        return true;
    }
    return false;
}

void ReadWriteLock::writeUnlock() {
    std::thread::id current_thread = std::this_thread::get_id();
    if (m_impl->write_owner == current_thread) {
        m_impl->write_owner = std::thread::id();
        m_impl->write_mutex.unlock();
    }
}

bool ReadWriteLock::isReadLocked() const {
    return m_impl->read_count > 0;
}

bool ReadWriteLock::isWriteLocked() const {
    return m_impl->write_owner != std::thread::id();
}

int ReadWriteLock::getReadCount() const {
    return m_impl->read_count;
}

struct LockMonitor::Impl {
    std::unordered_map<std::string, int> lock_counts;
    std::unordered_map<std::string, int> contention_counts;
    std::mutex mutex;
    std::atomic<int> total_locks;
    std::atomic<int> total_contentions;

    Impl() : total_locks(0), total_contentions(0) {}
};

LockMonitor& LockMonitor::getInstance() {
    static LockMonitor instance;
    return instance;
}

void LockMonitor::recordLock(const std::string& name, int lock_count) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->lock_counts[name] = lock_count;
    m_impl->total_locks++;
}

void LockMonitor::recordUnlock(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->lock_counts[name] = 0;
}

void LockMonitor::recordContention(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->contention_counts[name]++;
    m_impl->total_contentions++;
}

int LockMonitor::getLockCount(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    auto it = m_impl->lock_counts.find(name);
    return it != m_impl->lock_counts.end() ? it->second : 0;
}

int LockMonitor::getContentionCount(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    auto it = m_impl->contention_counts.find(name);
    return it != m_impl->contention_counts.end() ? it->second : 0;
}

int LockMonitor::getTotalLocks() const {
    return m_impl->total_locks;
}

int LockMonitor::getTotalContentions() const {
    return m_impl->total_contentions;
}

void LockMonitor::reset() {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->lock_counts.clear();
    m_impl->contention_counts.clear();
    m_impl->total_locks = 0;
    m_impl->total_contentions = 0;
}

std::string LockMonitor::getStats() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::ostringstream oss;
    oss << "Lock Stats:\n";
    oss << "  Total Locks: " << m_impl->total_locks << "\n";
    oss << "  Total Contentions: " << m_impl->total_contentions << "\n";
    for (const auto& kv : m_impl->contention_counts) {
        oss << "  " << kv.first << ": " << kv.second << " contentions\n";
    }
    return oss.str();
}

} // namespace util
} // namespace base
