#include "../include/memory/smart_ptr.h"
#include "../include/io/json.h"
#include "../include/io/xml.h"
#include "../include/io/filesystem.h"
#include "../include/util/thread_pool.h"
#include "../include/util/time.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <random>
#include <thread>
#include <chrono>

using namespace base::memory;
using namespace base::io;
using namespace base::util;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(1, 100);
std::uniform_int_distribution<> size_dis(1, 20);

void test_smart_ptr_random() {
    std::cout << "\n=== Random Smart Pointer Tests ===" << std::endl;

    std::cout << "[Test 1] shared_ptr basic operations..." << std::endl;
    {
        shared_ptr<int> sp1(new int(dis(gen)));
        shared_ptr<int> sp2(sp1);
        shared_ptr<int> sp3 = sp1;

        assert(sp1.use_count() == 3);
        assert(sp2.use_count() == 3);
        assert(*sp1 == *sp2);

        sp3.reset(new int(dis(gen)));
        assert(sp1.use_count() == 2);

        shared_ptr<int> sp4(std::move(sp1));
        assert(sp2.use_count() == 2);
        assert(sp1.get() == nullptr);
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 2] shared_ptr with custom deleter..." << std::endl;
    {
        bool deleted = false;
        {
            auto deleter = [&deleted](int* p) {
                deleted = true;
                delete p;
            };
            shared_ptr<int> sp(new int(42), deleter);
        }
        assert(deleted);
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 3] unique_ptr operations..." << std::endl;
    {
        unique_ptr<int> up1(new int(dis(gen)));
        unique_ptr<int> up2(std::move(up1));
        assert(up1.get() == nullptr);
        assert(up2.get() != nullptr);

        int* raw = up2.release();
        assert(up2.get() == nullptr);
        delete raw;
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 4] weak_ptr lock and expired..." << std::endl;
    {
        shared_ptr<int> sp(new int(100));
        weak_ptr<int> wp(sp);

        assert(!wp.expired());
        assert(wp.use_count() == 1);

        shared_ptr<int> sp2 = wp.lock();
        assert(sp2.use_count() == 2);

        sp.reset();
        assert(wp.expired());
        assert(wp.lock().get() == nullptr);
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 5] Thread safety (concurrent access)..." << std::endl;
    {
        shared_ptr<int> sp(new int(0));
        std::vector<std::thread> threads;

        for (int i = 0; i < 10; i++) {
            threads.emplace_back([&sp]() {
                for (int j = 0; j < 100; j++) {
                    shared_ptr<int> temp = sp;
                    temp = sp;
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        assert(sp.use_count() == 1);
        std::cout << "  PASSED (count: " << sp.use_count() << ")" << std::endl;
    }

    std::cout << "All Smart Pointer random tests passed!" << std::endl;
}

void test_json_random() {
    std::cout << "\n=== Random JSON Tests ===" << std::endl;

    std::cout << "[Test 1] Parse and modify JSON..." << std::endl;
    {
        std::string jsonStr = "{\"name\":\"test\",\"age\":25,\"score\":95.5,\"active\":true,\"items\":[1,2,3]}";
        Json json = Json::parse(jsonStr);

        assert(json.isObject());
        assert(json["name"].asString() == "test");
        assert(json["age"].asNumber() == 25);
        assert(json["score"].asNumber() == 95.5);
        assert(json["active"].asBool() == true);
        assert(json["items"].isArray());
        assert(json["items"].size() == 3);

        json["name"] = "modified";
        json["age"] = 30;
        json["items"].push_back(4);

        std::string output = json.toString();
        assert(output.find("modified") != std::string::npos);
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 2] Create JSON from scratch..." << std::endl;
    {
        Json obj(Json::Type::OBJECT);
        obj["string"] = "hello";
        obj["number"] = 42;
        obj["decimal"] = 3.14;
        obj["boolean"] = false;
        obj["null"] = nullptr;

        Json arr(Json::Type::ARRAY);
        arr.push_back("one");
        arr.push_back("two");
        arr.push_back("three");
        obj["array"] = arr;

        std::string output = obj.toString();
        assert(output.find("hello") != std::string::npos);
        assert(output.find("42") != std::string::npos);
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 3] Nested JSON operations..." << std::endl;
    {
        Json root(Json::Type::OBJECT);
        Json nested(Json::Type::OBJECT);
        nested["level2"] = "deep value";
        root["level1"] = nested;

        assert(root["level1"]["level2"].asString() == "deep value");

        root["level1"]["level2"] = "modified";
        assert(root["level1"]["level2"].asString() == "modified");
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 4] JSON keys and remove..." << std::endl;
    {
        Json obj(Json::Type::OBJECT);
        obj["a"] = 1;
        obj["b"] = 2;
        obj["c"] = 3;

        auto keys = obj.keys();
        assert(keys.size() == 3);

        obj.remove("b");
        assert(!obj.has("b"));
        assert(obj.has("a"));
        assert(obj.has("c"));
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 5] JSON array manipulation..." << std::endl;
    {
        Json arr(Json::Type::ARRAY);
        for (int i = 0; i < 10; i++) {
            arr.push_back(i * 10);
        }

        assert(arr.size() == 10);
        assert(arr[0].asNumber() == 0);
        assert(arr[9].asNumber() == 90);

        arr.pop_back();
        assert(arr.size() == 9);
        assert(arr[8].asNumber() == 80);
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "All JSON random tests passed!" << std::endl;
}

void test_xml_random() {
    std::cout << "\n=== Random XML Tests ===" << std::endl;

    std::cout << "[Test 1] Create and serialize XML..." << std::endl;
    {
        XmlDocument doc("catalog");

        XmlDocument::Node book = doc.getRoot().addChild("book");
        book.setAttribute("id", "001");
        book.addChild("title", "C++ Programming");
        book.addChild("author", "John Doe");
        book.addChild("price", "59.99");

        XmlDocument::Node book2 = doc.getRoot().addChild("book");
        book2.setAttribute("id", "002");
        book2.addChild("title", "Data Structures");
        book2.addChild("author", "Jane Smith");
        book2.addChild("price", "49.99");

        std::string xmlStr = doc.toString();
        assert(xmlStr.find("<catalog>") != std::string::npos);
        assert(xmlStr.find("<book") != std::string::npos);
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 2] Parse XML string..." << std::endl;
    {
        std::string xmlStr = "<?xml version=\"1.0\"?><root><item value=\"test\">content</item></root>";
        XmlDocument doc = XmlDocument::parse(xmlStr);

        assert(doc.validate());
        XmlDocument::Node root = doc.getRoot();
        assert(root.getName() == "root");

        auto items = root.getChildren("item");
        assert(items.size() == 1);
        assert(items[0].getAttribute("value") == "test");
        assert(items[0].getText() == "content");
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 3] XML node attribute operations..." << std::endl;
    {
        XmlDocument::Node node("person");
        node.setAttribute("id", "123");
        node.setAttribute("type", "developer");

        assert(node.hasAttribute("id"));
        assert(node.getAttribute("id") == "123");
        assert(node.hasAttribute("type"));
        assert(node.getAttribute("type") == "developer");

        node.removeAttribute("type");
        assert(!node.hasAttribute("type"));

        node.setAttribute("type", "manager");
        assert(node.getAttribute("type") == "manager");
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 4] XML child node operations..." << std::endl;
    {
        XmlDocument doc("parent");

        XmlDocument::Node child1 = doc.getRoot().addChild("child");
        child1.setText("first");

        XmlDocument::Node child2 = doc.getRoot().addChild("child");
        child2.setText("second");

        auto children = doc.getRoot().getChildren("child");
        assert(children.size() == 2);
        assert(children[0].getText() == "first");
        assert(children[1].getText() == "second");

        assert(doc.getRoot().hasChildren());
        doc.getRoot().clearChildren();
        assert(!doc.getRoot().hasChildren());
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 5] Deep nested XML..." << std::endl;
    {
        XmlDocument doc("level0");

        XmlDocument::Node* current = &doc.getRoot();
        for (int i = 1; i <= 5; i++) {
            XmlDocument::Node next = current->addChild("level" + std::to_string(i));
            next.setText("content at level " + std::to_string(i));
            current = &next;
        }

        std::string xml = doc.toString();
        assert(xml.find("level0") != std::string::npos);
        assert(xml.find("level5") != std::string::npos);
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "All XML random tests passed!" << std::endl;
}

void test_filesystem_random() {
    std::cout << "\n=== Random FileSystem Tests ===" << std::endl;

    std::cout << "[Test 1] Directory operations..." << std::endl;
    {
        std::string testDir = "./random_test_dir";

        if (FileSystem::directoryExists(testDir)) {
            FileSystem::deleteDirectory(testDir);
        }

        assert(!FileSystem::directoryExists(testDir));
        assert(FileSystem::createDirectory(testDir));
        assert(FileSystem::directoryExists(testDir));

        std::string subDir = testDir + "/subdir";
        assert(FileSystem::createDirectories(subDir));
        assert(FileSystem::directoryExists(subDir));

        assert(FileSystem::deleteDirectory(subDir));
        assert(!FileSystem::directoryExists(subDir));
        assert(FileSystem::deleteDirectory(testDir));
        assert(!FileSystem::directoryExists(testDir));
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 2] File operations..." << std::endl;
    {
        std::string testDir = "./fs_test_dir";
        FileSystem::createDirectories(testDir);

        std::string testFile = testDir + "/test.txt";

        assert(FileSystem::createFile(testFile));
        assert(FileSystem::fileExists(testFile));

        {
            FileSystem::File file(testFile, "w");
            assert(file.open());
            file.writeLine("Line 1");
            file.writeLine("Line 2");
            file.writeLine("Line 3");
            file.close();
        }

        {
            FileSystem::File file(testFile, "r");
            assert(file.open());

            std::string line;
            int count = 0;
            while (file.readLine(line)) {
                count++;
            }
            assert(count == 3);
            file.close();
        }

        std::string newPath = testDir + "/renamed.txt";
        assert(FileSystem::renameFile(testFile, newPath));
        assert(!FileSystem::fileExists(testFile));
        assert(FileSystem::fileExists(newPath));

        std::string copyPath = testDir + "/copy.txt";
        assert(FileSystem::copyFile(newPath, copyPath));
        assert(FileSystem::fileExists(copyPath));

        assert(FileSystem::deleteFile(newPath));
        assert(FileSystem::deleteFile(copyPath));
        assert(FileSystem::deleteDirectory(testDir));
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 3] Path operations..." << std::endl;
    {
        std::string path = "/home/user/documents/file.txt";

        assert(FileSystem::isAbsolutePath("/absolute/path"));
        assert(FileSystem::isRelativePath("relative/path"));
        assert(!FileSystem::isAbsolutePath("relative/path"));

        std::string filename = FileSystem::getFileName(path);
        assert(filename == "file.txt");

        std::string ext = FileSystem::getFileExtension(path);
        assert(ext == ".txt");

        std::string dir = FileSystem::getDirectoryName(path);
        assert(dir == "/home/user/documents");

        std::string joined = FileSystem::joinPath("/path/to", "file.txt");
        assert(joined.find("file.txt") != std::string::npos);
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 4] File seek and tell..." << std::endl;
    {
        std::string testDir = "./fs_test_dir2";
        FileSystem::createDirectories(testDir);
        std::string testFile = testDir + "/seek_test.bin";

        {
            FileSystem::File file(testFile, "wb");
            assert(file.open());
            const char* data = "0123456789";
            file.write(data, 10);
            file.close();
        }

        {
            FileSystem::File file(testFile, "rb");
            assert(file.open());

            assert(file.tell() == 0);
            file.seek(5, 0);
            assert(file.tell() == 5);

            char buffer[10];
            size_t read = file.read(buffer, 5);
            assert(read == 5);
            buffer[read] = '\0';
            assert(buffer[0] == '5');

            file.seek(-3, 1);
            assert(file.tell() == 7);

            file.close();
        }

        FileSystem::deleteFile(testFile);
        FileSystem::deleteDirectory(testDir);
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 5] Current directory..." << std::endl;
    {
        std::string cwd = FileSystem::getCurrentDirectory();
        assert(!cwd.empty());
        assert(FileSystem::directoryExists(cwd));

        std::string absPath = FileSystem::getAbsolutePath(".");
        assert(!absPath.empty());
        std::cout << "  PASSED (cwd: " << cwd << ")" << std::endl;
    }

    std::cout << "All FileSystem random tests passed!" << std::endl;
}

void test_threadpool_random() {
    std::cout << "\n=== Random ThreadPool Tests ===" << std::endl;

    std::cout << "[Test 1] Basic task submission..." << std::endl;
    {
        ThreadPool pool(4);

        std::vector<std::future<int>> futures;
        for (int i = 0; i < 10; i++) {
            auto future = pool.submit([i]() {
                return i * i;
            });
            futures.push_back(std::move(future));
        }

        int sum = 0;
        for (auto& f : futures) {
            sum += f.get();
        }

        assert(sum == 285);
        std::cout << "  PASSED (sum: " << sum << ")" << std::endl;

        pool.shutdown();
    }

    std::cout << "[Test 2] Thread pool running state..." << std::endl;
    {
        ThreadPool pool(2);
        assert(pool.isRunning());
        assert(pool.getThreadCount() == 2);

        pool.submit([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });

        assert(pool.isRunning());

        pool.shutdown();
        assert(!pool.isRunning());
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 3] Task with different return types..." << std::endl;
    {
        ThreadPool pool(2);

        auto intFuture = pool.submit([]() -> int { return 42; });
        auto stringFuture = pool.submit([]() -> std::string { return "hello"; });
        auto doubleFuture = pool.submit([]() -> double { return 3.14; });

        assert(intFuture.get() == 42);
        assert(stringFuture.get() == "hello");
        assert(doubleFuture.get() > 3.13 && doubleFuture.get() < 3.15);

        pool.shutdown();
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 4] Concurrent tasks..." << std::endl;
    {
        ThreadPool pool(8);
        std::atomic<int> counter(0);

        std::vector<std::future<void>> futures;
        for (int i = 0; i < 100; i++) {
            futures.push_back(pool.submit([&counter]() {
                counter.fetch_add(1);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }));
        }

        for (auto& f : futures) {
            f.get();
        }

        assert(counter.load() == 100);
        std::cout << "  PASSED (counter: " << counter.load() << ")" << std::endl;

        pool.shutdown();
    }

    std::cout << "All ThreadPool random tests passed!" << std::endl;
}

void test_time_random() {
    std::cout << "\n=== Random Time Tests ===" << std::endl;

    std::cout << "[Test 1] Timestamp generation..." << std::endl;
    {
        int64_t ts = Time::timestamp();
        int64_t tsMs = Time::timestampMillis();

        assert(ts > 0);
        assert(tsMs > ts * 1000);

        std::cout << "  Timestamp: " << ts << std::endl;
        std::cout << "  Timestamp (ms): " << tsMs << std::endl;
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 2] Time formatting..." << std::endl;
    {
        std::string date = Time::getCurrentDate();
        std::string time = Time::getCurrentTime();

        assert(date.length() == 10);
        assert(time.length() == 8);
        assert(date[4] == '-');
        assert(date[7] == '-');
        assert(time[2] == ':');
        assert(time[5] == ':');

        std::string formatted = Time::format("%Y-%m-%d %H:%M:%S");
        assert(formatted.length() == 19);
        std::cout << "  Date: " << date << std::endl;
        std::cout << "  Time: " << time << std::endl;
        std::cout << "  Formatted: " << formatted << std::endl;
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 3] Timer operations..." << std::endl;
    {
        Time::Timer timer;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        double seconds = timer.elapsedSeconds();
        int64_t ms = timer.elapsedMilliseconds();

        assert(ms >= 95);
        assert(seconds >= 0.095);

        std::cout << "  Elapsed: " << ms << "ms (" << seconds << "s)" << std::endl;

        timer.reset();
        int64_t afterReset = timer.elapsedMilliseconds();
        assert(afterReset < 10);

        std::cout << "  After reset: " << afterReset << "ms" << std::endl;
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "[Test 4] Timer precision..." << std::endl;
    {
        Time::Timer timer;

        for (int i = 0; i < 1000; i++) {
            volatile int x = i * i;
        }

        int64_t elapsed = timer.elapsedMilliseconds();
        assert(elapsed >= 0);
        std::cout << "  1000 operations took: " << elapsed << "ms" << std::endl;
        std::cout << "  PASSED" << std::endl;
    }

    std::cout << "All Time random tests passed!" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   BaseLib Random Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    test_smart_ptr_random();
    test_json_random();
    test_xml_random();
    test_filesystem_random();
    test_threadpool_random();
    test_time_random();

    std::cout << "\n========================================" << std::endl;
    std::cout << "   All Random Tests PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
