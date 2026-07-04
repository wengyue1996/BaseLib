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

void testNestedXmlParsing() {
    std::cout << "Testing nested XML parsing..." << std::endl;

    XmlDocument::Node root("root");
    root.setAttribute("version", "1.0");

    XmlDocument::Node& child1 = root.addChild("item");
    child1.setAttribute("id", "1");
    child1.setText("First");

    XmlDocument::Node& child2 = root.addChild("item");
    child2.setAttribute("id", "2");
    child2.setText("Second");

    XmlDocument::Node& subChild = child2.addChild("sub");
    subChild.setText("SubItem");

    assert(root.getAllChildren().size() == 2);
    assert(root.getAllChildren()[1].getAllChildren().size() == 1);

    XmlDocument doc;
    doc.setRoot(root);

    std::string xml = doc.toString();
    assert(xml.find("<root") != std::string::npos);
    assert(xml.find("version=\"1.0\"") != std::string::npos);
    assert(xml.find("<item") != std::string::npos);
    assert(xml.find("id=\"1\"") != std::string::npos);
    assert(xml.find("First") != std::string::npos);
    assert(xml.find("<sub>") != std::string::npos);
    assert(xml.find("SubItem") != std::string::npos);

    std::cout << "Nested XML parsing tests passed!" << std::endl;
}

void testXmlRoundtrip() {
    std::cout << "Testing XML roundtrip..." << std::endl;

    XmlDocument::Node root("config");
    XmlDocument::Node& section = root.addChild("section");
    section.setAttribute("name", "network");
    section.addChild("host", "127.0.0.1");
    section.addChild("port", "8080");

    XmlDocument doc;
    doc.setRoot(root);

    std::string xml = doc.toString();
    XmlDocument parsed = XmlDocument::parse(xml);
    std::string reparsed = parsed.toString();
    assert(reparsed.find("config") != std::string::npos);
    assert(reparsed.find("section") != std::string::npos);
    assert(reparsed.find("127.0.0.1") != std::string::npos);

    std::cout << "XML roundtrip tests passed!" << std::endl;
}

void testRecursiveDirectoryOperations() {
    std::cout << "Testing recursive directory operations..." << std::endl;

    std::string baseDir = "./test_recursive_dir";
    assert(FileSystem::createDirectory(baseDir));

    std::string subDir1 = FileSystem::joinPath(baseDir, "sub1");
    std::string subDir2 = FileSystem::joinPath(baseDir, "sub2");
    assert(FileSystem::createDirectory(subDir1));
    assert(FileSystem::createDirectory(subDir2));

    std::string file1 = FileSystem::joinPath(baseDir, "file1.txt");
    std::string file2 = FileSystem::joinPath(subDir1, "file2.txt");
    assert(FileSystem::createFile(file1));
    assert(FileSystem::createFile(file2));

    assert(FileSystem::fileExists(file1));
    assert(FileSystem::fileExists(file2));
    assert(FileSystem::directoryExists(subDir1));

    assert(FileSystem::deleteDirectoryRecursive(baseDir));
    assert(!FileSystem::directoryExists(baseDir));

    std::cout << "Recursive directory tests passed!" << std::endl;
}

void testFilesystemErrorHandling() {
    std::cout << "Testing filesystem error handling..." << std::endl;

    assert(!FileSystem::fileExists("./__nonexistent_file__.txt"));
    assert(!FileSystem::directoryExists("./__nonexistent_dir__"));
    assert(!FileSystem::deleteDirectoryRecursive("./__nonexistent_dir__"));

    std::string error = FileSystem::getLastError();
    // Just verify getLastError() is callable on all platforms

    std::cout << "Filesystem error handling tests passed!" << std::endl;
}

void testFilesystemPathOperations() {
    std::cout << "Testing filesystem path operations..." << std::endl;

    assert(FileSystem::isAbsolutePath(FileSystem::getAbsolutePath(".")));
    assert(FileSystem::isRelativePath("relative/path"));

    assert(FileSystem::getFileName("/path/to/file.txt") == "file.txt");
    assert(FileSystem::getFileExtension("/path/to/file.txt") == ".txt");
    assert(FileSystem::getFileExtension("/path/to/file") == "");
    assert(FileSystem::getDirectoryName("/path/to/file.txt") == "/path/to");

    std::string cwd = FileSystem::getCurrentDirectory();
    assert(!cwd.empty());

    std::string joined = FileSystem::joinPath("/path/to", "file.txt");
    assert(joined.find("file.txt") != std::string::npos);
    assert(joined.find("/path/to") != std::string::npos);

    std::cout << "Filesystem path operation tests passed!" << std::endl;
}

int main() {
    std::cout << "=== IO Module Tests ===" << std::endl;
    testJsonParse();
    testJsonArray();
    testJsonSerialize();
    testXml();
    testNestedXmlParsing();
    testXmlRoundtrip();
    testFileSystem();
    testRecursiveDirectoryOperations();
    testFilesystemErrorHandling();
    testFilesystemPathOperations();
    std::cout << "\nAll IO module tests passed!" << std::endl;
    return 0;
}