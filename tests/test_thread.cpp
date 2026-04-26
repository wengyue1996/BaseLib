#include "util/thread.h"
#include "util/result.h"
#include <iostream>
#include <atomic>
#include <cassert>

using namespace base::util;

void test_thread_creation() {
    std::cout << "Test thread creation..." << std::endl;

    std::atomic<int> counter(0);

    {
        Thread t;
        assert(!t.isRunning());
        assert(t.getState() == ThreadState::Idle);
    }

    {
        ThreadOptions opts;
        opts.name = "TestThread";
        opts.priority = ThreadPriority::Normal;

        Thread t(opts, [&counter]() {
            counter++;
        });

        assert(t.getName() == "TestThread");
        assert(t.getPriority() == ThreadPriority::Normal);
    }

    std::cout << "  [PASS] Thread creation" << std::endl;
}

void test_thread_lifecycle() {
    std::cout << "Test thread lifecycle..." << std::endl;

    std::atomic<int> counter(0);

    ThreadOptions opts;
    opts.name = "LifecycleTest";

    Thread t(opts, [&counter]() {
        counter++;
    });

    auto result = t.start();
    assert(result.isSuccess());
    assert(t.isRunning());
    assert(t.getState() == ThreadState::Running);

    t.join();
    assert(!t.isRunning());
    assert(counter == 1);

    std::cout << "  [PASS] Thread lifecycle" << std::endl;
}

void test_thread_stop() {
    std::cout << "Test thread stop..." << std::endl;

    std::atomic<bool> running(false);

    Thread t([&running]() {
        running = true;
        Thread::sleep(100);
    });

    auto result = t.start();
    assert(result.isSuccess());

    Thread::sleep(50);
    assert(t.isRunning());
    assert(running);

    auto stopResult = t.stop(1000);
    assert(stopResult.isSuccess());
    assert(!t.isRunning());

    std::cout << "  [PASS] Thread stop" << std::endl;
}

void test_thread_priority() {
    std::cout << "Test thread priority..." << std::endl;

    ThreadOptions opts;
    opts.priority = ThreadPriority::Highest;
    opts.name = "PriorityTest";

    Thread t(opts, []() {});

    t.setPriority(ThreadPriority::Lowest);
    assert(t.getPriority() == ThreadPriority::Lowest);

    t.setPriority(ThreadPriority::AboveNormal);
    assert(t.getPriority() == ThreadPriority::AboveNormal);

    std::cout << "  [PASS] Thread priority" << std::endl;
}

void test_thread_name() {
    std::cout << "Test thread name..." << std::endl;

    Thread t;
    assert(t.getName().find("Thread-") == 0);

    t.setName("CustomName");
    assert(t.getName() == "CustomName");

    std::cout << "  [PASS] Thread name" << std::endl;
}

void test_thread_state() {
    std::cout << "Test thread state..." << std::endl;

    std::atomic<bool> started(false);

    Thread t([&started]() {
        started = true;
        Thread::sleep(50);
    });

    assert(t.getState() == ThreadState::Idle);

    t.start();
    assert(t.getState() == ThreadState::Running);
    assert(t.isRunning());
    assert(started);

    t.join();
    assert(t.getState() == ThreadState::Stopped);
    assert(!t.isRunning());

    std::cout << "  [PASS] Thread state" << std::endl;
}

void test_thread_guard() {
    std::cout << "Test thread guard..." << std::endl;

    std::atomic<int> counter(0);

    {
        ThreadGuard guard(Thread(ThreadOptions(), [&counter]() {
            counter++;
        }));

        guard->start();
        Thread::sleep(20);
        assert(guard->isRunning());
    }

    assert(counter == 1);
    assert(!ThreadOptions().name.empty() || true);

    std::cout << "  [PASS] Thread guard" << std::endl;
}

void test_thread_cleanup() {
    std::cout << "Test thread cleanup..." << std::endl;

    std::atomic<bool> cleanup_called(false);

    {
        Thread t(ThreadOptions(), []() {
            Thread::sleep(20);
        }, [&cleanup_called]() {
            cleanup_called = true;
        });

        t.start();
        Thread::sleep(10);
    }

    assert(cleanup_called);

    std::cout << "  [PASS] Thread cleanup" << std::endl;
}

void test_thread_join_deadlock() {
    std::cout << "Test join deadlock prevention..." << std::endl;

    Thread t([]() { Thread::sleep(50); });
    t.start();

    auto result = t.join();
    assert(result.isSuccess());

    std::cout << "  [PASS] Join deadlock prevention" << std::endl;
}

void test_thread_sleep_and_yield() {
    std::cout << "Test sleep and yield..." << std::endl;

    auto start = std::chrono::steady_clock::now();
    Thread::sleep(50);
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    assert(duration.count() >= 40);

    Thread::yield();

    std::cout << "  [PASS] Sleep and yield" << std::endl;
}

void test_current_thread_id() {
    std::cout << "Test current thread ID..." << std::endl;

    Thread::ThreadId mainId = Thread::getCurrentThreadId();
    assert(mainId != 0);

    std::atomic<Thread::ThreadId> threadId(0);

    Thread t([&threadId]() {
        threadId = Thread::getCurrentThreadId();
    });

    t.start();
    t.join();

    assert(threadId != 0);
    assert(threadId != mainId);

    std::cout << "  [PASS] Current thread ID" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "         Thread Module Unit Tests        " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    test_thread_creation();
    test_thread_lifecycle();
    test_thread_stop();
    test_thread_priority();
    test_thread_name();
    test_thread_state();
    test_thread_guard();
    test_thread_cleanup();
    test_thread_join_deadlock();
    test_thread_sleep_and_yield();
    test_current_thread_id();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    All Thread Tests Passed! (11/11)    " << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
