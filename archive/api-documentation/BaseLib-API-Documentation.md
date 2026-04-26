# BaseLib C++基础库 API接口文档

## 文档信息

| 项目 | 内容 |
|------|------|
| 版本 | v2.0 |
| 创建日期 | 2026-04-26 |
| 更新日期 | 2026-04-26 |
| 支持平台 | Windows / Linux / macOS |
| C++标准 | C++11 |
| 第三方依赖 | 无（仅使用C++标准库） |

---

## 目录

1. [概述](#1-概述)
2. [核心模块 (Core)](#2-核心模块-core)
   - 2.1 [日志模块 (Logger)](#21-日志模块-logger)
   - 2.2 [错误处理 (Error)](#22-错误处理-error)
   - 2.3 [结果封装 (Result)](#23-结果封装-result)
3. [内存管理 (Memory)](#3-内存管理-memory)
   - 3.1 [智能指针 (Smart Pointer)](#31-智能指针-smart-pointer)
4. [数据结构 (Data Structures)](#4-数据结构-data-structures)
   - 4.1 [JSON模块](#41-json模块)
   - 4.2 [XML模块](#42-xml模块)
5. [文件系统 (IO)](#5-文件系统-io)
   - 5.1 [文件系统模块 (FileSystem)](#51-文件系统模块-filesystem)
6. [网络通信 (Network)](#6-网络通信-network)
   - 6.1 [TCP模块](#61-tcp模块)
   - 6.2 [UDP模块](#62-udp模块)
   - 6.3 [HTTP模块](#63-http模块)
   - 6.4 [HTTP客户端模块 (HttpClient)](#64-http客户端模块-httpclient)
   - 6.5 [HTTP服务端模块 (HttpServer)](#65-http服务端模块-httpserver)
7. [并发编程 (Concurrency)](#7-并发编程-concurrency)
   - 7.1 [线程模块 (Thread)](#71-线程模块-thread)
   - 7.2 [线程池模块 (ThreadPool)](#72-线程池模块-threadpool)
   - 7.3 [锁抽象 (Lock)](#73-锁抽象-lock)
8. [工具模块 (Utilities)](#8-工具模块-utilities)
   - 8.1 [配置管理 (Config)](#81-配置管理-config)
   - 8.2 [时间工具 (Time)](#82-时间工具-time)
9. [错误码参考](#9-错误码参考)
10. [跨平台注意事项](#10-跨平台注意事项)
11. [编译和链接](#11-编译和链接)
12. [最佳实践](#12-最佳实践)

---

## 1. 概述

### 1.1 库简介

BaseLib是一个跨平台的C++基础库，提供常用数据结构、网络通信、并发编程、文件IO等功能。所有模块仅依赖C++标准库，无需额外第三方依赖。

### 1.2 命名空间

| 命名空间 | 说明 |
|---------|------|
| `base` | 根命名空间 |
| `base::log` | 日志模块 |
| `base::net` | 网络模块 |
| `base::util` | 工具模块 |
| `base::io` | IO模块 |
| `base::mem` | 内存管理模块 |

### 1.3 头文件引用

```cpp
#include "base.h"                    // 主头文件，包含所有模块
#include "core/logger.h"             // 日志
#include "util/error.h"              // 错误处理
#include "util/result.h"             // 结果封装
#include "memory/smart_ptr.h"        // 智能指针
#include "io/json.h"                 // JSON解析
#include "io/xml.h"                  // XML解析
#include "io/filesystem.h"           // 文件系统
#include "net/tcp.h"                 // TCP
#include "net/udp.h"                 // UDP
#include "net/http.h"                 // HTTP
#include "net/http_client.h"          // HTTP客户端
#include "net/http_server.h"         // HTTP服务端
#include "util/thread.h"             // 线程
#include "util/thread_pool.h"        // 线程池
#include "util/lock.h"               // 锁
#include "util/config.h"             // 配置
#include "util/time.h"               // 时间
```

---

## 2. 核心模块 (Core)

### 2.1 日志模块 (Logger)

**头文件**: `include/core/logger.h`
**命名空间**: `base::log`

#### 2.1.1 日志级别

```cpp
namespace base::log {
    enum class Level {
        DEBUG,  // 调试信息
        INFO,   // 一般信息
        WARN,   // 警告信息
        ERROR,  // 错误信息
        FATAL   // 致命错误
    };
}
```

#### 2.1.2 初始化和关闭

```cpp
// 初始化日志系统
// 参数: logDir - 日志目录路径，默认为 "logs"
void Logger::init(const std::string& logDir = "logs");

// 关闭日志系统
void Logger::shutdown();

// 检查是否已初始化
bool Logger::isInitialized();
```

**请求参数**:

| 参数名称 | 数据类型 | 是否必填 | 默认值 | 说明 |
|---------|---------|---------|-------|------|
| logDir | string | 否 | "logs" | 日志文件存储目录 |

**调用示例**:

```cpp
base::log::Logger::init("logs");
BASE_LOG_INFO("Application", "Starting...");
```

#### 2.1.3 日志输出宏

```cpp
// 使用默认模块名的日志宏
BASE_LOG_DEBUG(msg)
BASE_LOG_INFO(msg)
BASE_LOG_WARN(msg)
BASE_LOG_ERROR(msg)
BASE_LOG_FATAL(msg)

// 指定模块名的日志宏
BASE_LOG_DEBUG(module, msg)
BASE_LOG_INFO(module, msg)
BASE_LOG_WARN(module, msg)
BASE_LOG_ERROR(module, msg)
BASE_LOG_FATAL(module, msg)
```

**请求参数**:

| 参数名称 | 数据类型 | 是否必填 | 默认值 | 说明 |
|---------|---------|---------|-------|------|
| module | string | 是 | 无 | 模块名称 |
| msg | string | 是 | 无 | 日志消息 |

**调用示例**:

```cpp
BASE_LOG_INFO("Network", "Connection established");
BASE_LOG_ERROR("Database", "Query failed: timeout");
```

#### 2.1.4 LoggerConfig 配置结构

```cpp
struct LoggerConfig {
    Level minLevel = Level::INFO;        // 最小日志级别
    size_t maxFileSize = 10 * 1024 * 1024; // 单文件最大字节数
    size_t maxFiles = 5;                  // 保留文件数
    bool consoleOutput = true;            // 是否输出到控制台
    bool fileOutput = true;               // 是否输出到文件
    std::string pattern = "[{timestamp}] [{level}] [{module}] {message}";
};
```

#### 2.1.5 完整使用示例

```cpp
#include "core/logger.h"
#include <iostream>

int main() {
    // 初始化日志系统
    base::log::Logger::init("logs");

    // 使用日志宏记录不同级别的日志
    BASE_LOG_DEBUG("App", "Debug information");
    BASE_LOG_INFO("App", "Application started");
    BASE_LOG_WARN("App", "Low memory warning");
    BASE_LOG_ERROR("App", "Error occurred");

    // 关闭日志系统
    base::log::Logger::shutdown();
    return 0;
}
```

---

### 2.2 错误处理 (Error)

**头文件**: `include/util/error.h`
**命名空间**: `base`

#### 2.2.1 错误码定义

```cpp
class ErrorCode {
public:
    // 通用错误 (1000-1999)
    static const int SUCCESS = 0;              // 成功
    static const int UNKNOWN_ERROR = 1000;      // 未知错误
    static const int INVALID_ARGUMENT = 1001;   // 无效参数
    static const int NULL_POINTER = 1002;       // 空指针
    static const int INVALID_STATE = 1003;      // 无效状态
    static const int TIMEOUT = 1004;           // 操作超时
    static const int NOT_IMPLEMENTED = 1005;   // 未实现

    // IO错误 (2000-2999)
    static const int IO_ERROR = 2000;          // IO错误
    static const int FILE_NOT_FOUND = 2001;     // 文件不存在
    static const int PERMISSION_DENIED = 2002;  // 权限拒绝
    static const int FILE_ALREADY_EXISTS = 2003; // 文件已存在
    static const int DIRECTORY_NOT_FOUND = 2004; // 目录不存在
    static const int INVALID_PATH = 2005;      // 无效路径

    // 网络错误 (3000-3999)
    static const int NETWORK_ERROR = 3000;     // 网络错误
    static const int CONNECTION_FAILED = 3001; // 连接失败
    static const int CONNECTION_CLOSED = 3002; // 连接关闭
    static const int SEND_FAILED = 3003;       // 发送失败
    static const int RECEIVE_FAILED = 3004;    // 接收失败
    static const int BIND_FAILED = 3005;       // 绑定失败
    static const int LISTEN_FAILED = 3006;     // 监听失败
    static const int ACCEPT_FAILED = 3007;     // 接受失败
    static const int INVALID_SOCKET = 3008;     // 无效socket
    static const int SOCKET_TIMEOUT = 3009;    // socket超时

    // 线程错误 (4000-4999)
    static const int THREAD_ERROR = 4000;      // 线程错误
    static const int THREAD_START_FAILED = 4001; // 线程启动失败
    static const int THREAD_JOIN_FAILED = 4002; // 线程加入失败
    static const int DEADLOCK = 4003;          // 死锁
    static const int LOCK_FAILED = 4004;       // 锁失败
    static const int UNLOCK_FAILED = 4005;     // 解锁失败

    // 内存错误 (5000-5999)
    static const int MEMORY_ERROR = 5000;      // 内存错误
    static const int ALLOCATION_FAILED = 5001; // 分配失败
    static const int NULL_DELETE = 5002;        // 空删除
    static const int DOUBLE_FREE = 5003;       // 双重释放

    // JSON错误 (6000-6999)
    static const int JSON_ERROR = 6000;        // JSON错误
    static const int JSON_PARSE_ERROR = 6001;  // JSON解析错误
    static const int JSON_INVALID_TYPE = 6002;  // JSON类型错误
    static const int JSON_KEY_NOT_FOUND = 6003; // JSON键不存在

    // 配置错误 (7000-7999)
    static const int CONFIG_ERROR = 7000;      // 配置错误
    static const int CONFIG_PARSE_ERROR = 7001; // 配置解析错误
    static const int CONFIG_SAVE_ERROR = 7002;  // 配置保存错误
};
```

#### 2.2.2 ErrorCode 类

```cpp
class ErrorCode {
public:
    ErrorCode();
    ErrorCode(int code, const std::string& message);
    ErrorCode(int code, const std::string& message, const std::string& details);

    int code() const;
    const std::string& message() const;
    const std::string& details() const;
    bool isSuccess() const;

    std::string toString() const;
    bool operator==(const ErrorCode& other) const;
    bool operator!=(const ErrorCode& other) const;
};
```

#### 2.2.3 完整使用示例

```cpp
#include "util/error.h"
#include <iostream>

int main() {
    base::ErrorCode error1;  // 默认成功
    base::ErrorCode error2(base::ErrorCode::INVALID_ARGUMENT, "Invalid parameter");
    base::ErrorCode error3(base::ErrorCode::FILE_NOT_FOUND, "File not found", "config.ini");

    std::cout << error2.toString() << std::endl;  // "[Error] 1001: Invalid parameter"

    if (error3 == base::ErrorCode::FILE_NOT_FOUND) {
        std::cout << "File missing!" << std::endl;
    }

    return 0;
}
```

---

### 2.3 结果封装 (Result)

**头文件**: `include/util/result.h`
**命名空间**: `base`

#### 2.3.1 Result 类模板

```cpp
template <typename T, typename E = ErrorCode>
class Result {
public:
    Result();
    Result(const T& value);
    Result(T&& value);
    Result(const E& error);
    Result(E&& error);

    bool isSuccess() const;
    bool isError() const;

    const T& value() const;
    T& value();
    const E& error() const;
    E& error();

    int errorCode() const;
    std::string errorMessage() const;

    template <typename U>
    Result<U, E> map(const std::function<U(const T&)>& func) const;

    T getValueOr(const T& defaultValue) const;
    E getErrorOr(const E& defaultError) const;

private:
    bool m_isSuccess;
    union {
        T m_value;
        E m_error;
    };
};
```

#### 2.3.2 工厂方法

```cpp
// 创建成功结果
template <typename T, typename E>
Result<T, E> Result<T, E>::success(const T& value);

// 创建错误结果
template <typename T, typename E>
Result<T, E> Result<T, E>::failure(const E& error);

// 创建成功结果（隐式转换）
template <typename T, typename E>
Result<T, E> Result<T, E>::success(T&& value);

// 创建错误结果（隐式转换）
template <typename T, typename E>
Result<T, E> Result<T, E>::failure(E&& error);
```

#### 2.3.3 完整使用示例

```cpp
#include "util/result.h"
#include "util/error.h"
#include <iostream>

base::Result<int> divide(int a, int b) {
    if (b == 0) {
        return base::Result<int>::failure(
            base::ErrorCode(base::ErrorCode::INVALID_ARGUMENT, "Division by zero")
        );
    }
    return base::Result<int>::success(a / b);
}

int main() {
    auto result1 = divide(10, 2);
    if (result1.isSuccess()) {
        std::cout << "10 / 2 = " << result1.value() << std::endl;
    }

    auto result2 = divide(10, 0);
    if (result2.isError()) {
        std::cout << "Error: " << result2.errorMessage() << std::endl;
    }

    // 使用 map 转换结果
    auto result3 = divide(20, 4).map([](int v) { return v * 2; });
    if (result3.isSuccess()) {
        std::cout << "20 / 4 * 2 = " << result3.value() << std::endl;
    }

    return 0;
}
```

---

## 3. 内存管理 (Memory)

### 3.1 智能指针 (Smart Pointer)

**头文件**: `include/memory/smart_ptr.h`
**命名空间**: `base::mem`

#### 3.1.1 类型定义

```cpp
// SharedPtr - 共享所有权智能指针
template <typename T>
using SharedPtr = base::mem::SharedPtr<T>;

// UniquePtr - 独占所有权智能指针
template <typename T>
using UniquePtr = base::mem::UniquePtr<T>;

// WeakPtr - 弱引用智能指针
template <typename T>
using WeakPtr = base::mem::WeakPtr<T>;

// MakeShared - 创建SharedPtr
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args);

// MakeUnique - 创建UniquePtr
template <typename T, typename... Args>
UniquePtr<T> MakeUnique(Args&&... args);
```

#### 3.1.2 SharedPtr 共享指针

```cpp
template <typename T>
class SharedPtr {
public:
    SharedPtr();
    SharedPtr(std::nullptr_t);
    explicit SharedPtr(T* ptr);

    SharedPtr(const SharedPtr& other);
    SharedPtr(SharedPtr&& other) noexcept;

    SharedPtr& operator=(const SharedPtr& other);
    SharedPtr& operator=(SharedPtr&& other) noexcept;

    ~SharedPtr();

    T* get() const;
    T& operator*() const;
    T* operator->() const;

    long useCount() const;
    bool unique() const;
    bool expired() const;

    void reset();
    void reset(T* ptr);

    void swap(SharedPtr& other) noexcept;

    operator bool() const;
};
```

#### 3.1.3 UniquePtr 独占指针

```cpp
template <typename T>
class UniquePtr {
public:
    UniquePtr();
    UniquePtr(std::nullptr_t);
    explicit UniquePtr(T* ptr);

    UniquePtr(UniquePtr&& other) noexcept;
    UniquePtr& operator=(UniquePtr&& other) noexcept;

    ~UniquePtr();

    T* get() const;
    T& operator*() const;
    T* operator->() const;

    T* release();
    void reset();
    void reset(T* ptr);

    void swap(UniquePtr& other) noexcept;

    operator bool() const;
};
```

#### 3.1.4 WeakPtr 弱引用指针

```cpp
template <typename T>
class WeakPtr {
public:
    WeakPtr();
    WeakPtr(std::nullptr_t);
    WeakPtr(const SharedPtr<T>& shared);
    WeakPtr(const WeakPtr& other);
    WeakPtr(WeakPtr&& other) noexcept;

    WeakPtr& operator=(const SharedPtr<T>& shared);
    WeakPtr& operator=(const WeakPtr& other);
    WeakPtr& operator=(WeakPtr&& other) noexcept;

    long useCount() const;
    bool expired() const;

    SharedPtr<T> lock() const;

    void reset();
    void swap(WeakPtr& other) noexcept;
};
```

#### 3.1.5 完整使用示例

```cpp
#include "memory/smart_ptr.h"
#include <iostream>

class Resource {
public:
    Resource(int value) : m_value(value) {
        std::cout << "Resource created: " << m_value << std::endl;
    }
    ~Resource() {
        std::cout << "Resource destroyed: " << m_value << std::endl;
    }
    int getValue() const { return m_value; }
private:
    int m_value;
};

int main() {
    // 使用 MakeShared 创建共享指针
    auto sp1 = base::mem::MakeShared<Resource>(100);
    std::cout << "use_count: " << sp1.useCount() << std::endl;  // 1

    // 复制共享指针
    auto sp2 = sp1;
    std::cout << "use_count: " << sp1.useCount() << std::endl;  // 2

    // 使用 lock 获取有效的共享指针
    if (!sp2.expired()) {
        std::cout << "Value: " << sp2->getValue() << std::endl;
    }

    // 使用 WeakPtr
    base::mem::WeakPtr<Resource> wp = sp1;
    sp1.reset();
    if (auto locked = wp.lock()) {
        std::cout << "Still alive: " << locked->getValue() << std::endl;
    } else {
        std::cout << "Resource already destroyed" << std::endl;
    }

    // 使用 UniquePtr
    auto up = base::mem::MakeUnique<Resource>(200);
    std::cout << "Unique value: " << up->getValue() << std::endl;

    return 0;
}
```

---

## 4. 数据结构 (Data Structures)

### 4.1 JSON模块

**头文件**: `include/io/json.h`
**命名空间**: `base::io`

#### 4.1.1 JsonValue 类型

```cpp
using JsonValue = base::io::JsonValue;
using JsonObject = base::io::JsonObject;
using JsonArray = base::io::JsonArray;
```

#### 4.1.2 JsonValue 类

```cpp
class JsonValue {
public:
    enum class Type {
        NULL_TYPE,
        BOOL,
        NUMBER,
        STRING,
        ARRAY,
        OBJECT
    };

    // 构造函数
    JsonValue();
    JsonValue(std::nullptr_t);
    JsonValue(bool value);
    JsonValue(int value);
    JsonValue(double value);
    JsonValue(const char* value);
    JsonValue(const std::string& value);
    JsonValue(JsonArray array);
    JsonValue(JsonObject object);

    // 类型查询
    Type type() const;
    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    // 值访问
    bool asBool() const;
    int asInt() const;
    double asDouble() const;
    const std::string& asString() const;
    const JsonArray& asArray() const;
    JsonArray& asArray();
    const JsonObject& asObject() const;
    JsonObject& asObject();

    // 数组操作
    size_t size() const;
    JsonValue& operator[](size_t index);
    const JsonValue& operator[](size_t index) const;

    // 对象操作
    JsonValue& operator[](const std::string& key);
    const JsonValue& operator[](const std::string& key) const;
    bool hasKey(const std::string& key) const;
    void remove(const std::string& key);

    // 序列化
    std::string stringify() const;
    static JsonValue parse(const std::string& json);
};
```

#### 4.1.3 完整使用示例

```cpp
#include "io/json.h"
#include <iostream>

int main() {
    // 创建JSON对象
    base::io::JsonObject obj;
    obj["name"] = "John";
    obj["age"] = 30;
    obj["is_active"] = true;

    // 创建嵌套数组
    base::io::JsonArray hobbies;
    hobbies.append("reading");
    hobbies.append("coding");
    hobbies.append("gaming");
    obj["hobbies"] = hobbies;

    // 序列化
    std::string jsonStr = obj.stringify();
    std::cout << jsonStr << std::endl;

    // 解析
    auto parsed = base::io::JsonValue::parse(jsonStr);
    std::cout << "Name: " << parsed["name"].asString() << std::endl;
    std::cout << "Age: " << parsed["age"].asInt() << std::endl;
    std::cout << "First hobby: " << parsed["hobbies"][0].asString() << std::endl;

    // 访问不存在的键
    if (!parsed.hasKey("email")) {
        std::cout << "Email not found" << std::endl;
    }

    return 0;
}
```

---

### 4.2 XML模块

**头文件**: `include/io/xml.h`
**命名空间**: `base::io`

#### 4.2.1 XmlNode 类

```cpp
class XmlNode {
public:
    XmlNode();
    XmlNode(const std::string& tagName);

    // 属性操作
    void setAttribute(const std::string& name, const std::string& value);
    std::string getAttribute(const std::string& name) const;
    bool hasAttribute(const std::string& name) const;
    void removeAttribute(const std::string& name);

    // 子节点操作
    void appendChild(const XmlNode& child);
    void removeChild(const XmlNode& child);
    XmlNode* findChild(const std::string& tagName);
    const XmlNode* findChild(const std::string& tagName) const;

    // 内容操作
    std::string getTagName() const;
    void setTagName(const std::string& name);
    std::string getText() const;
    void setText(const std::string& text);

    // 序列化
    std::string toString(int indent = 0) const;
    static XmlNode parse(const std::string& xml);
};
```

#### 4.2.2 完整使用示例

```cpp
#include "io/xml.h"
#include <iostream>

int main() {
    // 创建XML文档
    base::io::XmlNode root("person");

    // 设置属性
    root.setAttribute("id", "12345");
    root.setAttribute("type", "admin");

    // 创建子节点
    base::io::XmlNode nameNode("name");
    nameNode.setText("John Doe");
    root.appendChild(nameNode);

    base::io::XmlNode ageNode("age");
    ageNode.setText("30");
    root.appendChild(ageNode);

    // 序列化
    std::string xmlStr = root.toString(2);
    std::cout << xmlStr << std::endl;

    // 解析
    auto parsed = base::io::XmlNode::parse(xmlStr);
    std::cout << "Tag: " << parsed.getTagName() << std::endl;
    std::cout << "ID: " << parsed.getAttribute("id") << std::endl;

    auto name = parsed.findChild("name");
    if (name) {
        std::cout << "Name: " << name->getText() << std::endl;
    }

    return 0;
}
```

---

## 5. 文件系统 (IO)

### 5.1 文件系统模块 (FileSystem)

**头文件**: `include/io/filesystem.h`
**命名空间**: `base::io`

#### 5.1.1 文件操作函数

```cpp
// 检查路径是否存在
bool exists(const std::string& path);

// 检查是否为文件
bool isFile(const std::string& path);

// 检查是否为目录
bool isDirectory(const std::string& path);

// 获取文件大小
size_t fileSize(const std::string& path);

// 读取文件内容
Result<std::string> readFile(const std::string& path);

// 写入文件内容
Result<void> writeFile(const std::string& path, const std::string& content);

// 追加文件内容
Result<void> appendFile(const std::string& path, const std::string& content);

// 删除文件
Result<void> removeFile(const std::string& path);

// 重命名文件
Result<void> renameFile(const std::string& oldPath, const std::string& newPath);

// 复制文件
Result<void> copyFile(const std::string& srcPath, const std::string& dstPath);

// 创建目录
Result<void> createDirectory(const std::string& path);

// 删除目录
Result<void> removeDirectory(const std::string& path);

// 列出目录内容
Result<std::vector<std::string>> listDirectory(const std::string& path);

// 获取当前工作目录
std::string getCurrentDirectory();

// 设置当前工作目录
bool setCurrentDirectory(const std::string& path);

// 获取文件扩展名
std::string getExtension(const std::string& path);

// 获取文件名
std::string getFileName(const std::string& path);

// 获取父目录
std::string getParentDirectory(const std::string& path);
```

#### 5.1.2 完整使用示例

```cpp
#include "io/filesystem.h"
#include <iostream>

int main() {
    std::string filename = "test.txt";

    // 写入文件
    auto writeResult = base::io::writeFile(filename, "Hello, BaseLib!");
    if (writeResult.isError()) {
        std::cerr << "Write failed: " << writeResult.errorMessage() << std::endl;
        return 1;
    }

    // 读取文件
    auto readResult = base::io::readFile(filename);
    if (readResult.isSuccess()) {
        std::cout << "Content: " << readResult.value() << std::endl;
    }

    // 获取文件信息
    if (base::io::exists(filename)) {
        std::cout << "File size: " << base::io::fileSize(filename) << std::endl;
        std::cout << "Extension: " << base::io::getExtension(filename) << std::endl;
        std::cout << "File name: " << base::io::getFileName(filename) << std::endl;
    }

    // 列出目录
    auto listResult = base::io::listDirectory(".");
    if (listResult.isSuccess()) {
        std::cout << "Files in current directory:" << std::endl;
        for (const auto& name : listResult.value()) {
            std::cout << "  " << name << std::endl;
        }
    }

    // 删除文件
    base::io::removeFile(filename);

    return 0;
}
```

---

## 6. 网络通信 (Network)

### 6.1 TCP模块

**头文件**: `include/net/tcp.h`
**命名空间**: `base::net`

#### 6.1.1 TcpSocket 类

```cpp
class TcpSocket {
public:
    TcpSocket();
    explicit TcpSocket(SOCKET socket);
    ~TcpSocket();

    // 连接操作
    Result<void> connect(const std::string& host, int port);
    Result<void> disconnect();

    // 发送数据
    Result<int> send(const void* buffer, int length);
    Result<int> send(const std::string& data);

    // 接收数据
    Result<int> receive(void* buffer, int length);
    Result<std::string> receive(int maxLength);

    // 配置
    void setTimeout(int timeoutMs);
    void setKeepAlive(bool enable);

    // 状态查询
    bool isConnected() const;
    std::string getRemoteAddress() const;
    int getRemotePort() const;
    std::string getLocalAddress() const;
    int getLocalPort() const;
};
```

#### 6.1.2 TcpServer 类

```cpp
class TcpServer {
public:
    TcpServer();
    ~TcpServer();

    // 监听
    Result<void> listen(int port, const std::string& host = "0.0.0.0");
    Result<void> stop();

    // 接受连接
    Result<TcpSocket> accept();

    // 配置
    void setTimeout(int timeoutMs);
    void setMaxConnections(int maxConnections);

    // 状态查询
    bool isRunning() const;
    int getPort() const;
};
```

#### 6.1.3 完整使用示例

**客户端**:

```cpp
#include "net/tcp.h"
#include <iostream>

int main() {
    base::net::TcpSocket client;

    auto result = client.connect("example.com", 80);
    if (result.isError()) {
        std::cerr << "Connection failed: " << result.errorMessage() << std::endl;
        return 1;
    }

    std::string request = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";
    client.send(request);

    auto response = client.receive(4096);
    if (response.isSuccess()) {
        std::cout << response.value() << std::endl;
    }

    client.disconnect();
    return 0;
}
```

**服务端**:

```cpp
#include "net/tcp.h"
#include <iostream>

int main() {
    base::net::TcpServer server;

    auto result = server.listen(8080, "0.0.0.0");
    if (result.isError()) {
        std::cerr << "Listen failed: " << result.errorMessage() << std::endl;
        return 1;
    }

    std::cout << "Server listening on port 8080..." << std::endl;

    while (true) {
        auto clientResult = server.accept();
        if (clientResult.isSuccess()) {
            auto& client = clientResult.value();
            std::cout << "Client connected from "
                      << client.getRemoteAddress() << ":"
                      << client.getRemotePort() << std::endl;

            auto data = client.receive(1024);
            if (data.isSuccess()) {
                std::cout << "Received: " << data.value() << std::endl;
                client.send("HTTP/1.1 200 OK\r\n\r\nHello!");
            }
            client.disconnect();
        }
    }

    server.stop();
    return 0;
}
```

---

### 6.2 UDP模块

**头文件**: `include/net/udp.h`
**命名空间**: `base::net`

#### 6.2.1 UdpSocket 类

```cpp
class UdpSocket {
public:
    UdpSocket();
    ~UdpSocket();

    // 绑定端口
    Result<void> bind(int port);
    Result<void> unbind();

    // 发送数据
    Result<int> sendTo(const void* buffer, int length,
                       const std::string& destHost, int destPort);
    Result<int> sendTo(const std::string& data,
                       const std::string& destHost, int destPort);

    // 接收数据
    Result<int> receiveFrom(void* buffer, int length,
                           std::string& srcHost, int& srcPort);
    Result<std::string> receiveFrom(int maxLength,
                                    std::string& srcHost, int& srcPort);

    // 广播
    Result<int> broadcast(const void* buffer, int length, int port);
    Result<int> broadcast(const std::string& data, int port);

    // 配置
    void setTimeout(int timeoutMs);
    void setBroadcast(bool enable);
};
```

#### 6.2.2 完整使用示例

```cpp
#include "net/udp.h"
#include <iostream>

int main() {
    base::net::UdpSocket socket;

    // 绑定端口
    auto bindResult = socket.bind(9000);
    if (bindResult.isError()) {
        std::cerr << "Bind failed: " << bindResult.errorMessage() << std::endl;
        return 1;
    }

    // 设置超时
    socket.setTimeout(5000);

    // 发送数据
    socket.sendTo("Hello, UDP server!", "127.0.0.1", 9001);

    // 接收数据
    std::string srcHost;
    int srcPort;
    auto data = socket.receiveFrom(1024, srcHost, srcPort);

    if (data.isSuccess()) {
        std::cout << "From " << srcHost << ":" << srcPort
                  << ": " << data.value() << std::endl;
    }

    socket.unbind();
    return 0;
}
```

---

### 6.3 HTTP模块

**头文件**: `include/net/http.h`
**命名空间**: `base::net`

#### 6.3.1 HttpRequest 类

```cpp
class HttpRequest {
public:
    HttpRequest();
    explicit HttpRequest(const std::string& url);

    void setMethod(const std::string& method);
    void setUrl(const std::string& url);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);

    std::string getMethod() const;
    std::string getUrl() const;
    std::string getHeader(const std::string& name) const;
    std::string getBody() const;
    std::string toString() const;
};
```

#### 6.3.2 HttpResponse 类

```cpp
class HttpResponse {
public:
    HttpResponse();
    explicit HttpResponse(const std::string& rawResponse);

    int getStatusCode() const;
    std::string getStatusMessage() const;
    std::string getHeader(const std::string& name) const;
    std::string getBody() const;
    std::string toString() const;
};
```

#### 6.3.3 HttpClient 类

```cpp
class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    Result<HttpResponse> get(const std::string& url);
    Result<HttpResponse> post(const std::string& url, const std::string& body = "");
    Result<HttpResponse> put(const std::string& url, const std::string& body = "");
    Result<HttpResponse> del(const std::string& url);
    Result<HttpResponse> request(const HttpRequest& request);

    void setTimeout(int timeoutMs);
    void setHeader(const std::string& name, const std::string& value);
};
```

#### 6.3.4 完整使用示例

```cpp
#include "net/http.h"
#include <iostream>

int main() {
    base::net::HttpClient client;
    client.setTimeout(10000);

    // GET请求
    auto getResult = client.get("http://example.com/api/data");
    if (getResult.isSuccess()) {
        auto& response = getResult.value();
        std::cout << "Status: " << response.getStatusCode() << std::endl;
        std::cout << "Body: " << response.getBody() << std::endl;
    }

    // POST请求
    base::net::HttpRequest postReq("http://example.com/api/submit");
    postReq.setMethod("POST");
    postReq.setBody("{\"name\": \"test\"}");

    auto postResult = client.request(postReq);
    if (postResult.isSuccess()) {
        std::cout << "POST Response: " << postResult.value().getBody() << std::endl;
    }

    return 0;
}
```

---

### 6.4 HTTP客户端模块 (HttpClient)

**头文件**: `include/net/http_client.h`
**命名空间**: `base::net`

#### 6.4.1 核心接口

```cpp
class IHttpClient {
public:
    virtual ~IHttpClient() = default;

    virtual Result<HttpResponse> get(const std::string& url, RequestConfig config = RequestConfig()) = 0;
    virtual Result<HttpResponse> post(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) = 0;
    virtual Result<HttpResponse> put(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) = 0;
    virtual Result<HttpResponse> del(const std::string& url, RequestConfig config = RequestConfig()) = 0;
    virtual Result<HttpResponse> patch(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) = 0;
    virtual Result<HttpResponse> request(const HttpRequest& request, RequestConfig config = RequestConfig()) = 0;

    virtual void addInterceptor(std::shared_ptr<IInterceptor> interceptor) = 0;
    virtual void removeInterceptor(IInterceptor* interceptor) = 0;
    virtual void clearInterceptors() = 0;

    virtual void setDefaultHeader(const std::string& key, const std::string& value) = 0;
    virtual void removeDefaultHeader(const std::string& key) = 0;
    virtual void clearDefaultHeaders() = 0;

    virtual RequestConfig getDefaultConfig() const = 0;
    virtual void setDefaultConfig(const RequestConfig& config) = 0;
};
```

#### 6.4.2 HttpClient 实现类

```cpp
class HttpClient : public IHttpClient {
public:
    HttpClient();
    ~HttpClient() override;

    Result<HttpResponse> get(const std::string& url, RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> post(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> put(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> del(const std::string& url, RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> patch(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> request(const HttpRequest& request, RequestConfig config = RequestConfig()) override;

    void addInterceptor(std::shared_ptr<IInterceptor> interceptor) override;
    void removeInterceptor(IInterceptor* interceptor) override;
    void clearInterceptors() override;

    void setDefaultHeader(const std::string& key, const std::string& value) override;
    void removeDefaultHeader(const std::string& key) override;
    void clearDefaultHeaders() override;

    RequestConfig getDefaultConfig() const override;
    void setDefaultConfig(const RequestConfig& config) override;

    static std::string urlEncode(const std::string& value);
    static std::string urlDecode(const std::string& value);
};
```

#### 6.4.3 HttpRequest 类

```cpp
class HttpRequest : public IHttpRequest {
public:
    HttpRequest();

    HttpMethod method() const override;
    const std::string& url() const override;
    const std::string& path() const override;
    const std::map<std::string, std::string>& headers() const override;
    const std::string& body() const override;
    const std::map<std::string, std::string>& queryParams() const override;
    CancellationToken* cancellationToken() const override;
    void* tag() const override;

    HttpRequest& setMethod(HttpMethod method);
    HttpRequest& setUrl(const std::string& url);
    HttpRequest& setPath(const std::string& path);
    HttpRequest& setHeader(const std::string& key, const std::string& value);
    HttpRequest& setBody(const std::string& body);
    HttpRequest& setQueryParam(const std::string& key, const std::string& value);
    HttpRequest& setCancellationToken(CancellationToken* token);
    HttpRequest& setTag(void* tag);
};
```

#### 6.4.4 HttpResponse 类

```cpp
class HttpResponse : public IHttpResponse {
public:
    HttpResponse();

    int statusCode() const override;
    const std::string& statusMessage() const override;
    const std::string& body() const override;
    const std::string& header(const std::string& name) const override;
    std::map<std::string, std::string> headers() const override;
    bool isSuccess() const override;
    bool isClientError() const override;
    bool isServerError() const override;

    void setStatusCode(int code) override;
    void setStatusMessage(const std::string& msg) override;
    void setBody(const std::string& body) override;
    void addHeader(const std::string& name, const std::string& value) override;
};
```

#### 6.4.5 IInterceptor 拦截器接口

```cpp
class IInterceptor {
public:
    virtual ~IInterceptor() = default;

    virtual bool onRequest(IHttpRequest& request) = 0;
    virtual bool onResponse(IHttpRequest& request, IHttpResponse& response) = 0;
    virtual bool onError(IHttpRequest& request, const ErrorCode& error) = 0;
};
```

#### 6.4.6 RequestConfig 请求配置

```cpp
struct RequestConfig {
    int timeout_ms = 30000;           // 超时时间（毫秒）
    bool follow_redirects = true;      // 是否跟随重定向
    int max_redirects = 3;             // 最大重定向次数
    std::string user_agent;           // 用户代理
    std::string proxy_host;           // 代理主机
    int proxy_port = 0;               // 代理端口
    bool verify_ssl = true;           // 是否验证SSL
    int max_retries = 0;              // 最大重试次数
    int retry_delay_ms = 1000;        // 重试延迟（毫秒）
};
```

#### 6.4.7 CancellationToken 取消令牌

```cpp
class CancellationToken {
public:
    CancellationToken();
    CancellationToken(const CancellationToken& other);

    void cancel();
    bool isCancelled() const;
    void reset();
};
```

#### 6.4.8 MockHttpClient 测试模拟客户端

```cpp
class MockHttpClient : public IHttpClient {
public:
    MockHttpClient();

    void setMockResponse(const HttpResponse& response);
    void setMockError(const ErrorCode& error);
    void clearMock();

    int getRequestCount() const;
    HttpRequest getLastRequest() const;

    // ... 继承接口方法
};
```

#### 6.4.9 辅助函数

```cpp
std::string HttpMethodToString(HttpMethod method);
HttpMethod StringToHttpMethod(const std::string& method);
```

#### 6.4.10 完整使用示例

```cpp
#include "net/http_client.h"
#include <iostream>

using namespace base::net;

class LoggingInterceptor : public IInterceptor {
public:
    bool onRequest(IHttpRequest& request) override {
        std::cout << "Request: " << HttpMethodToString(request.method())
                  << " " << request.url() << std::endl;
        return true;
    }

    bool onResponse(IHttpRequest& request, IHttpResponse& response) override {
        std::cout << "Response: " << response.statusCode()
                  << " for " << request.url() << std::endl;
        return true;
    }

    bool onError(IHttpRequest& request, const base::ErrorCode& error) override {
        std::cout << "Error: " << error.toString()
                  << " for " << request.url() << std::endl;
        return true;
    }
};

int main() {
    HttpClient client;

    client.addInterceptor(std::make_shared<LoggingInterceptor>());
    client.setDefaultHeader("Authorization", "Bearer token123");

    RequestConfig config;
    config.timeout_ms = 5000;

    auto result = client.get("http://example.com/api/users", config);
    if (result.isSuccess()) {
        std::cout << "Response: " << result.value().body() << std::endl;
    }

    auto postResult = client.post("http://example.com/api/users",
                                   "{\"name\": \"John\"}");
    if (postResult.isSuccess()) {
        std::cout << "Created: " << postResult.value().body() << std::endl;
    }

    CancellationToken token;
    HttpRequest request;
    request.setUrl("http://example.com/api/slow")
           .setCancellationToken(&token);

    token.cancel();

    MockHttpClient mockClient;
    HttpResponse mockResponse;
    mockResponse.setStatusCode(200);
    mockResponse.setBody("{\"result\": \"mocked\"}");
    mockClient.setMockResponse(mockResponse);

    auto mockResult = mockClient.get("http://test.com/api");
    if (mockResult.isSuccess()) {
        std::cout << "Mock response: " << mockResult.value().body() << std::endl;
    }

    return 0;
}
```

---

### 6.5 HTTP服务端模块 (HttpServer)

**头文件**: `include/net/http_server.h`
**命名空间**: `base::net`

#### 6.5.1 核心接口

```cpp
class IHttpServer {
public:
    virtual ~IHttpServer() = default;

    virtual Result<void> start() = 0;
    virtual Result<void> stop(int timeout_ms = 5000) = 0;
    virtual bool isRunning() const = 0;

    virtual void get(const std::string& path, HttpHandler handler) = 0;
    virtual void post(const std::string& path, HttpHandler handler) = 0;
    virtual void put(const std::string& path, HttpHandler handler) = 0;
    virtual void del(const std::string& path, HttpHandler handler) = 0;
    virtual void patch(const std::string& path, HttpHandler handler) = 0;
    virtual void options(const std::string& path, HttpHandler handler) = 0;
    virtual void head(const std::string& path, HttpHandler handler) = 0;
    virtual void any(const std::string& path, HttpHandler handler) = 0;

    virtual void use(MiddlewareHandler middleware) = 0;
    virtual void use(const std::string& path, MiddlewareHandler middleware) = 0;

    virtual ServerConfig getConfig() const = 0;
    virtual void setConfig(const ServerConfig& config) = 0;
};
```

#### 6.5.2 HttpServer 实现类

```cpp
class HttpServer : public IHttpServer {
public:
    explicit HttpServer(const ServerConfig& config = ServerConfig());
    ~HttpServer() override;

    Result<void> start() override;
    Result<void> stop(int timeout_ms = 5000) override;
    bool isRunning() const override;

    void get(const std::string& path, HttpHandler handler) override;
    void post(const std::string& path, HttpHandler handler) override;
    void put(const std::string& path, HttpHandler handler) override;
    void del(const std::string& path, HttpHandler handler) override;
    void patch(const std::string& path, HttpHandler handler) override;
    void options(const std::string& path, HttpHandler handler) override;
    void head(const std::string& path, HttpHandler handler) override;
    void any(const std::string& path, HttpHandler handler) override;

    void use(MiddlewareHandler middleware) override;
    void use(const std::string& path, MiddlewareHandler middleware) override;

    ServerConfig getConfig() const override;
    void setConfig(const ServerConfig& config) override;
};
```

#### 6.5.3 数据结构

```cpp
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    OPTIONS,
    HEAD,
    ANY
};

enum class HttpStatus {
    OK = 200,
    CREATED = 201,
    NO_CONTENT = 204,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    SERVICE_UNAVAILABLE = 503
};

struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::string query_string;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
    std::map<std::string, std::string> path_params;
    std::string body;
    std::string remote_addr;
    int remote_port;
    uint64_t request_id;
};

struct HttpResponse {
    HttpStatus status;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string content_type;

    HttpResponse();

    void setStatus(HttpStatus s);
    void setBody(const std::string& b);
    void setContentType(const std::string& ct);
    void setHeader(const std::string& key, const std::string& value);
};

struct ServerConfig {
    int port = 8080;
    std::string host = "0.0.0.0";
    int num_threads = 4;
    int connection_timeout_ms = 30000;
    int max_request_size = 10 * 1024 * 1024;
    int max_keepalive_requests = 100;
    bool enable_cors = false;
    std::string cors_allow_origin = "*";
    std::string log_dir = "logs";
    bool enable_logging = true;
};

using HttpHandler = std::function<void(const HttpRequest&, HttpResponse&)>;
using MiddlewareHandler = std::function<bool(const HttpRequest&, HttpResponse&)>;
```

#### 6.5.4 辅助函数

```cpp
std::string HttpMethodToString(HttpMethod method);
HttpMethod StringToHttpMethod(const std::string& method);
std::string HttpStatusToString(HttpStatus status);
int HttpStatusToCode(HttpStatus status);
```

#### 6.5.5 完整使用示例

```cpp
#include "net/http_server.h"
#include "core/logger.h"
#include <iostream>

using namespace base::net;

int main() {
    base::log::Logger::init("logs");

    ServerConfig config;
    config.port = 8080;
    config.host = "0.0.0.0";
    config.num_threads = 4;
    config.enable_cors = true;

    HttpServer server(config);

    server.use([](const HttpRequest& req, HttpResponse& res) {
        std::cout << "Middleware: " << req.path << std::endl;
        return true;
    });

    server.get("/api/health", [](const HttpRequest& req, HttpResponse& res) {
        res.setStatus(HttpStatus::OK);
        res.setBody("{\"status\": \"healthy\"}");
        res.setContentType("application/json");
    });

    server.post("/api/users", [](const HttpRequest& req, HttpResponse& res) {
        res.setStatus(HttpStatus::CREATED);
        res.setBody("{\"id\": 1, \"name\": \"User\"}");
        res.setContentType("application/json");
    });

    server.put("/api/users/:id", [](const HttpRequest& req, HttpResponse& res) {
        res.setStatus(HttpStatus::OK);
        res.setBody("{\"id\": 1, \"updated\": true}");
        res.setContentType("application/json");
    });

    server.del("/api/users/:id", [](const HttpRequest& req, HttpResponse& res) {
        res.setStatus(HttpStatus::NO_CONTENT);
        res.setBody("");
    });

    auto result = server.start();
    if (result.isSuccess()) {
        std::cout << "Server started on port 8080" << std::endl;
        std::cout << "Press Enter to stop..." << std::endl;
        std::cin.get();
        server.stop();
    } else {
        std::cerr << "Failed to start server: " << result.error().toString() << std::endl;
    }

    base::log::Logger::shutdown();
    return 0;
}
```

---

## 7. 并发编程 (Concurrency)

### 7.1 线程模块 (Thread)

**头文件**: `include/util/thread.h`
**命名空间**: `base::util`

#### 7.1.1 Thread 类

```cpp
class Thread {
public:
    using ThreadId = uint64_t;
    using ThreadFunc = std::function<void()>;

    Thread();
    explicit Thread(ThreadFunc func);
    ~Thread();

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    Thread(Thread&& other) noexcept;
    Thread& operator=(Thread&& other) noexcept;

    bool start();
    bool stop(int timeoutMs = 5000);
    bool pause();
    bool resume();
    bool join(int timeoutMs = 0);
    bool detach();

    ThreadId getId() const;
    bool isRunning() const;
    bool isPaused() const;
    bool isJoinable() const;

    static ThreadId getCurrentThreadId();
    static void sleep(int milliseconds);
    static void yield();
};
```

#### 7.1.2 ThreadGuard RAII封装

```cpp
class ThreadGuard {
public:
    explicit ThreadGuard(Thread& thread);
    ~ThreadGuard();

    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};
```

#### 7.1.3 AutoJoin RAII封装

```cpp
class AutoJoin {
public:
    explicit AutoJoin(Thread& thread);
    ~AutoJoin();

    AutoJoin(const AutoJoin&) = delete;
    AutoJoin& operator=(const AutoJoin&) = delete;
};
```

#### 7.1.4 完整使用示例

```cpp
#include "util/thread.h"
#include <iostream>
#include <chrono>

using namespace base::util;

void workerFunction(int taskId) {
    std::cout << "Task " << taskId << " started on thread "
              << Thread::getCurrentThreadId() << std::endl;

    Thread::sleep(100);
    std::cout << "Task " << taskId << " completed" << std::endl;
}

int main() {
    Thread thread1([]() { workerFunction(1); });
    Thread thread2([]() { workerFunction(2); });

    {
        AutoJoin guard1(thread1);
        AutoJoin guard2(thread2);

        thread1.start();
        thread2.start();

        std::cout << "All tasks started, waiting for completion..." << std::endl;
    }

    std::cout << "All threads completed" << std::endl;
    return 0;
}
```

---

### 7.2 线程池模块 (ThreadPool)

**头文件**: `include/util/thread_pool.h`
**命名空间**: `base::util`

#### 7.2.1 ThreadPool 类

```cpp
class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = 4);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename F, typename... Args>
    auto submit(F&& func, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    void shutdown();
    bool isRunning() const;
    size_t getThreadCount() const;
    size_t getTaskCount() const;
};
```

#### 7.2.2 完整使用示例

```cpp
#include "util/thread_pool.h"
#include <iostream>
#include <vector>

using namespace base::util;

int main() {
    ThreadPool pool(4);

    std::vector<std::future<int>> results;

    for (int i = 0; i < 10; ++i) {
        results.emplace_back(pool.submit([i]() {
            return i * i;
        }));
    }

    std::cout << "Submitted 10 tasks" << std::endl;

    for (auto& result : results) {
        std::cout << "Result: " << result.get() << std::endl;
    }

    pool.shutdown();
    std::cout << "ThreadPool shutdown complete" << std::endl;

    return 0;
}
```

---

### 7.3 锁抽象 (Lock)

**头文件**: `include/util/lock.h`
**命名空间**: `base::util`

#### 7.3.1 ILock 接口

```cpp
class ILock {
public:
    virtual ~ILock() = default;
    virtual void lock() = 0;
    virtual bool tryLock() = 0;
    virtual void unlock() = 0;
};
```

#### 7.3.2 RecursiveMutex 递归互斥锁

```cpp
class RecursiveMutex : public ILock {
public:
    RecursiveMutex();
    ~RecursiveMutex() override;

    void lock() override;
    bool tryLock() override;
    void unlock() override;
};
```

#### 7.3.3 NonRecursiveMutex 非递归互斥锁

```cpp
class NonRecursiveMutex : public ILock {
public:
    NonRecursiveMutex();
    ~NonRecursiveMutex() override;

    void lock() override;
    bool tryLock() override;
    void unlock() override;
};
```

#### 7.3.4 ReadWriteLock 读写锁

```cpp
class ReadWriteLock {
public:
    ReadWriteLock();
    ~ReadWriteLock();

    void lockRead();
    void lockWrite();
    bool tryLockRead();
    bool tryLockWrite();
    void unlock();
};
```

#### 7.3.5 LockGuard RAII锁封装

```cpp
template <typename T>
class LockGuard {
public:
    explicit LockGuard(T& lock);
    ~LockGuard();

    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;

private:
    T& m_lock;
};
```

#### 7.3.6 TryLockGuard 尝试锁RAII封装

```cpp
template <typename T>
class TryLockGuard {
public:
    explicit TryLockGuard(T& lock);
    ~TryLockGuard();

    bool isLocked() const;
    explicit operator bool() const;

    TryLockGuard(const TryLockGuard&) = delete;
    TryLockGuard& operator=(const TryLockGuard&) = delete;

private:
    T& m_lock;
    bool m_locked;
};
```

#### 7.3.7 LockMonitor 锁监控器

```cpp
template <typename T>
class LockMonitor {
public:
    explicit LockMonitor(T& lock) : m_lock(lock) {
        m_lock.lock();
    }

    ~LockMonitor() {
        m_lock.unlock();
    }

    T& getLock() { return m_lock; }

private:
    T& m_lock;
};
```

#### 7.3.8 完整使用示例

```cpp
#include "util/lock.h"
#include "util/thread.h"
#include <iostream>
#include <vector>

using namespace base::util;

class Counter {
public:
    void increment() {
        LockGuard<RecursiveMutex> guard(m_mutex);
        ++m_count;
    }

    int get() const {
        LockGuard<RecursiveMutex> guard(m_mutex);
        return m_count;
    }

    void incrementRead() {
        m_rwLock.lockRead();
        int value = m_readCount;
        Thread::sleep(10);
        m_readCount = value + 1;
        m_rwLock.unlock();
    }

    void incrementWrite() {
        m_rwLock.lockWrite();
        ++m_count;
        m_rwLock.unlock();
    }

private:
    mutable RecursiveMutex m_mutex;
    int m_count = 0;
    ReadWriteLock m_rwLock;
    int m_readCount = 0;
};

int main() {
    Counter counter;

    std::vector<Thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < 100; ++j) {
                counter.increment();
            }
        });
    }

    for (auto& thread : threads) {
        thread.start();
        thread.join();
    }

    std::cout << "Final count: " << counter.get() << std::endl;
    return 0;
}
```

---

## 8. 工具模块 (Utilities)

### 8.1 配置管理 (Config)

**头文件**: `include/util/config.h`
**命名空间**: `base::util`

#### 8.1.1 Config 类

```cpp
class Config {
public:
    Config();

    static Config load(const std::string& filePath);
    bool save(const std::string& filePath) const;
    std::string toJson() const;

    template <typename T>
    T get(const std::string& key, const T& defaultValue = T()) const;

    template <typename T>
    void set(const std::string& key, const T& value);

    bool has(const std::string& key) const;
    void remove(const std::string& key);
    void clear();

    class Exception;
};
```

#### 8.1.2 Exception 异常类

```cpp
class Config::Exception : public std::exception {
public:
    Exception(int code, const std::string& message);
    int code() const;
    const char* what() const noexcept override;
};
```

#### 8.1.3 完整使用示例

```cpp
#include "util/config.h"
#include <iostream>

int main() {
    try {
        auto config = base::util::Config::load("app.ini");

        std::string host = config.get<std::string>("database.host", "localhost");
        int port = config.get<int>("database.port", 3306);
        std::string user = config.get<std::string>("database.username", "root");

        std::cout << "Database: " << host << ":" << port << std::endl;
        std::cout << "User: " << user << std::endl;

        if (config.has("app.debug")) {
            bool debug = config.get<bool>("app.debug", false);
            std::cout << "Debug mode: " << (debug ? "ON" : "OFF") << std::endl;
        }

        config.set("database.host", "new-host.example.com");
        config.set("app.max_connections", 200);
        config.save("app_modified.ini");

        std::string json = config.toJson();
        std::cout << json << std::endl;

        config.remove("app.timeout");

    } catch (const base::util::Config::Exception& e) {
        std::cerr << "Config error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

---

### 8.2 时间工具 (Time)

**头文件**: `include/util/time.h`
**命名空间**: `base::util`

#### 8.2.1 Time 类

```cpp
class Time {
public:
    static int64_t timestamp();
    static int64_t timestampMillis();
    static std::string format(const std::string& formatStr);
    static std::string getCurrentDate();
    static std::string getCurrentTime();
};
```

#### 8.2.2 Time::Timer 类

```cpp
class Time::Timer {
public:
    Timer();
    void reset();
    double elapsedSeconds() const;
    int64_t elapsedMilliseconds() const;
};
```

#### 8.2.3 完整使用示例

```cpp
#include "util/time.h"
#include <iostream>

using namespace base::util;

int main() {
    std::cout << "Current timestamp: " << Time::timestamp() << std::endl;
    std::cout << "Current timestamp (ms): " << Time::timestampMillis() << std::endl;

    std::cout << "Current date: " << Time::getCurrentDate() << std::endl;
    std::cout << "Current time: " << Time::getCurrentTime() << std::endl;

    std::string formatted = Time::format("%Y-%m-%d %H:%M:%S");
    std::cout << "Formatted: " << formatted << std::endl;

    Time::Timer timer;
    timer.reset();

    for (volatile int i = 0; i < 1000000; ++i) {}

    std::cout << "Elapsed: " << timer.elapsedMilliseconds() << " ms" << std::endl;

    return 0;
}
```

---

## 9. 错误码参考

### 9.1 通用错误 (1000-1999)

| 错误码 | 常量名 | 说明 |
|-------|--------|------|
| 0 | SUCCESS | 成功 |
| 1000 | UNKNOWN_ERROR | 未知错误 |
| 1001 | INVALID_ARGUMENT | 无效参数 |
| 1002 | NULL_POINTER | 空指针 |
| 1003 | INVALID_STATE | 无效状态 |
| 1004 | TIMEOUT | 操作超时 |
| 1005 | NOT_IMPLEMENTED | 未实现 |

### 9.2 IO错误 (2000-2999)

| 错误码 | 常量名 | 说明 |
|-------|--------|------|
| 2000 | IO_ERROR | IO错误 |
| 2001 | FILE_NOT_FOUND | 文件不存在 |
| 2002 | PERMISSION_DENIED | 权限拒绝 |
| 2003 | FILE_ALREADY_EXISTS | 文件已存在 |
| 2004 | DIRECTORY_NOT_FOUND | 目录不存在 |
| 2005 | INVALID_PATH | 无效路径 |

### 9.3 网络错误 (3000-3999)

| 错误码 | 常量名 | 说明 |
|-------|--------|------|
| 3000 | NETWORK_ERROR | 网络错误 |
| 3001 | CONNECTION_FAILED | 连接失败 |
| 3002 | CONNECTION_CLOSED | 连接关闭 |
| 3003 | SEND_FAILED | 发送失败 |
| 3004 | RECEIVE_FAILED | 接收失败 |
| 3005 | BIND_FAILED | 绑定失败 |
| 3006 | LISTEN_FAILED | 监听失败 |
| 3007 | ACCEPT_FAILED | 接受失败 |
| 3008 | INVALID_SOCKET | 无效socket |
| 3009 | SOCKET_TIMEOUT | socket超时 |

### 9.4 线程错误 (4000-4999)

| 错误码 | 常量名 | 说明 |
|-------|--------|------|
| 4000 | THREAD_ERROR | 线程错误 |
| 4001 | THREAD_START_FAILED | 线程启动失败 |
| 4002 | THREAD_JOIN_FAILED | 线程加入失败 |
| 4003 | DEADLOCK | 死锁 |
| 4004 | LOCK_FAILED | 锁失败 |
| 4005 | UNLOCK_FAILED | 解锁失败 |

### 9.5 内存错误 (5000-5999)

| 错误码 | 常量名 | 说明 |
|-------|--------|------|
| 5000 | MEMORY_ERROR | 内存错误 |
| 5001 | ALLOCATION_FAILED | 分配失败 |
| 5002 | NULL_DELETE | 空删除 |
| 5003 | DOUBLE_FREE | 双重释放 |

### 9.6 JSON错误 (6000-6999)

| 错误码 | 常量名 | 说明 |
|-------|--------|------|
| 6000 | JSON_ERROR | JSON错误 |
| 6001 | JSON_PARSE_ERROR | JSON解析错误 |
| 6002 | JSON_INVALID_TYPE | JSON类型错误 |
| 6003 | JSON_KEY_NOT_FOUND | JSON键不存在 |

### 9.7 配置错误 (7000-7999)

| 错误码 | 常量名 | 说明 |
|-------|--------|------|
| 7000 | CONFIG_ERROR | 配置错误 |
| 7001 | CONFIG_PARSE_ERROR | 配置解析错误 |
| 7002 | CONFIG_SAVE_ERROR | 配置保存错误 |

---

## 10. 跨平台注意事项

### 10.1 Windows平台

```cpp
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#endif
```

**注意事项**:
- Windows宏与HTTP方法名冲突 (`DELETE`, `PATCH`, `OPTIONS`)
- 需要在包含头文件后使用 `#undef` 解除宏定义
- 网络初始化需要调用 `WSAStartup()`

### 10.2 Linux平台

```cpp
#ifdef __linux__
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif
```

**注意事项**:
- socket类型为 `int`，而非 `SOCKET`
- `INVALID_SOCKET` 值为 `-1`

---

## 11. 编译和链接

### 11.1 CMake 构建

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyProject)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(path/to/BaseLib)

target_link_libraries(MyProject PRIVATE BaseLib)
```

### 11.2 MSVC 编译

```batch
cl /EHsc /std:c++11 /Iinclude your_source.cpp /link /LIBPATH:lib baselib.lib
```

### 11.3 GCC/Clang 编译

```bash
g++ -std=c++11 -Iinclude your_source.cpp -L./lib -lbaselib -pthread -o your_program
```

---

## 12. 最佳实践

### 12.1 错误处理

```cpp
auto result = someFunction();
if (result.isError()) {
    BASE_LOG_ERROR("Module", "Operation failed: " + result.errorMessage());
    return result.errorCode();
}
```

### 12.2 线程安全

```cpp
class ThreadSafeClass {
private:
    mutable Mutex m_mutex;
    int m_data;

public:
    int getData() const {
        LockGuard<Mutex> guard(m_mutex);
        return m_data;
    }
};
```

### 12.3 资源管理

```cpp
void example() {
    auto file = base::io::readFile("data.txt");
    if (file.isSuccess()) {
        process(file.value());
    }
}
```

### 12.4 日志使用

```cpp
BASE_LOG_INFO("Module", "Starting operation");
BASE_LOG_DEBUG("Module", "Variable value: " + std::to_string(value));
BASE_LOG_WARN("Module", "Low memory warning");
BASE_LOG_ERROR("Module", "Operation failed with error: " + error.toString());
```

---

## 附录

### A. 库文件位置

| 平台 | 静态库 | 动态库 |
|------|--------|--------|
| Windows x64 | lib/BaseLib-x64.lib | bin/BaseLib-x64.dll |
| Windows x86 | lib/BaseLib-win32.lib | bin/BaseLib-win32.dll |
| Linux | lib/libBaseLib.a | lib/libBaseLib.so |
| macOS | lib/libBaseLib.a | lib/libBaseLib.dylib |

### B. 联系方式

- 项目主页: https://github.com/example/BaseLib
- 问题反馈: https://github.com/example/BaseLib/issues
