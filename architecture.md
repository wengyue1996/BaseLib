# BaseLib 程序架构文档

## 1. 系统总体架构

### 1.1 架构层次

BaseLib 是一个零依赖的 C++11 跨平台基础库，采用分层架构设计：

```
┌────────────────────────────────────────────────────────────┐
│                    应用层 (Application)                    │
├────────────────────────────────────────────────────────────┤
│                    工具层 (util)                           │
│  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐  │
│  │ Config    │ │ Exception │ │ ThreadPool│ │ Time      │  │
│  │ 配置管理  │ │ 错误处理  │ │ 线程池    │ │ 时间工具  │  │
│  └───────────┘ └───────────┘ └───────────┘ └───────────┘  │
│  ┌───────────┐ ┌───────────┐ ┌───────────┐               │
│  │ Lock      │ │ Result<T> │ │ Thread    │               │
│  │ 线程同步  │ │ 结果类型  │ │ 线程封装  │               │
│  └───────────┘ └───────────┘ └───────────┘               │
├────────────────────────────────────────────────────────────┤
│                    网络层 (net)                            │
│  ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌───────────┐  │
│  │TCP    │ │UDP    │ │HTTP   │ │HTTP   │ │HTTP       │  │
│  │Client │ │Socket │ │Client │ │Server │ │(Simple)   │  │
│  │/Server│ │       │ │       │ │       │ │           │  │
│  └───────┘ └───────┘ └───────┘ └───────┘ └───────────┘  │
├────────────────────────────────────────────────────────────┤
│                    核心层 (core + io + memory)             │
│  ┌───────────┐ ┌───────────┐ ┌───────────┐               │
│  │ Logger    │ │ FileSystem│ │ Smart Ptr │               │
│  │ 日志系统  │ │ 文件IO    │ │ 智能指针  │               │
│  └───────────┘ └───────────┘ └───────────┘               │
│  ┌───────────┐ ┌───────────┐                             │
│  │ JSON      │ │ XML       │                             │
│  │ 解析/序列 │ │ 解析/序列 │                             │
│  └───────────┘ └───────────┘                             │
└────────────────────────────────────────────────────────────┘
```

### 1.2 模块依赖关系

```
工具层 (util) ───── 依赖 ────→ 核心层 (core/io)
    │                              ↑
    │                              │
网络层 (net) ───── 依赖 ──────────┘
    ↑
    │
核心层 (core/io/memory) ─ 无外部依赖（仅标准库）
```

- 工具层依赖核心层（Result/ErrorCode、Logger）
- 网络层依赖核心层（Logger、ErrorCode）和工具层（Result）
- 核心层仅依赖 C++11 标准库和平台 API

## 2. 核心模块划分

### 2.1 核心层 (core / io / memory)

| 模块 | 职责 | 文件位置 |
|------|------|----------|
| 日志系统 | 分级日志、控制台/文件输出、日志轮转、多线程安全 | include/core/logger.h, src/core/logger.cpp |
| 智能指针 | shared_ptr、unique_ptr、weak_ptr、自定义删除器 | include/memory/smart_ptr.h |
| JSON处理 | 解析、序列化、类型查询、转义处理 | include/io/json.h, src/io/json.cpp |
| XML处理 | DOM解析、节点操作、属性管理 | include/io/xml.h, src/io/xml.cpp |
| 文件系统 | 文件/目录CRUD、递归删除、Unicode路径、错误信息 | include/io/filesystem.h, src/io/filesystem.cpp |

### 2.2 网络层 (net)

| 模块 | 职责 | 文件位置 |
|------|------|----------|
| TCP模块 | TCP客户端/服务端、连接管理、数据传输 | include/net/tcp.h, src/net/tcp.cpp |
| UDP模块 | UDP数据报、组播/广播 | include/net/udp.h, src/net/udp.cpp |
| HTTP客户端 | GET/POST/PUT/DELETE、重定向跟随、自动重试、代理支持 | include/net/http_client.h, src/net/http_client.cpp |
| HTTP服务端 | 路由注册、中间件、CORS、Keep-Alive、多线程处理 | include/net/http_server.h, src/net/http_server.cpp |
| SimpleHTTP | 简化的HTTP GET/POST请求实现 | include/net/http.h, src/net/http.cpp |

### 2.3 工具层 (util)

| 模块 | 职责 | 文件位置 |
|------|------|----------|
| 配置管理 | 键值存储、JSON序列化、类型安全get/set | include/util/config.h, src/util/config.cpp |
| 错误处理 | 异常类层次结构、错误类别分类 | include/util/error.h, src/util/error.cpp |
| Result类型 | 泛型Result<T>、Result<void>、统一ErrorCode | include/util/result.h |
| 线程同步 | RecursiveMutex、NonRecursiveMutex、ReadWriteLock、LockGuard | include/util/lock.h, src/util/lock.cpp |
| 线程封装 | Thread类、线程生命周期管理 | include/util/thread.h, src/util/thread.cpp |
| 线程池 | 任务调度、pause/resume、waitAll | include/util/thread_pool.h, src/util/thread_pool.cpp |
| 时间工具 | 时间戳、DateTime、格式化、Timer | include/util/time.h, src/util/time.cpp |

## 3. 模块间接口定义

### 3.1 智能指针模块

```cpp
namespace base {
namespace memory {

template <typename T>
class shared_ptr {
public:
    shared_ptr();
    shared_ptr(T* ptr);
    template <typename Deleter>
    shared_ptr(T* ptr, Deleter deleter);  // 自定义删除器
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
    bool unique() const;
    void reset(T* ptr = nullptr);
    void swap(shared_ptr& other);
};

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args);

template <typename T>
class weak_ptr {
public:
    weak_ptr();
    weak_ptr(const shared_ptr<T>& sp);
    weak_ptr(const weak_ptr& other);
    ~weak_ptr();

    bool expired() const;
    size_t use_count() const;
    shared_ptr<T> lock() const;
};

template <typename T>
class unique_ptr {
public:
    unique_ptr(T* ptr = nullptr);
    unique_ptr(unique_ptr&& other) noexcept;
    ~unique_ptr();

    T* get() const;
    T& operator*() const;
    T* operator->() const;
    explicit operator bool() const;

    unique_ptr& operator=(unique_ptr&& other) noexcept;
    void reset(T* ptr = nullptr);
    T* release();
    void swap(unique_ptr& other);
};

} // namespace memory
} // namespace base
```

### 3.2 日志系统模块

```cpp
namespace base {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR_LEVEL = 3,
    FATAL = 4
};

class Logger {
public:
    static Logger& getInstance();

    void init(const std::string& logDir, const std::string& logFile = "app.log");
    void setLogLevel(LogLevel level);
    void setMaxFileSize(size_t maxSize);
    void setMaxFileCount(int maxCount);
    void setConsoleOutput(bool enable);

    void debug(const std::string& module, const std::string& message);
    void info(const std::string& module, const std::string& message);
    void warning(const std::string& module, const std::string& message);
    void error(const std::string& module, const std::string& message);
    void fatal(const std::string& module, const std::string& message);

    void shutdown();
};

// 便捷宏
#define BASE_LOG_DEBUG(module, msg) ...
#define BASE_LOG_INFO(module, msg) ...
#define BASE_LOG_WARNING(module, msg) ...
#define BASE_LOG_ERROR(module, msg) ...
#define BASE_LOG_FATAL(module, msg) ...

} // namespace base
```

### 3.3 IO模块

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
    bool isBool() const;
    bool isNull() const;

    std::string asString() const;
    double asNumber() const;
    bool asBool() const;

    Json& operator[](const std::string& key);
    Json& operator[](size_t index);
    bool has(const std::string& key) const;
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
    class Node {
    public:
        Node();
        Node(const std::string& name);
        Node(const std::string& name, const std::string& text);

        std::string getName() const;
        std::string getText() const;
        void setText(const std::string& text);

        std::string getAttribute(const std::string& name) const;
        void setAttribute(const std::string& name, const std::string& value);

        Node& addChild(const std::string& name);
        Node& addChild(const std::string& name, const std::string& text);
        std::vector<Node> getChildren(const std::string& name = "") const;
        const std::vector<Node>& getAllChildren() const;
    };

    static XmlDocument parse(const std::string& xmlStr);
    static XmlDocument load(const std::string& filePath);

    XmlDocument();
    XmlDocument(const std::string& rootName);

    std::string toString() const;
    bool save(const std::string& filePath) const;

    Node getRoot() const;
    void setRoot(const Node& root);
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
    static bool copyFile(const std::string& srcPath, const std::string& destPath);

    static bool directoryExists(const std::string& path);
    static bool createDirectory(const std::string& path);
    static bool createDirectories(const std::string& path);
    static bool deleteDirectory(const std::string& path);
    static bool deleteDirectoryRecursive(const std::string& path);
    static std::vector<std::string> listDirectory(const std::string& path, bool recursive = false);

    static bool setPermissions(const std::string& path, int permissions);
    static int getPermissions(const std::string& path);
    static std::string getLastError();

    static std::string getAbsolutePath(const std::string& path);
    static std::string getFileName(const std::string& path);
    static std::string getFileExtension(const std::string& path);
    static std::string getDirectoryName(const std::string& path);
    static std::string getCurrentDirectory();

    static bool isAbsolutePath(const std::string& path);
    static bool isRelativePath(const std::string& path);
    static std::string joinPath(const std::string& path1, const std::string& path2);

    class File {
    public:
        File(const std::string& path, const std::string& mode);
        ~File();

        bool open();
        void close();
        bool isOpen() const;
        bool isEndOfFile() const;

        size_t read(void* buffer, size_t size);
        size_t write(const void* buffer, size_t size);
        size_t seek(long offset, int whence);
        long tell() const;
        size_t size() const;
        void flush();

        bool readLine(std::string& line);
        bool writeLine(const std::string& line);
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
    TcpClient();
    ~TcpClient();

    bool connect(const std::string& host, int port);
    void disconnect();
    bool isConnected() const;

    int send(const char* data, int length);
    int recv(char* buffer, int length);

    void setConnectTimeout(int timeoutMs);
};

class TcpServer {
public:
    TcpServer();
    ~TcpServer();

    bool start(int port);
    void stop();
    bool isRunning() const;

    using DataCallback = std::function<void(int clientId, const char* data, int length)>;
    void setDataCallback(DataCallback callback);
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
    void close();

    int sendTo(const char* data, int length, const std::string& host, int port);
    int recvFrom(char* buffer, int length, std::string& host, int& port);
};

} // namespace net
} // namespace base
```

#### 3.4.3 HTTP客户端

```cpp
namespace base {
namespace net {

enum class HttpMethod { GET, POST, PUT, DELETE, PATCH, OPTIONS, HEAD };

struct RequestConfig {
    int timeout_ms = 30000;
    bool follow_redirects = true;
    int max_redirects = 3;
    std::string user_agent = "BaseLib-HttpClient/1.0";
    std::string proxy_host;
    int proxy_port = 0;
    bool verify_ssl = true;
    int max_retries = 0;
    int retry_delay_ms = 1000;
};

class HttpClient : public IHttpClient {
public:
    Result<HttpResponse> get(const std::string& url, RequestConfig config = RequestConfig());
    Result<HttpResponse> post(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig());
    Result<HttpResponse> put(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig());
    Result<HttpResponse> del(const std::string& url, RequestConfig config = RequestConfig());
    Result<HttpResponse> patch(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig());

    void addInterceptor(std::shared_ptr<IInterceptor> interceptor);
    void removeInterceptor(IInterceptor* interceptor);

    static std::string urlEncode(const std::string& value);
    static std::string urlDecode(const std::string& value);
};

class MockHttpClient : public IHttpClient {
    // 用于单元测试的Mock客户端，可预设响应或错误
    void setMockResponse(const HttpResponse& response);
    void setMockError(const ErrorCode& error);
    int getRequestCount() const;
    HttpRequest getLastRequest() const;
};

} // namespace net
} // namespace base
```

#### 3.4.4 HTTP服务端

```cpp
namespace base {
namespace net {

enum class HttpStatus {
    OK = 200, CREATED = 201, NO_CONTENT = 204,
    BAD_REQUEST = 400, UNAUTHORIZED = 401, FORBIDDEN = 403,
    NOT_FOUND = 404, METHOD_NOT_ALLOWED = 405,
    INTERNAL_SERVER_ERROR = 500, NOT_IMPLEMENTED = 501,
    SERVICE_UNAVAILABLE = 503
};

struct ServerConfig {
    int port = 8080;
    std::string host = "0.0.0.0";
    int num_threads = 4;
    int connection_timeout_ms = 30000;
    int max_request_size = 1048576;
    int max_keepalive_requests = 100;
    bool enable_cors = false;
    std::string cors_allow_origin = "*";
};

using HttpHandler = std::function<void(const HttpRequest&, HttpResponse&)>;
using MiddlewareHandler = std::function<bool(const HttpRequest&, HttpResponse&)>;

class HttpServer {
public:
    HttpServer(const ServerConfig& config = ServerConfig());

    Result<void> start();
    Result<void> stop(int timeout_ms = 5000);
    bool isRunning() const;

    void get(const std::string& path, HttpHandler handler);
    void post(const std::string& path, HttpHandler handler);
    void put(const std::string& path, HttpHandler handler);
    void del(const std::string& path, HttpHandler handler);
    void patch(const std::string& path, HttpHandler handler);
    void options(const std::string& path, HttpHandler handler);
    void head(const std::string& path, HttpHandler handler);
    void any(const std::string& path, HttpHandler handler);

    void use(MiddlewareHandler middleware);
    void use(const std::string& path, MiddlewareHandler middleware);
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

    bool has(const std::string& key) const;
    void remove(const std::string& key);
    bool save(const std::string& filePath) const;
    std::string toJson() const;
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
    Exception(int code, const std::string& message, const std::string& details);

    int code() const;
    const std::string& message() const;
    const std::string& details() const;
    const char* what() const noexcept override;
    void setDetails(const std::string& details);
};

class ErrorCategory {
public:
    enum Category {
        SYSTEM = 1000, NETWORK = 2000, FILE = 3000,
        MEMORY = 4000, LOGIC = 5000, RUNTIME = 6000, UNKNOWN = 9999
    };
    static const std::string& getCategoryName(Category category);
    static Category getCategory(int errorCode);
};

} // namespace util
} // namespace base
```

#### 3.5.3 Result 和 ErrorCode

```cpp
namespace base {

class ErrorCode {
public:
    enum Code {
        SUCCESS = 0,
        // 通用 (100X)
        INVALID_ARGUMENT = 1001, NULL_POINTER = 1002, OUT_OF_RANGE = 1003,
        INVALID_STATE = 1004, NOT_IMPLEMENTED = 1005, INVALID_CONFIG = 1006,
        // 网络 (200X)
        NETWORK_ERROR = 2001, CONNECTION_FAILED = 2002, CONNECTION_TIMEOUT = 2003,
        SEND_FAILED = 2004, RECV_FAILED = 2005, SOCKET_ERROR = 2006,
        BIND_FAILED = 2007, LISTEN_FAILED = 2008, ACCEPT_FAILED = 2009,
        NETWORK_CONNECTION_REFUSED = 2010, NETWORK_HOST_UNREACHABLE = 2011,
        // 文件 (300X)
        FILE_NOT_FOUND = 3001, FILE_OPEN_FAILED = 3002, FILE_READ_FAILED = 3003,
        FILE_WRITE_FAILED = 3004, DIRECTORY_NOT_FOUND = 3005,
        PERMISSION_DENIED = 3006, FILE_ALREADY_EXISTS = 3007, FILE_IO_ERROR = 3008,
        // 内存 (4001)
        MEMORY_ALLOC_FAILED = 4001,
        // JSON (410X)
        JSON_PARSE_ERROR = 4101, JSON_INVALID_TYPE = 4102, JSON_KEY_NOT_FOUND = 4103,
        // XML (500X)
        XML_PARSE_ERROR = 5001, XML_INVALID_FORMAT = 5002,
        // 线程 (600X)
        THREAD_ERROR = 6001, THREAD_CREATE_FAILED = 6002,
        THREAD_JOIN_FAILED = 6003, DEADLOCK = 6004,
        // 未知
        UNKNOWN_ERROR = 9999
    };

    ErrorCode();
    ErrorCode(Code code);
    ErrorCode(Code code, const std::string& message);

    Code code() const;
    const std::string& message() const;
    bool isSuccess() const;
    bool isError() const;
    std::string toString() const;
    static std::string getDefaultMessage(Code code);
    static std::string getErrorMessage(Code code);
};

template<typename T>
class Result {
public:
    static Result success(const T& value);
    static Result failure(ErrorCode::Code code);
    static Result failure(ErrorCode::Code code, const std::string& message);
    static Result failure(const ErrorCode& error);

    bool isSuccess() const;
    bool isError() const;
    bool hasValue() const;
    const T& value() const;
    T& value();
    const ErrorCode& error() const;
    ErrorCode::Code errorCode() const;
    const std::string& errorMessage() const;
    const T* ptr() const;
};

template<>
class Result<void> {
public:
    static Result success();
    static Result failure(ErrorCode::Code code);
    static Result failure(ErrorCode::Code code, const std::string& message);
    static Result failure(const ErrorCode& error);

    bool isSuccess() const;
    bool isError() const;
    const ErrorCode& error() const;
    ErrorCode::Code errorCode() const;
    const std::string& errorMessage() const;
};

typedef Result<void> Status;

} // namespace base
```

#### 3.5.4 线程同步

```cpp
namespace base {
namespace util {

class RecursiveMutex {
public:
    RecursiveMutex();
    ~RecursiveMutex();
    void lock();
    void unlock();
    bool tryLock(int timeout_ms = 0);
};

class NonRecursiveMutex {
public:
    NonRecursiveMutex();
    ~NonRecursiveMutex();
    void lock();
    void unlock();
    bool tryLock(int timeout_ms = 0);
};

class ReadWriteLock {
public:
    ReadWriteLock();
    ~ReadWriteLock();
    void readLock();
    void readUnlock();
    bool tryReadLock(int timeout_ms = 0);
    void writeLock();
    void writeUnlock();
    bool tryWriteLock(int timeout_ms = 0);
};

template <typename T>
class LockGuard {
public:
    explicit LockGuard(T& lock);
    ~LockGuard();
};

} // namespace util
} // namespace base
```

#### 3.5.5 线程池

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
    void pause();
    void resume();
    bool isPaused() const;
    void waitAll();

    size_t getThreadCount() const;
    size_t getTaskCount() const;
};

} // namespace util
} // namespace base
```

#### 3.5.6 时间工具

```cpp
namespace base {
namespace util {

struct DateTime {
    int year, month, day, hour, minute, second, millisecond;
    DateTime();
    static DateTime now();
    std::string format(const std::string& formatStr) const;
};

class Time {
public:
    static int64_t timestamp();
    static int64_t timestampMillis();
    static std::string format(const std::string& formatStr);
    static std::string getCurrentDate();
    static std::string getCurrentTime();

    class Timer {
    public:
        Timer();
        void reset();
        double elapsedSeconds() const;
        int64_t elapsedMilliseconds() const;
    };
};

} // namespace util
} // namespace base
```

## 4. 技术栈

| 类别 | 技术选型 | 说明 |
|------|---------|------|
| 开发语言 | C++11 | 仅依赖标准库，零第三方依赖 |
| 构建工具 | CMake 3.16+ | 跨平台构建 |
| 平台支持 | Windows, Linux, macOS | 条件编译处理平台差异 |
| 网络 | BSD Socket API | 原生 socket，无第三方网络库 |
| JSON/XML | 自实现解析器 | 纯 C++11，无外部依赖 |
| 日志 | 自实现 | 线程安全，文件轮转，无外部依赖 |
| 智能指针 | 自实现 | atomic 引用计数，自定义删除器 |
| 线程同步 | std::mutex + condition_variable | ReadWriteLock 写优先策略 |
| 测试 | assert + manual | 无测试框架依赖 |

## 5. 数据流向

### 5.1 HTTP 服务端

```
客户端请求 → HttpServer::acceptLoop() → parseHttpRequest() → middleware链
           → processRequest() → 路由匹配 → handler回调 → buildHttpResponse() → 客户端
```

### 5.2 HTTP 客户端

```
应用代码 → HttpClient::get/post() → interceptor链(onRequest)
         → doRequest() → DNS解析(getaddrinfo) → socket连接
         → [代理/重定向/重试循环] → HTTP发送 → 接收响应
         → interceptor链(onResponse) → Result<HttpResponse>
```

### 5.3 日志系统

```
应用代码 → BASE_LOG_* 宏 → Logger::debug/info/etc() → 日志级别过滤
         → 格式化(线程安全localtime) → 控制台输出 + 文件写入(带轮转)
```

### 5.4 线程池

```
应用代码 → ThreadPool::submit() → 任务入队 → worker线程唤醒
         → 任务执行(pause检查) → 结果写入future → done_condition通知
```

## 6. 关键技术实现

### 6.1 ReadWriteLock（写优先策略）

使用 `std::mutex` + `std::condition_variable` 实现：
- 读锁：等待写者活跃或写者等待时阻塞
- 写锁：等待读者或写者活跃时阻塞
- 写优先：writeUnlock 优先唤醒等待的写者，避免写饥饿

### 6.2 线程安全保证

- `localtime` → `localtime_s`/`localtime_r`（线程安全版本）
- `gethostbyname` → `getaddrinfo`（可重入）
- `inet_ntoa` → `inet_ntop`（缓冲区由调用者提供）
- `Exception::what()` → 构造时预格式化 m_what 字符串
- shared_ptr 引用计数 → `std::atomic` + `memory_order_acq_rel`

### 6.3 HTTP 客户端重定向/重试/代理

在 `doRequest()` 中实现循环控制：
- 3xx 响应 + `follow_redirects` → 提取 Location → 重新解析 URL → 继续循环
- 5xx 或网络错误 + `max_retries > 0` → `retry_delay_ms` 等待 → 继续循环
- proxy_host 配置 → socket 连接到代理 → 发送完整 URL

### 6.4 HTTP 服务端 Keep-Alive

`handleClient()` 中循环读取请求：
- 解析 `Connection: keep-alive` 头
- 复用同一 socket 直到达到 `max_keepalive_requests`
- 空闲超时保护（`connection_timeout_ms`）

### 6.5 零第三方依赖

所有模块（JSON解析、XML解析、HTTP解析、日志、智能指针、线程池）均为自实现，仅依赖：
- C++11 标准库
- 平台原生 API（Win32 socket / POSIX socket / pthread）

## 7. 架构设计原则

- **零依赖**：纯 C++11 标准库 + 平台 API，无第三方库依赖
- **跨平台**：Win32/POSIX 条件编译，统一的平台抽象层
- **Result 模式**：使用 `Result<T>` 而非异常进行错误传播
- **RAII**：LockGuard、File、Thread 等资源管理遵循 RAII 原则
- **Pimpl**：核心类使用 Pimpl 惯用法保证 ABI 稳定性
- **线程安全**：所有共享数据结构使用原子操作或互斥锁保护
