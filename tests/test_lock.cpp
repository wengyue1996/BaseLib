#include "util/lock.h"
#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <cassert>

using namespace base::util;

void test_recursive_mutex_basic() {
    std::cout << "Test RecursiveMutex basic..." << std::endl;

    RecursiveMutex mtx("TestMutex");
    assert(!mtx.isLocked());
    assert(mtx.getLockCount() == 0);

    mtx.lock();
    assert(mtx.isLocked());
    assert(mtx.getLockCount() == 1);

    mtx.unlock();
    assert(!mtx.isLocked());
    assert(mtx.getLockCount() == 0);

    std::cout << "  [PASS] RecursiveMutex basic" << std::endl;
}

void test_recursive_mutex_reentrant() {
    std::cout << "Test RecursiveMutex reentrant..." << std::endl;

    RecursiveMutex mtx("ReentrantMutex");
    mtx.lock();
    assert(mtx.getLockCount() == 1);

    mtx.lock();
    assert(mtx.getLockCount() == 2);

    mtx.unlock();
    assert(mtx.getLockCount() == 1);

    mtx.unlock();
    assert(mtx.getLockCount() == 0);

    std::cout << "  [PASS] RecursiveMutex reentrant" << std::endl;
}

void test_lock_guard() {
    std::cout << "Test LockGuard..." << std::endl;

    RecursiveMutex mtx("GuardMutex");

    {
        LockGuard<RecursiveMutex> guard(&mtx);
        assert(mtx.isLocked());
        assert(guard.isLocked());
    }

    assert(!mtx.isLocked());

    std::cout << "  [PASS] LockGuard" << std::endl;
}

void test_try_lock_guard() {
    std::cout << "Test TryLockGuard..." << std::endl;

    RecursiveMutex mtx("TryGuardMutex");

    {
        TryLockGuard<RecursiveMutex> guard(&mtx);
        assert(guard.isLocked());
        assert(guard);
    }

    assert(!mtx.isLocked());

    std::cout << "  [PASS] TryLockGuard" << std::endl;
}

void test_non_recursive_mutex_basic() {
    std::cout << "Test NonRecursiveMutex basic..." << std::endl;

    NonRecursiveMutex mtx("NRMutex");
    mtx.lock();
    assert(mtx.isLocked());
    mtx.unlock();
    assert(!mtx.isLocked());

    std::cout << "  [PASS] NonRecursiveMutex basic" << std::endl;
}

void test_read_write_lock() {
    std::cout << "Test ReadWriteLock..." << std::endl;

    ReadWriteLock rwlock("RWLock");

    rwlock.readLock();
    assert(rwlock.isReadLocked());
    rwlock.readUnlock();
    assert(!rwlock.isReadLocked());

    rwlock.writeLock();
    assert(rwlock.isWriteLocked());
    rwlock.writeUnlock();
    assert(!rwlock.isWriteLocked());

    std::cout << "  [PASS] ReadWriteLock" << std::endl;
}

void test_multithread_mutex() {
    std::cout << "Test multithread mutex..." << std::endl;

    RecursiveMutex mtx("MTMutex");
    std::atomic<int> counter(0);
    const int iterations = 1000;
    const int threads = 4;

    auto work = [&]() {
        for (int i = 0; i < iterations; ++i) {
            LockGuard<RecursiveMutex> guard(&mtx);
            counter++;
        }
    };

    std::vector<std::thread> thread_list;
    for (int i = 0; i < threads; ++i) {
        thread_list.emplace_back(work);
    }

    for (auto& t : thread_list) {
        t.join();
    }

    assert(counter == iterations * threads);

    std::cout << "  [PASS] Multithread mutex (counter=" << counter << ")" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "         Lock Module Unit Tests          " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    test_recursive_mutex_basic();
    test_recursive_mutex_reentrant();
    test_lock_guard();
    test_try_lock_guard();
    test_non_recursive_mutex_basic();
    test_read_write_lock();
    test_multithread_mutex();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "       All Basic Lock Tests Passed!     " << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
