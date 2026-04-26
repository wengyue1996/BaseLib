# 开发工程师技能文档

## 角色职责

### 核心职责
- 参与需求分析，理解功能需求
- 设计和实现功能模块
- 编写代码和单元测试
- 编写技术文档
- 修复缺陷和优化性能

### 技术能力要求
- 熟练掌握C++语言和标准库
- 理解设计模式和软件架构
- 熟悉跨平台开发技术
- 掌握代码调试和性能分析技能

## 开发流程

### 1. 需求理解
1. 参加需求评审会议
2. 理解功能需求和技术要求
3. 确认有疑问的地方
4. 提出技术实现建议

### 2. 架构设计
1. 设计模块接口
2. 定义数据结构
3. 设计算法实现
4. 考虑扩展性和性能

### 3. 编码实现
1. 遵循编码规范
2. 编写清晰的代码注释
3. 注意代码安全
4. 确保跨平台兼容

### 4. 单元测试
1. 编写测试用例
2. 验证功能正确性
3. 测试边界条件
4. 测试错误处理

### 5. 文档编写
1. 编写接口文档
2. 编写使用示例
3. 记录已知问题

## 已完成的功能模块

### 核心库 (include/, src/)

#### 1. 智能指针 (memory/smart_ptr.h)
- **功能**：shared_ptr, unique_ptr, weak_ptr, make_shared, make_unique
- **标准**：C++11，完全重新实现
- **实现要点**：ControlBlock虚函数表设计、引用计数原子操作、RAII原则
- **特性**：线程安全引用计数、移动语义支持、weak_ptr expired检测
- **测试**：test_smart_ptr.cpp (11个测试用例)

#### 2. JSON处理 (io/json.h)
- **功能**：JSON解析、序列化、嵌套结构处理
- **实现要点**：递归下降解析器、转义字符处理
- **测试**：test_io.cpp

#### 3. XML处理 (io/xml.h)
- **功能**：XML解析、生成、节点和属性操作
- **实现要点**：DOM风格解析、实体编码/解码
- **测试**：test_io.cpp

#### 4. 文件系统 (io/filesystem.h)
- **功能**：文件读写、目录操作、路径处理
- **实现要点**：跨平台API封装、大文件支持
- **测试**：test_io.cpp

#### 5. TCP网络 (net/tcp.h)
- **功能**：TCP客户端/服务器、IPv6支持
- **实现要点**：Winsock/Berkeley Socket统一接口、超时控制
- **测试**：test_net.cpp
- **日志**：使用BASE_LOG_*宏

#### 6. UDP网络 (net/udp.h)
- **功能**：UDP套接字、多播支持
- **实现要点**：跨平台socket封装
- **测试**：test_net.cpp

#### 7. HTTP客户端 (net/http.h)
- **功能**：HTTP GET/POST/PUT/DELETE、URL编解码
- **实现要点**：原始socket实现、HTTP/1.1协议
- **测试**：test_http.cpp

#### 8. 错误处理 (util/result.h)
- **功能**：Result<T, E>类型、ErrorCode枚举
- **标准**：C++11兼容（无C++20 std::expected依赖）
- **实现要点**：C++2010兼容模式
- **测试**：test_result.cpp

#### 9. 配置管理 (util/config.h)
- **功能**：配置加载/保存、JSON格式转换
- **实现要点**：类型安全get/set模板
- **测试**：test_util.cpp

#### 10. 线程池 (util/thread_pool.h)
- **功能**：线程池管理、任务队列、异步任务
- **实现要点**：std::future/std::packaged_task、线程安全队列
- **测试**：test_util.cpp

#### 11. 线程管理 (util/thread.h)
- **功能**：线程创建/启动/停止/暂停/恢复/等待、优先级设置、状态监控
- **跨平台**：Windows/Linux/macOS
- **实现要点**：原生线程封装、RAII资源管理、Result<T,E>错误处理
- **测试**：test_thread.cpp

#### 12. 锁抽象框架 (util/lock.h)
- **功能**：统一锁接口、递归锁、非递归锁、读写锁、锁守卫
- **接口**：ILock抽象类、RecursiveMutex、NonRecursiveMutex、ReadWriteLock
- **实现要点**：跨平台线程ID处理、RAII LockGuard/TryLockGuard
- **测试**：test_lock.cpp

#### 13. 时间工具 (util/time.h)
- **功能**：时间戳、格式化、定时器
- **实现要点**：std::chrono、steady_clock计时
- **测试**：test_util.cpp

#### 14. 日志模块 (core/logger.h)
- **功能**：多级别日志(DEBUG/INFO/WARN/ERROR/FATAL)、文件输出、格式化
- **配置**：LoggerConfig初始化、内部日志开关控制
- **宏**：BASE_LOG_DEBUG、BASE_LOG_INFO、BASE_LOG_WARN、BASE_LOG_ERROR、BASE_LOG_FATAL
- **实现要点**：线程安全、日志轮转、上下文信息(时间戳/模块名/文件名/行号/函数名)
- **测试**：test_logger.cpp

## 项目结构

```
BaseLib/
├── include/           # 头文件目录
│   ├── base.h        # 主头文件
│   ├── core/         # 核心模块
│   │   └── logger.h  # 日志模块
│   ├── io/          # 输入输出
│   │   ├── json.h
│   │   ├── xml.h
│   │   └── filesystem.h
│   ├── memory/       # 内存管理
│   │   └── smart_ptr.h
│   ├── net/         # 网络模块
│   │   ├── tcp.h
│   │   ├── udp.h
│   │   └── http.h
│   └── util/        # 工具模块
│       ├── config.h
│       ├── error.h
│       ├── lock.h
│       ├── result.h
│       ├── thread.h
│       ├── thread_pool.h
│       └── time.h
├── src/              # 源代码
├── tests/            # 单元测试 (12个)
├── scripts/          # 构建脚本
├── archive/          # 文档存档
├── .trae/skills/     # SKILL文档
├── BaseLib.vcxproj  # VS项目文件
├── BaseLib.sln       # VS解决方案
└── CMakeLists.txt    # CMake配置
```

## 编码规范

### 命名规范
- 类名：大写字母开头，如 `Json`、`TcpServer`
- 方法名：小写字母开头，如 `parse`、`send`
- 成员变量：m_前缀，如 `m_ptr`、`m_port`
- 常量：k前缀，如 `kDefaultPort`

### 代码风格
- 使用4空格缩进
- 大括号单独一行
- 命名空间不缩进

### 安全规范
- 防止内存泄漏
- 避免缓冲区溢出
- 处理空指针
- 线程安全保护

### 日志规范
- 使用BASE_LOG_*宏记录日志
- 模块名称使用有意义的标识符
- 错误和致命错误使用BASE_LOG_ERROR/BASE_LOG_FATAL

## 构建系统

### CMake
```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

### Visual Studio
直接打开 BaseLib.sln 或 BaseLib.vcxproj

### 打包脚本
- `scripts/build-win32.bat` - Win32构建
- `scripts/build-win64.bat` - x64构建
- `scripts/build-package.ps1` - PowerShell打包脚本

### 运行测试
```bash
build/Release/test_all.exe     # 所有测试
build/Release/test_logger.exe  # 日志测试
build/Release/test_lock.exe    # 锁测试
build/Release/test_thread.exe  # 线程测试
build/Release/test_result.exe  # Result测试
```

## 问题排查

### 排查步骤
1. 查看错误信息和日志
2. 使用调试器定位问题
3. 检查代码逻辑
4. 查阅官方文档
5. 分析问题根因

### 常见问题
- 编译错误：检查头文件包含和链接库
- 链接错误：检查符号导出和库依赖
- 运行时错误：使用调试器定位
- 性能问题：使用性能分析工具

## 技能提升方向

### 技术能力
1. 深入理解C++标准库
2. 掌握更多设计模式
3. 学习性能优化技巧
4. 理解并发编程

### 工具使用
1. 熟练使用调试器
2. 掌握性能分析工具
3. 使用版本控制系统
4. 使用持续集成工具
