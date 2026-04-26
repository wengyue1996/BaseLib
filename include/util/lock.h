#ifndef BASE_LOCK_H
#define BASE_LOCK_H

#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include "result.h"

namespace base {
namespace util {

class ILock {
public:
    virtual ~ILock() = default;

    virtual void lock() = 0;
    virtual bool tryLock() = 0;
    virtual bool tryLock(int timeout_ms) = 0;
    virtual void unlock() = 0;

    virtual bool isLocked() const = 0;
    virtual int getLockCount() const = 0;
    virtual std::string getName() const = 0;
};

class RecursiveMutex : public ILock {
public:
    explicit RecursiveMutex(const std::string& name = "");
    ~RecursiveMutex() override;

    void lock() override;
    bool tryLock() override;
    bool tryLock(int timeout_ms) override;
    void unlock() override;

    bool isLocked() const override;
    int getLockCount() const override;
    std::string getName() const override;

    RecursiveMutex(const RecursiveMutex&) = delete;
    RecursiveMutex& operator=(const RecursiveMutex&) = delete;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class NonRecursiveMutex : public ILock {
public:
    explicit NonRecursiveMutex(const std::string& name = "");
    ~NonRecursiveMutex() override;

    void lock() override;
    bool tryLock() override;
    bool tryLock(int timeout_ms) override;
    void unlock() override;

    bool isLocked() const override;
    int getLockCount() const override;
    std::string getName() const override;

    NonRecursiveMutex(const NonRecursiveMutex&) = delete;
    NonRecursiveMutex& operator=(const NonRecursiveMutex&) = delete;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

enum class LockScopeType {
    Global,
    Module,
    Resource,
    Custom
};

class LockScope {
public:
    LockScope(ILock* lock, LockScopeType type = LockScopeType::Custom);
    LockScope(ILock& lock, LockScopeType type = LockScopeType::Custom);
    ~LockScope();

    LockScope(const LockScope&) = delete;
    LockScope& operator=(const LockScope&) = delete;

    LockScope(LockScope&& other) noexcept;
    LockScope& operator=(LockScope&& other) noexcept;

    void release();
    bool isAcquired() const;

    LockScopeType getType() const { return m_type; }
    const std::string& getLockName() const { return m_lockName; }

private:
    ILock* m_lock;
    bool m_acquired;
    LockScopeType m_type;
    std::string m_lockName;
};

template<typename T>
class LockGuard {
public:
    explicit LockGuard(T* lock) : m_lock(lock) {
        if (m_lock) {
            m_lock->lock();
            m_locked = true;
        }
    }

    explicit LockGuard(T& lock) : m_lock(&lock) {
        m_lock->lock();
        m_locked = true;
    }

    ~LockGuard() {
        if (m_locked && m_lock) {
            m_lock->unlock();
        }
    }

    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;

    LockGuard(LockGuard&& other) noexcept
        : m_lock(other.m_lock), m_locked(other.m_locked) {
        other.m_locked = false;
    }

    LockGuard& operator=(LockGuard&& other) noexcept {
        if (this != &other) {
            if (m_locked && m_lock) {
                m_lock->unlock();
            }
            m_lock = other.m_lock;
            m_locked = other.m_locked;
            other.m_locked = false;
        }
        return *this;
    }

    void unlock() {
        if (m_locked && m_lock) {
            m_lock->unlock();
            m_locked = false;
        }
    }

    void lock() {
        if (!m_locked && m_lock) {
            m_lock->lock();
            m_locked = true;
        }
    }

    bool isLocked() const { return m_locked; }

private:
    T* m_lock;
    bool m_locked = false;
};

template<typename T>
class TryLockGuard {
public:
    explicit TryLockGuard(T* lock) : m_lock(lock), m_locked(false) {
        if (m_lock) {
            m_locked = m_lock->tryLock();
        }
    }

    explicit TryLockGuard(T& lock) : m_lock(&lock), m_locked(false) {
        m_locked = m_lock->tryLock();
    }

    ~TryLockGuard() {
        if (m_locked && m_lock) {
            m_lock->unlock();
        }
    }

    TryLockGuard(const TryLockGuard&) = delete;
    TryLockGuard& operator=(const TryLockGuard&) = delete;

    TryLockGuard(TryLockGuard&& other) noexcept
        : m_lock(other.m_lock), m_locked(other.m_locked) {
        other.m_locked = false;
    }

    TryLockGuard& operator=(TryLockGuard&& other) noexcept {
        if (this != &other) {
            if (m_locked && m_lock) {
                m_lock->unlock();
            }
            m_lock = other.m_lock;
            m_locked = other.m_locked;
            other.m_locked = false;
        }
        return *this;
    }

    bool isLocked() const { return m_locked; }
    explicit operator bool() const { return m_locked; }

private:
    T* m_lock;
    bool m_locked;
};

class ReadWriteLock {
public:
    ReadWriteLock(const std::string& name = "");
    ~ReadWriteLock();

    void readLock();
    bool tryReadLock();
    bool tryReadLock(int timeout_ms);
    void readUnlock();

    void writeLock();
    bool tryWriteLock();
    bool tryWriteLock(int timeout_ms);
    void writeUnlock();

    bool isReadLocked() const;
    bool isWriteLocked() const;
    int getReadCount() const;

    ReadWriteLock(const ReadWriteLock&) = delete;
    ReadWriteLock& operator=(const ReadWriteLock&) = delete;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class LockMonitor {
public:
    static LockMonitor& getInstance();

    void recordLock(const std::string& name, int lock_count);
    void recordUnlock(const std::string& name);
    void recordContention(const std::string& name);

    int getLockCount(const std::string& name) const;
    int getContentionCount(const std::string& name) const;
    int getTotalLocks() const;
    int getTotalContentions() const;

    void reset();
    std::string getStats() const;

private:
    LockMonitor() = default;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

using Mutex = RecursiveMutex;
using RMutex = RecursiveMutex;
using NRMutex = NonRecursiveMutex;

} // namespace util
} // namespace base

#endif // BASE_LOCK_H
