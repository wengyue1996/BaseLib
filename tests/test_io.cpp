#include "../include/io/json.h"
#include "../include/io/xml.h"
#include "../include/io/filesystem.h"
#include <iostream>
#include <cassert>

using namespace base::io;

void testJsonParse() {
    std::cout << "Testing JSON parsing..." << std::endl;
    Json json = Json::parse("{\"name\":\"test\",\"value\":123,\"active\":true}");
    assert(json.isObject());
    assert(json["name"].asString() == "test");
    assert(json["value"].asNumber() == 123);
    assert(json["active"].asBool() == true);
    std::cout << "JSON parsing tests passed!" << std::endl;
}

void testJsonArray() {
    std::cout << "Testing JSON array..." << std::endl;
    Json arr = Json::parse("[1,2,3,4,5]");
    assert(arr.isArray());
    assert(arr.size() == 5);
    assert(arr[0].asNumber() == 1);
    std::cout << "JSON array tests passed!" << std::endl;
}

void testJsonSerialize() {
    std::cout << "Testing JSON serialization..." << std::endl;
    Json obj;
    obj["key1"] = "value1";
    obj["key2"] = 42;
    std::string jsonStr = obj.toString();
    assert(jsonStr.find("key1") != std::string::npos);
    std::cout << "JSON serialization tests passed!" << std::endl;
}

void testXml() {
    std::cout << "Testing XML..." << std::endl;
    XmlDocument doc("root");
    doc.getRoot().setAttribute("version", "1.0");
    doc.getRoot().addChild("item", "value1");
    std::string xmlStr = doc.toString();
    assert(xmlStr.find("<root") != std::string::npos);
    std::cout << "XML tests passed!" << std::endl;
}

void testFileSystem() {
    std::cout << "Testing FileSystem..." << std::endl;
    std::string testDir = "./test_fs_dir";
    assert(FileSystem::createDirectory(testDir));
    assert(FileSystem::directoryExists(testDir));

    std::string testFile = testDir + "/test.txt";
    assert(FileSystem::createFile(testFile));
    assert(FileSystem::fileExists(testFile));

    {
        FileSystem::File file(testFile, "w");
        assert(file.open());
        assert(file.writeLine("Hello"));
        file.close();
    }

    {
        FileSystem::File file(testFile, "r");
        assert(file.open());
        std::string line;
        assert(file.readLine(line));
        assert(line == "Hello");
        file.close();
    }

    assert(FileSystem::deleteFile(testFile));
    assert(FileSystem::deleteDirectory(testDir));
    std::cout << "FileSystem tests passed!" << std::endl;
}

int main() {
    std::cout << "=== IO Module Tests ===" << std::endl;
    testJsonParse();
    testJsonArray();
    testJsonSerialize();
    testXml();
    testFileSystem();
    std::cout << "\nAll IO module tests passed!" << std::endl;
    return 0;
}