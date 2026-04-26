# C++基础库 - Logger接口文档

## 1. 基本信息

- **模块名称**：Logger
- **功能描述**：提供分级日志记录功能，支持多输出目标、日志轮转、自定义格式等特性
- **版本号**：1.0.0
- **适用平台**：Windows、Linux、macOS
- **依赖项**：C++标准库（无第三方依赖）

## 2. 命名空间

所有接口位于 `base::log` 命名空间下。

## 3. 核心接口

### 3.1 初始化与配置

#### 3.1.1 init

**功能**：初始化日志系统，创建日志目录和文件

**接口**：
```cpp
static void init(const std::string& logDir);
```

**参数**：
| 参数名 | 类型 | 是否必填 | 说明 |
|--------|------|----------|------|
| logDir | std::string | 是 | 日志文件存储目录路径 |

**返回值**：无

**说明**：如果目录不存在，会自动创建

#### 3.1.2 setLevel

**功能**：设置日志级别，低于该级别的日志将不会输出

**接口**：
```cpp
static void setLevel(Level level);
```

**参数**：
| 参数名 | 类型 | 是否必填 | 说明 |
|--------|------|----------|------|
| level | Level | 是 | 日志级别（DEBUG、INFO、WARN、ERROR、FATAL） |

**返回值**：无

#### 3.1.3 setFormat

**功能**：设置日志输出格式

**接口**：
```cpp
static void setFormat(const std::string& format);
```

**参数**：
| 参数名 | 类型 | 是否必填 | 说明 |
|--------|------|----------|------|
| format | std::string | 是 | 日志格式字符串，支持占位符 |

**返回值**：无

**格式占位符**：
| 占位符 | 说明 | 示例 |
|--------|------|------|
| %TIME% | 当前时间（精确到毫秒） | 2026-04-26 12:34:56.789 |
| %LEVEL% | 日志级别 | INFO |
| %MODULE% | 模块名 | Demo |
| %FILE% | 源文件名（包含扩展名） | log_demo.cpp |
| %LINE% | 代码行号 | 42 |
| %FUNCTION% | 函数名 | demoCustomFormat |
| %MSG% | 日志消息 | This is a message |

**默认格式**：
```
[%TIME%] [%LEVEL%] [%MODULE%] [%FILE%:%LINE%] [%FUNCTION%] %MSG%
```

#### 3.1.4 setRotation

**功能**：设置日志文件大小限制，超过后会自动轮转

**接口**：
```cpp
static void setRotation(size_t maxFileSize);
```

**参数**：
| 参数名 | 类型 | 是否必填 | 说明 |
|--------|------|----------|------|
| maxFileSize | size_t | 是 | 单个日志文件的最大大小（字节） |

**返回值**：无

**默认值**：200 * 1024 * 1024（200MB）

#### 3.1.5 shutdown

**功能**：关闭日志系统，刷新并关闭日志文件

**接口**：
```cpp
static void shutdown();
```

**参数**：无

**返回值**：无

### 3.2 日志输出

#### 3.2.1 直接调用接口

**功能**：输出不同级别的日志

**接口**：
```cpp
static void debug(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
static void info(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
static void warn(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
static void error(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
static void fatal(const std::string& module, const std::string& message, const std::string& file, int line, const std::string& function);
```

**参数**：
| 参数名 | 类型 | 是否必填 | 说明 |
|--------|------|----------|------|
| module | std::string | 是 | 模块名称 |
| message | std::string | 是 | 日志消息内容 |
| file | std::string | 是 | 源文件路径 |
| line | int | 是 | 代码行号 |
| function | std::string | 是 | 函数名称 |

**返回值**：无

#### 3.2.2 宏定义接口（推荐）

**功能**：使用宏自动获取文件、行号和函数名

**宏定义**：
```cpp
#define LOG_DEBUG(module, message) Logger::debug(module, message, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(module, message) Logger::info(module, message, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARN(module, message) Logger::warn(module, message, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(module, message) Logger::error(module, message, __FILE__, __LINE__, __FUNCTION__)
#define LOG_FATAL(module, message) Logger::fatal(module, message, __FILE__, __LINE__, __FUNCTION__)
```

**参数**：
| 参数名 | 类型 | 是否必填 | 说明 |
|--------|------|----------|------|
| module | std::string | 是 | 模块名称 |
| message | std::string | 是 | 日志消息内容 |

**返回值**：无

## 4. 数据类型

### 4.1 日志级别（Level）

```cpp
enum class Level {
    DEBUG,  // 调试信息
    INFO,   // 一般信息
    WARN,   // 警告信息
    ERROR,  // 错误信息
    FATAL   // 致命错误
};
```

## 5. 错误处理

### 5.1 错误情况

| 错误情况 | 处理方式 | 表现 |
|----------|----------|------|
| 日志文件创建失败 | 输出错误信息到标准错误，继续运行 | 控制台输出 "Failed to open log file!" |
| 日志文件写入失败 | 输出错误信息到标准错误，继续运行 | 控制台输出 "Failed to write to log file!" |
| 日志文件刷新失败 | 输出错误信息到标准错误，继续运行 | 控制台输出 "Failed to flush log file!" |
| 日志轮转失败 | 输出错误信息到标准错误，继续尝试使用原文件 | 控制台输出 "Failed to open new log file during rotation!" |

### 5.2 错误码

| 错误码 | 含义 |
|--------|------|
| 0 | 成功 |
| 1 | 文件创建失败 |
| 2 | 文件写入失败 |
| 3 | 文件刷新失败 |
| 4 | 目录创建失败 |

## 6. 日志文件命名规范

日志文件采用以下命名格式：

```
可执行文件名_进程号_日志文件生成时间.log
```

- **可执行文件名**：当前运行的可执行文件名称（不含扩展名）
- **进程号**：当前进程的ID
- **日志文件生成时间**：文件创建时的系统时间，格式为 `YYYYMMDDHHMMSS`（年月日时分秒）

**示例**：
```
log_demo_12345_20260426123456.log
```

## 7. 调用示例

### 7.1 基本使用

```cpp
#include "core/logger.h"

using namespace base::log;

int main() {
    // 初始化日志系统
    Logger::init("logs");
    
    // 输出不同级别的日志
    LOG_DEBUG("Main", "Debug message");
    LOG_INFO("Main", "Info message");
    LOG_WARN("Main", "Warning message");
    LOG_ERROR("Main", "Error message");
    LOG_FATAL("Main", "Fatal message");
    
    // 关闭日志系统
    Logger::shutdown();
    
    return 0;
}
```

### 7.2 高级配置

```cpp
#include "core/logger.h"

using namespace base::log;

void processData() {
    // 设置日志级别为WARN，低于WARN的日志不会输出
    Logger::setLevel(Level::WARN);
    
    // 输出不同级别的日志
    LOG_DEBUG("Process", "This won't be printed");
    LOG_INFO("Process", "This won't be printed either");
    LOG_WARN("Process", "This will be printed");
    LOG_ERROR("Process", "This will be printed too");
    
    // 自定义日志格式
    Logger::setFormat("[%TIME%] %LEVEL% - %MODULE% - %FILE%:%LINE% - %MSG%");
    LOG_INFO("Process", "Message with custom format");
    
    // 设置日志文件大小限制为1MB
    Logger::setRotation(1024 * 1024);
    
    // 输出大量日志，测试轮转
    for (int i = 0; i < 1000; i++) {
        LOG_INFO("Process", "Log message " + std::to_string(i));
    }
}

int main() {
    // 初始化日志系统
    Logger::init("logs");
    
    // 处理数据
    processData();
    
    // 关闭日志系统
    Logger::shutdown();
    
    return 0;
}
```

## 8. 使用限制

1. **线程安全**：日志系统是线程安全的，可以在多线程环境中使用
2. **性能考虑**：
   - 日志写入会进行文件IO操作，可能影响性能
   - 建议在性能敏感的代码路径中使用适当的日志级别
3. **文件系统**：
   - 需要对日志目录有写入权限
   - 确保磁盘空间充足，避免日志文件占用过多空间
4. **跨平台**：
   - 在Windows平台上，路径分隔符为 `\`
   - 在Linux/macOS平台上，路径分隔符为 `/`
5. **日志级别**：
   - DEBUG：用于开发调试
   - INFO：用于一般信息
   - WARN：用于警告信息
   - ERROR：用于错误信息
   - FATAL：用于致命错误，通常在程序崩溃前使用

## 9. 最佳实践

1. **模块命名**：使用有意义的模块名称，便于区分不同组件的日志
2. **日志内容**：日志消息应清晰、简洁，包含必要的上下文信息
3. **日志级别**：根据消息的重要性选择合适的日志级别
4. **错误处理**：在捕获异常时，使用ERROR级别记录错误信息
5. **性能优化**：
   - 在循环中避免频繁输出日志
   - 对于调试信息，使用DEBUG级别
   - 对于关键操作，使用INFO级别
6. **日志轮转**：根据实际需求设置合理的日志文件大小限制
7. **目录结构**：为不同的应用或模块创建独立的日志目录

## 10. 版本历史

| 版本 | 日期 | 变更内容 |
|------|------|----------|
| 1.0.0 | 2026-04-26 | 初始版本：实现基本日志功能、分级日志、多输出目标、日志轮转、自定义格式、线程安全、跨平台兼容 |

## 11. 常见问题

### 11.1 日志文件没有生成

**可能原因**：
- 日志目录不存在且无法创建
- 没有写入权限
- 磁盘空间不足

**解决方案**：
- 确保日志目录存在且有写入权限
- 检查磁盘空间
- 查看控制台输出的错误信息

### 11.2 日志级别设置不生效

**可能原因**：
- 日志级别设置在初始化之前
- 代码中多次设置日志级别

**解决方案**：
- 在初始化之后设置日志级别
- 检查代码中是否有多次设置日志级别的情况

### 11.3 日志轮转不工作

**可能原因**：
- 日志文件大小未达到设置的限制
- 磁盘空间不足
- 日志目录权限问题

**解决方案**：
- 检查日志文件大小限制设置
- 确保磁盘空间充足
- 检查日志目录权限

### 11.4 跨平台兼容性问题

**可能原因**：
- 路径分隔符使用错误
- 平台特定的API调用

**解决方案**：
- 使用相对路径或平台无关的路径处理
- 确保代码中使用了条件编译处理平台差异

## 12. 联系方式

如有问题或建议，请联系：
- 项目维护者：C++基础库团队
- 邮箱：support@baselib.com
- 网站：https://baselib.com