#include "../include/memory/smart_ptr.h"
#include <iostream>
#include <atomic>
#include <cassert>
#include <thread>
#include <vector>

using namespace base::memory;

static std::atomic<int> g_destructor_count(0);

class TestObject {
public:
    TestObject() : m_value(0) {}
    explicit TestObject(int v) : m_value(v) {}
    ~TestObject() { g_destructor_count++; }
    int getValue() const { return m_value; }
    void setValue(int v) { m_value = v; }
private:
    int m_value;
};

void test_shared_ptr_basic() {
    std::cout << "Test shared_ptr basic..." << std::endl;
    g_destructor_count = 0;
    {
        shared_ptr<TestObject> p1(new TestObject(42));
        assert(p1);
        assert(p1->getValue() == 42);
        assert(p1.use_count() == 1);
        assert(p1.unique());
    }
    assert(g_destructor_count == 1);
    std::cout << "  [PASS]" << std::endl;
}

void test_shared_ptr_copy() {
    std::cout << "Test shared_ptr copy..." << std::endl;
    g_destructor_count = 0;
    {
        shared_ptr<TestObject> p1(new TestObject(42));
        shared_ptr<TestObject> p2 = p1;
        assert(p1.use_count() == 2);
        assert(p2.use_count() == 2);
    }
    assert(g_destructor_count == 1);
    std::cout << "  [PASS]" << std::endl;
}

void test_shared_ptr_move() {
    std::cout << "Test shared_ptr move..." << std::endl;
    g_destructor_count = 0;
    {
        shared_ptr<TestObject> p1(new TestObject(100));
        shared_ptr<TestObject> p2 = std::move(p1);
        assert(!p1);
        assert(p2);
        assert(p2->getValue() == 100);
        assert(p2.use_count() == 1);
    }
    assert(g_destructor_count == 1);
    std::cout << "  [PASS]" << std::endl;
}

void test_shared_ptr_reset() {
    std::cout << "Test shared_ptr reset..." << std::endl;
    g_destructor_count = 0;
    {
        shared_ptr<TestObject> p1(new TestObject(50));
        p1 = shared_ptr<TestObject>(new TestObject(60));
        assert(p1->getValue() == 60);
        assert(p1.use_count() == 1);
        assert(g_destructor_count == 1);
        p1.reset();
        assert(!p1);
        assert(g_destructor_count == 2);
    }
    std::cout << "  [PASS]" << std::endl;
}

void test_shared_ptr_swap() {
    std::cout << "Test shared_ptr swap..." << std::endl;
    g_destructor_count = 0;
    {
        shared_ptr<TestObject> p1(new TestObject(10));
        shared_ptr<TestObject> p2(new TestObject(20));
        p1.swap(p2);
        assert(p1->getValue() == 20);
        assert(p2->getValue() == 10);
    }
    assert(g_destructor_count == 2);
    std::cout << "  [PASS]" << std::endl;
}

void test_make_shared() {
    std::cout << "Test make_shared..." << std::endl;
    g_destructor_count = 0;
    {
        auto p1 = make_shared<TestObject>(123);
        assert(p1);
        assert(p1->getValue() == 123);
        auto p2 = p1;
        assert(p1.use_count() == 2);
    }
    assert(g_destructor_count == 1);
    std::cout << "  [PASS]" << std::endl;
}

void test_unique_ptr_basic() {
    std::cout << "Test unique_ptr basic..." << std::endl;
    g_destructor_count = 0;
    {
        unique_ptr<TestObject> p1(new TestObject(77));
        assert(p1);
        assert((*p1).getValue() == 77);
        unique_ptr<TestObject> p2 = std::move(p1);
        assert(!p1);
        assert(p2);
    }
    assert(g_destructor_count == 1);
    std::cout << "  [PASS]" << std::endl;
}

void test_make_unique() {
    std::cout << "Test make_unique..." << std::endl;
    g_destructor_count = 0;
    {
        auto p1 = make_unique<TestObject>(999);
        assert(p1);
        assert(p1->getValue() == 999);
    }
    assert(g_destructor_count == 1);
    std::cout << "  [PASS]" << std::endl;
}

void test_weak_ptr_basic() {
    std::cout << "Test weak_ptr basic..." << std::endl;
    g_destructor_count = 0;
    {
        shared_ptr<TestObject> sp(new TestObject(111));
        weak_ptr<TestObject> wp = sp;
        assert(!wp.expired());
        assert(wp.use_count() == 1);
        shared_ptr<TestObject> sp2 = wp.lock();
        assert(sp2);
        assert(sp2->getValue() == 111);
    }
    assert(g_destructor_count == 1);
    std::cout << "  [PASS]" << std::endl;
}

void test_weak_ptr_expired() {
    std::cout << "Test weak_ptr expired..." << std::endl;
    g_destructor_count = 0;
    {
        weak_ptr<TestObject> wp;
        assert(wp.expired());
        {
            shared_ptr<TestObject> sp(new TestObject(222));
            wp = sp;
            assert(!wp.expired());
        }
        assert(wp.expired());
        shared_ptr<TestObject> sp = wp.lock();
        assert(!sp);
    }
    assert(g_destructor_count == 1);
    std::cout << "  [PASS]" << std::endl;
}

void test_multithread() {
    std::cout << "Test multithread..." << std::endl;
    g_destructor_count = 0;
    shared_ptr<TestObject> sp(new TestObject(0));
    const int num_threads = 4;
    const int iterations = 1000;

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&sp, iterations]() {
            for (int i = 0; i < iterations; ++i) {
                shared_ptr<TestObject> local = sp;
                local->setValue(local->getValue() + 1);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    assert(sp->getValue() == num_threads * iterations);
    sp.reset();
    assert(g_destructor_count == 1);
    std::cout << "  [PASS] (counter=" << num_threads * iterations << ")" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "      Smart Pointer Unit Tests           " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    test_shared_ptr_basic();
    test_shared_ptr_copy();
    test_shared_ptr_move();
    test_shared_ptr_reset();
    test_shared_ptr_swap();
    test_make_shared();
    test_unique_ptr_basic();
    test_make_unique();
    test_weak_ptr_basic();
    test_weak_ptr_expired();
    test_multithread();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  All Smart Pointer Tests Passed! (11)  " << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
