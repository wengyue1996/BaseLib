#include "../include/util/config.h"
#include "../include/util/error.h"
#include "../include/util/result.h"
#include "../include/util/thread_pool.h"
#include "../include/util/time.h"
#include <iostream>
#include <cassert>

using namespace base::util;
using base::ErrorCode;

void testConfig() {
    std::cout << "Testing Config..." << std::endl;

    Config config;
    config.set("name", "test");
    config.set("value", 42);
    config.set("active", true);

    assert(config.get("name", std::string("")) == "test");
    assert(config.get("value", 0) == 42);
    assert(config.get("active", false) == true);
    assert(config.has("name"));
    assert(!config.has("nonexistent"));

    config.remove("value");
    assert(!config.has("value"));

    // Boundary: missing key returns default
    assert(config.get("missing", std::string("default")) == "default");
    assert(config.get("missing_int", 999) == 999);
    assert(config.get("missing_bool", true) == true);

    // Boundary: invalid value returns default
    config.set("str_val", "hello");
    assert(config.get("str_val", 0) == 0);

    std::string jsonStr = config.toJson();
    assert(jsonStr.find("name") != std::string::npos);

    std::cout << "Config tests passed!" << std::endl;
}

void testError() {
    std::cout << "Testing Error..." << std::endl;

    Exception e(1001, "Invalid argument", "param is null");
    assert(e.code() == 1001);
    assert(e.message() == "Invalid argument");
    assert(e.details() == "param is null");

    e.setDetails("new details");
    assert(e.details() == "new details");

    assert(ErrorCode::SUCCESS == 0);
    assert(ErrorCode::INVALID_ARGUMENT == 1001);
    assert(ErrorCode::FILE_NOT_FOUND == 3001);

    std::cout << "Error tests passed!" << std::endl;
}

void testThreadPool() {
    std::cout << "Testing ThreadPool..." << std::endl;

    ThreadPool pool(4);
    assert(pool.isRunning());
    assert(pool.getThreadCount() == 4);
    assert(pool.getTaskCount() == 0);

    int value = 0;
    auto future = pool.submit([&value]() {
        return ++value;
    });

    assert(future.get() == 1);
    assert(pool.getTaskCount() == 0);

    // Pause/Resume
    pool.pause();
    assert(pool.isPaused());
    pool.resume();
    assert(!pool.isPaused());

    // waitAll
    {
        std::atomic<int> counter(0);
        for (int i = 0; i < 4; ++i) {
            pool.submit([&counter]() { counter++; });
        }
        pool.waitAll();
        assert(counter.load() == 4);
    }

    pool.shutdown();
    assert(!pool.isRunning());

    std::cout << "ThreadPool tests passed!" << std::endl;
}

void testTime() {
    std::cout << "Testing Time..." << std::endl;

    int64_t ts = Time::timestamp();
    assert(ts > 0);
    (void)ts;

    int64_t tsMillis = Time::timestampMillis();
    assert(tsMillis > 0);
    (void)tsMillis;

    std::string date = Time::getCurrentDate();
    assert(date.find('-') != std::string::npos);

    std::string time = Time::getCurrentTime();
    assert(time.find(':') != std::string::npos);

    Time::Timer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(timer.elapsedMilliseconds() >= 100);

    // DateTime
    DateTime dt = DateTime::now();
    assert(dt.year >= 2026);
    assert(dt.month >= 1 && dt.month <= 12);
    assert(dt.day >= 1 && dt.day <= 31);
    std::string formatted = dt.format("%Y-%m-%d");
    assert(!formatted.empty());

    std::cout << "Time tests passed!" << std::endl;
}

int main() {
    std::cout << "=== Util Module Tests ===" << std::endl;

    testConfig();
    testError();
    testThreadPool();
    testTime();

    std::cout << "\nAll util module tests passed!" << std::endl;
    return 0;
}