# C++基础库程序架构文档

## 1. 系统总体架构

### 1.1 架构层次

C++基础库采用分层架构设计，从上到下分为以下层次：

```
┌────────────────────────────────────────────────────────────┐
│                        应用层                             │
├────────────────────────────────────────────────────────────┤
│                        工具层                             │
│  ┌───────────┐  ┌───────────┐  ┌───────────┐  ┌───────────┐ │
│  │  配置管理  │  │  错误处理  │  │  线程池   │  │  时间工具  │ │
│  └───────────┘  └───────────┘  └───────────┘  └───────────┘ │
├────────────────────────────────────────────────────────────┤
│                        网络层                             │
│  ┌───────────┐  ┌───────────┐                             │
│  │   TCP模块  │  │   UDP模块  │                             │
│  └───────────┘  └───────────┘                             │
├────────────────────────────────────────────────────────────┤
│                        核心层                             │
│  ┌───────────┐  ┌───────────┐  ┌───────────┐              │
│  │  日志系统  │  │  文件IO   │  │  智能指针  │              │
│  └───────────┘  └───────────┘  └───────────┘              │
├────────────────────────────────────────────────────────────┤
│                        基础层                             │
│  ┌───────────┐  ┌───────────┐  ┌───────────┐              │
│  │  平台抽象  │  │  类型定义  │  │  工具函数  │              │
│  └───────────┘  └───────────┘  └───────────┘              │
└────────────────────────────────────────────────────────────┘
```

### 1.2 模块依赖关系

```
工具层 ────────────────┐
    ↑                 │
    │                 │
网络层 ───────────────┘
    ↑
    │
核心层 ────────────────┐
    ↑                 │
    │                 │
基础层 ───────────────┘
```

- 工具层依赖核心层和网络层
- 网络层依赖核心层
- 核心层依赖基础层
- 基础层不依赖其他层

## 2. 核心模块划分

### 2.1 基础层

| 模块 | 职责 | 文件位置 |
|------|------|----------|
| 平台抽象 | 处理平台差异，提供统一接口 | include/base/platform.h |
| 类型定义 | 定义基础类型和常量 | include/base/types.h |
| 工具函数 | 提供通用工具函数 | include/base/utils.h |

### 2.2 核心层

| 模块 | 职责 | 文件位置 |
|------|------|----------|
| 智能指针 | 实现shared_ptr、unique_ptr、weak_ptr | include/memory/smart_ptr.h |
| 日志系统 | 实现分级日志、多输出目标、日志轮转 | include/core/logger.h |
| 文件IO | 实现JSON/XML处理和文件系统操作 | include/core/json.h, include/core/xml.h, include/core/filesystem.h |

### 2.3 网络层

| 模块 | 职责 | 文件位置 |
|------|------|----------|
| TCP模块 | 实现TCP连接管理、数据传输、SSL支持 | include/net/tcp.h |
| UDP模块 | 实现UDP数据报发送/接收、组播/广播 | include/net/udp.h |

### 2.4 工具层

| 模块 | 职责 | 文件位置 |
|------|------|----------|
| 配置管理 | 实现配置加载、解析和验证 | include/util/config.h |
| 错误处理 | 实现异常类层次结构和错误处理 | include/util/error.h |
| 线程池 | 实现线程池管理和任务调度 | include/util/thread_pool.h |
| 时间工具 | 实现时间戳、格式化和定时器 | include/util/time.h |

## 3. 模块间接口定义

### 3.1 智能指针模块

```cpp
namespace base {
namespace memory {

template <typename T>
class shared_ptr {
public:
    shared_ptr(T* ptr = nullptr);
    template <typename Deleter>
    shared_ptr(T* ptr, Deleter deleter);
    shared_ptr(const shared_ptr& other);
    shared_ptr(shared_ptr&& other) noexcept;
    ~shared_ptr();
    
    shared_ptr& operator=(const shared_ptr& other);
    shared_ptr& operator=(shared_ptr&& other) noexcept;
    
    T* get() const;
    T& operator*() const;
    T* operator->() const;
    explicit operator bool() const;
    
    size_t use_count() const;
    void reset(T* ptr = nullptr);
    void swap(shared_ptr& other);
};

// unique_ptr和weak_ptr接口类似

} // namespace memory
} // namespace base
```

### 3.2 日志系统模块

```cpp
namespace base {
namespace log {

enum class Level {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class Logger {
public:
    static void init(const std::string& logDir, const std::string& logName);
    static void setLevel(Level level);
    static void setFormat(const std::string& format);
    static void setRotation(size_t maxFileSize, int maxFiles);
    
    static void debug(const std::string& module, const std::string& message);
    static void info(const std::string& module, const std::string& message);
    static void warn(const std::string& module, const std::string& message);
    static void error(const std::string& module, const std::string& message);
    static void fatal(const std::string& module, const std::string& message);
};

} // namespace log
} // namespace base
```

### 3.3 文件IO模块

#### 3.3.1 JSON处理

```cpp
namespace base {
namespace io {

class Json {
public:
    static Json parse(const std::string& jsonStr);
    std::string toString() const;
    
    bool isObject() const;
    bool isArray() const;
    bool isString() const;
    bool isNumber() const;
    bool isBoolean() const;
    bool isNull() const;
    
    Json& operator[](const std::string& key);
    bool has(const std::string& key) const;
    
    Json& operator[](size_t index);
    size_t size() const;
    void push_back(const Json& value);
};

} // namespace io
} // namespace base
```

#### 3.3.2 XML处理

```cpp
namespace base {
namespace io {

class XmlDocument {
public:
    static XmlDocument load(const std::string& filePath);
    static XmlDocument parse(const std::string& xmlStr);
    
    bool save(const std::string& filePath) const;
    std::string toString() const;
    
    class Node {
    public:
        std::string getName() const;
        std::string getText() const;
        void setText(const std::string& text);
        
        std::string getAttribute(const std::string& name) const;
        void setAttribute(const std::string& name, const std::string& value);
        
        Node addChild(const std::string& name);
        std::vector<Node> getChildren(const std::string& name = "") const;
    };
    
    Node getRoot() const;
};

} // namespace io
} // namespace base
```

#### 3.3.3 文件系统操作

```cpp
namespace base {
namespace io {

class FileSystem {
public:
    static bool fileExists(const std::string& path);
    static bool createFile(const std::string& path);
    static bool deleteFile(const std::string& path);
    static bool renameFile(const std::string& oldPath, const std::string& newPath);
    
    static bool directoryExists(const std::string& path);
    static bool createDirectory(const std::string& path);
    static bool deleteDirectory(const std::string& path);
    static std::vector<std::string> listDirectory(const std::string& path);
    
    static bool setPermissions(const std::string& path, int permissions);
    static int getPermissions(const std::string& path);
    
    class File {
    public:
        File(const std::string& path, const std::string& mode);
        ~File();
        
        bool open();
        void close();
        bool isOpen() const;
        
        size_t read(void* buffer, size_t size);
        size_t write(const void* buffer, size_t size);
        size_t seek(size_t offset, int whence);
        size_t tell() const;
        size_t size() const;
    };
};

} // namespace io
} // namespace base
```

### 3.4 网络模块

#### 3.4.1 TCP模块

```cpp
namespace base {
namespace net {

class TcpClient {
public:
    TcpClient(const std::string& host, int port);
    ~TcpClient();
    
    bool connect();
    void disconnect();
    bool isConnected() const;
    
    int send(const void* data, size_t length);
    int recv(void* buffer, size_t length);
    
    void setConnectTimeout(int timeoutMs);
    void setReadTimeout(int timeoutMs);
    void setWriteTimeout(int timeoutMs);
    
    void enableSSL();
    bool isSSL() const;
};

class TcpServer {
public:
    TcpServer(int port);
    ~TcpServer();
    
    bool start();
    void stop();
    
    using ConnectionCallback = std::function<void(int clientId)>;
    using DataCallback = std::function<void(int clientId, const void* data, size_t length)>;
    using DisconnectionCallback = std::function<void(int clientId)>;
    
    void setConnectionCallback(ConnectionCallback callback);
    void setDataCallback(DataCallback callback);
    void setDisconnectionCallback(DisconnectionCallback callback);
};

} // namespace net
} // namespace base
```

#### 3.4.2 UDP模块

```cpp
namespace base {
namespace net {

class UdpSocket {
public:
    UdpSocket();
    ~UdpSocket();
    
    bool bind(int port);
    void unbind();
    
    int sendTo(const void* data, size_t length, const std::string& host, int port);
    int recvFrom(void* buffer, size_t length, std::string& host, int& port);
    
    bool joinMulticastGroup(const std::string& groupAddr);
    bool leaveMulticastGroup(const std::string& groupAddr);
    
    using DataCallback = std::function<void(const void* data, size_t length, const std::string& host, int port)>;
    void setDataCallback(DataCallback callback);
    void startAsync();
    void stopAsync();
};

} // namespace net
} // namespace base
```

### 3.5 工具模块

#### 3.5.1 配置管理

```cpp
namespace base {
namespace util {

class Config {
public:
    static Config load(const std::string& filePath);
    template <typename T>
    T get(const std::string& key, const T& defaultValue = T()) const;
    template <typename T>
    void set(const std::string& key, const T& value);
    bool save(const std::string& filePath) const;
};

} // namespace util
} // namespace base
```

#### 3.5.2 错误处理

```cpp
namespace base {
namespace util {

class Exception : public std::exception {
public:
    Exception(int code, const std::string& message);
    int code() const;
    const char* what() const noexcept override;
private:
    int m_code;
    std::string m_message;
};

} // namespace util
} // namespace base
```

#### 3.5.3 线程池

```cpp
namespace base {
namespace util {

class ThreadPool {
public:
    ThreadPool(size_t threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();
    
    template <typename Func, typename... Args>
    auto submit(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>;
    
    void shutdown();
    bool isRunning() const;
};

} // namespace util
} // namespace base
```

#### 3.5.4 时间工具

```cpp
namespace base {
namespace util {

class Time {
public:
    static int64_t timestamp();
    static std::string format(const std::string& format);
    
    class Timer {
    public:
        Timer();
        double elapsed() const;
        void reset();
    };
};

} // namespace util
} // namespace base
```

## 4. 技术栈选型

| 模块 | 技术选型 | 版本要求 | 备注 |
|------|---------|---------|------|
| 开发语言 | C++ | C++11及以上 | 兼容C++2010 |
| 构建工具 | CMake | 3.16+ | 跨平台构建 |
| 日志系统 | spdlog | 1.10.0+ | 高性能、header-only |
| 网络引擎 | Boost.Asio | 1.78.0+ | 跨平台网络库 |
| JSON处理 | nlohmann/json | 3.10.5+ | 现代C++ JSON库 |
| XML处理 | pugixml | 1.12.0+ | 轻量级XML解析库 |
| 测试框架 | Google Test | 1.11.0+ | 单元测试框架 |
| 加密库 | OpenSSL | 1.1.1+ | SSL/TLS支持 |

## 5. 数据流向设计

### 5.1 日志系统数据流

```
应用代码 → Logger接口 → 日志级别过滤 → 格式化 → 输出目标（控制台/文件）
```

### 5.2 网络模块数据流

#### 5.2.1 TCP数据流

```
发送：应用代码 → TcpClient::send() → 数据封装 → 网络发送
接收：网络接收 → 数据解析 → TcpServer回调 → 应用代码
```

#### 5.2.2 UDP数据流

```
发送：应用代码 → UdpSocket::sendTo() → 数据封装 → 网络发送
接收：网络接收 → 数据解析 → UdpSocket回调 → 应用代码
```

### 5.3 文件IO数据流

```
读取：文件 → FileSystem::File::read() → 应用代码
写入：应用代码 → FileSystem::File::write() → 文件
JSON/XML：文件 → 解析 → 内存对象 → 应用代码
```

### 5.4 配置管理数据流

```
配置文件 → Config::load() → 内存配置对象 → 应用代码
应用代码 → Config::set() → 内存配置对象 → Config::save() → 配置文件
```

## 6. 关键技术难点及解决方案

### 6.1 跨平台编译支持

**难点**：不同平台的API和特性差异
**解决方案**：
- 使用条件编译处理平台差异
- 抽象平台相关的实现细节
- 为不同平台提供专门的构建配置
- 定期在各平台上进行编译和测试

### 6.2 线程安全的智能指针

**难点**：引用计数的线程安全
**解决方案**：
- 使用原子操作实现线程安全的引用计数
- 提供weak_ptr解决循环引用问题
- 支持自定义删除器，增强灵活性

### 6.3 TCP粘包/分包处理

**难点**：TCP数据流的边界识别
**解决方案**：
- 采用长度前缀法解决粘包问题
- 实现缓冲区管理，处理分包情况
- 使用状态机解析数据

### 6.4 高效的日志系统

**难点**：线程安全的日志写入、高效的日志轮转
**解决方案**：
- 使用互斥锁保证线程安全
- 后台线程处理日志写入，减少对主线程的影响
- 实现日志缓冲区，批量写入
- 采用策略模式支持不同输出目标

### 6.5 大文件处理

**难点**：大文件的读写性能
**解决方案**：
- 使用内存映射文件处理大文件
- 实现分块读写，减少内存占用
- 优化文件IO操作，减少系统调用

### 6.6 SSL/TLS加密集成

**难点**：SSL/TLS的正确配置和使用
**解决方案**：
- 集成OpenSSL库实现加密
- 提供简单的SSL配置接口
- 处理SSL握手和证书验证

## 7. 架构设计决策

### 7.1 模块化设计

- **决策**：采用模块化设计，各模块之间通过清晰的接口进行交互
- **理由**：提高代码的可维护性和可扩展性，便于团队协作
- **影响**：需要定义清晰的模块边界和接口，确保模块间的解耦

### 7.2 跨平台兼容

- **决策**：优先确保在Linux、Windows和macOS三个主流操作系统上能够成功编译
- **理由**：扩大库的适用范围，提高代码的可移植性
- **影响**：需要处理平台差异，增加开发和测试的工作量

### 7.3 性能优先

- **决策**：在设计和实现中优先考虑性能
- **理由**：基础库的性能直接影响上层应用的性能
- **影响**：需要优化算法和数据结构，平衡性能和代码复杂度

### 7.4 接口稳定性

- **决策**：对外暴露的接口需保持稳定性和向后兼容性
- **理由**：确保库的使用者能够稳定使用，减少升级成本
- **影响**：需要谨慎设计接口，避免频繁变更

### 7.5 安全第一

- **决策**：在设计和实现中优先考虑安全性
- **理由**：基础库的安全性直接影响上层应用的安全性
- **影响**：需要防止内存泄漏、避免缓冲区溢出、处理异常情况

## 8. 实施计划

### 8.1 开发顺序

1. **基础层**：平台抽象、类型定义、工具函数
2. **核心层**：智能指针、日志系统、文件IO
3. **网络层**：TCP模块、UDP模块
4. **工具层**：配置管理、错误处理、线程池、时间工具

### 8.2 测试策略

- **功能驱动测试**：一个功能一个功能地测试，确保当前功能测试通过后，再进行下一个功能的开发
- **测试先行**：每个功能模块在开发前制定详细的测试计划和测试用例
- **持续集成**：每个功能模块完成后，立即集成到主分支并进行集成测试
- **跨平台验证**：每个功能模块在开发过程中，定期在各目标平台上进行编译和测试

### 8.3 交付物

- **库文件**：静态链接库（.a/.lib）和动态链接库（.so/.dll/.dylib）
- **文档**：API文档、使用示例、安装脚本、部署指南
- **测试**：测试用例、测试报告

## 9. 结论

本架构设计文档详细规划了C++基础库的系统架构、核心模块、接口定义、技术栈选型、数据流向设计和关键技术难点及解决方案。通过模块化设计和合理的技术选型，确保了库的可扩展性、可维护性和性能。

架构设计满足项目需求，具备可实施性。在实施过程中，将严格按照架构设计进行开发，确保代码质量和功能完整性。