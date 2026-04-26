#include "../include/memory/smart_ptr.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>

using namespace base::memory;

void testSharedPtr() {
    std::cout << "Testing shared_ptr..." << std::endl;

    {
        shared_ptr<int> sp1(new int(10));
        assert(*sp1 == 10);
        assert(sp1.use_count() == 1);

        {
            shared_ptr<int> sp2 = sp1;
            assert(*sp2 == 10);
            assert(sp1.use_count() == 2);
            assert(sp2.use_count() == 2);
        }

        assert(sp1.use_count() == 1);
    }

    {
        shared_ptr<int> sp1(new int(20));
        shared_ptr<int> sp2 = sp1;
        shared_ptr<int> sp3;
        sp3 = sp2;
        assert(*sp3 == 20);
        assert(sp3.use_count() == 3);
    }

    std::cout << "shared_ptr tests passed!" << std::endl;
}

void testUniquePtr() {
    std::cout << "Testing unique_ptr..." << std::endl;

    {
        unique_ptr<int> up1(new int(100));
        assert(*up1 == 100);

        unique_ptr<int> up2 = std::move(up1);
        assert(up1.get() == nullptr);
        assert(*up2 == 100);

        up2.reset(new int(200));
        assert(*up2 == 200);

        unique_ptr<int> up3 = std::move(up2);
        assert(up3.get() != nullptr);
    }

    std::cout << "unique_ptr tests passed!" << std::endl;
}

void testWeakPtr() {
    std::cout << "Testing weak_ptr..." << std::endl;

    {
        shared_ptr<int> sp1(new int(300));
        weak_ptr<int> wp1 = sp1;

        assert(!wp1.expired());
        assert(wp1.use_count() == 1);

        shared_ptr<int> sp2 = wp1.lock();
        assert(*sp2 == 300);
        assert(sp1.use_count() == 2);

        sp1.reset();
        sp2.reset();

        assert(wp1.expired());
    }

    {
        shared_ptr<int> sp1(new int(400));
        weak_ptr<int> wp1 = sp1;

        sp1.reset();

        shared_ptr<int> sp2 = wp1.lock();
        assert(sp2.get() == nullptr);
    }

    std::cout << "weak_ptr tests passed!" << std::endl;
}

void testThreadSafety() {
    std::cout << "Testing thread safety..." << std::endl;

    shared_ptr<int> sp(new int(0));

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&sp]() {
            for (int j = 0; j < 100; ++j) {
                shared_ptr<int> sp2 = sp;
                int value = *sp2;
                (void)value;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Thread safety tests passed!" << std::endl;
}

int main() {
    std::cout << "=== Smart Pointer Tests ===" << std::endl;

    testSharedPtr();
    testUniquePtr();
    testWeakPtr();
    testThreadSafety();

    std::cout << "\nAll smart pointer tests passed!" << std::endl;
    return 0;
}