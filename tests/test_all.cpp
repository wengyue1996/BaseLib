#include "../include/io/json.h"
#include "../include/io/xml.h"
#include "../include/io/filesystem.h"
#include <iostream>
#include <cassert>

using namespace base::io;

int main() {
    std::cout << "=== BaseLib All Modules Test ===" << std::endl;

    std::cout << "\n[1] Testing JSON..." << std::endl;
    Json json = Json::parse("{\"name\":\"test\",\"value\":123}");
    assert(json.isObject());
    assert(json["name"].asString() == "test");
    assert(json["value"].asNumber() == 123);
    std::cout << "JSON tests passed!" << std::endl;

    std::cout << "\n[2] Testing XML..." << std::endl;
    XmlDocument doc("root");
    doc.getRoot().setAttribute("version", "1.0");
    doc.getRoot().addChild("item", "value1");
    std::string xmlStr = doc.toString();
    assert(xmlStr.find("<root") != std::string::npos);
    std::cout << "XML tests passed!" << std::endl;

    std::cout << "\n[3] Testing FileSystem..." << std::endl;
    std::string testDir = "./test_dir";
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
    assert(FileSystem::deleteFile(testFile));
    assert(FileSystem::deleteDirectory(testDir));
    std::cout << "FileSystem tests passed!" << std::endl;

    std::cout << "\n=== All tests passed! ===" << std::endl;
    return 0;
}