# C++基础库 - 构建说明

## 项目结构

```
e:\BaseLib\
├── include/          # 头文件目录
│   ├── core/         # 核心模块（logger）
│   ├── memory/       # 内存模块（smart_ptr）
│   ├── io/           # 输入输出模块（json, xml, filesystem）
│   ├── net/          # 网络模块（tcp, udp）
│   ├── util/         # 工具模块（config, error, thread_pool, time）
│   └── base.h        # 主头文件
├── src/              # 源文件目录
│   ├── core/
│   ├── memory/
│   ├── io/
│   ├── net/
│   └── util/
├── tests/            # 测试目录
│   ├── test_logger.cpp
│   ├── test_smart_ptr.cpp
│   ├── test_io.cpp
│   ├── test_net.cpp
│   └── test_util.cpp
├── examples/         # 示例目录
│   └── log_demo.cpp
├── build/            # 构建目录
└── CMakeLists.txt    # CMake构建配置
```

## 构建步骤

### Windows (Visual Studio)

1. 创建build目录并进入：
```bash
mkdir build
cd build
```

2. 使用CMake生成Visual Studio项目：
```bash
cmake .. -G "Visual Studio 16 2019" -A x64
```

3. 编译项目：
```bash
cmake --build . --config Release
```

### Linux / macOS

1. 创建build目录并进入：
```bash
mkdir build
cd build
```

2. 使用CMake配置：
```bash
cmake ..
```

3. 编译项目：
```bash
cmake --build . --config Release
```

## 生成的文件

构建完成后，在 `build/Release/` 目录下会生成以下文件：

### 库文件
- `BaseLibStatic.lib` - 静态库（Windows）
- `BaseLibShared.dll` - 动态库（Windows）
- `libBaseLibStatic.a` - 静态库（Linux/macOS）
- `libBaseLibShared.so` - 动态库（Linux）
- `libBaseLibShared.dylib` - 动态库（macOS）

### 可执行文件
- `test_logger.exe` / `test_logger` - 日志模块测试程序
- `log_demo.exe` / `log_demo` - 日志模块演示程序
- `test_smart_ptr.exe` / `test_smart_ptr` - 智能指针测试程序
- `test_io.exe` / `test_io` - 文件IO测试程序
- `test_net.exe` / `test_net` - 网络测试程序
- `test_util.exe` / `test_util` - 工具模块测试程序

## 运行测试

### Windows
```bash
cd build/Release
test_logger.exe
test_smart_ptr.exe
test_io.exe
test_net.exe
test_util.exe
```

### Linux / macOS
```bash
cd build
./test_logger
./test_smart_ptr
./test_io
./test_net
./test_util
```

## 模块列表

| 模块 | 头文件 | 功能描述 |
|------|--------|----------|
| Logger | include/core/logger.h | 分级日志系统 |
| Smart Pointer | include/memory/smart_ptr.h | 智能指针实现 |
| JSON | include/io/json.h | JSON解析和生成 |
| XML | include/io/xml.h | XML解析和生成 |
| FileSystem | include/io/filesystem.h | 文件系统操作 |
| TCP | include/net/tcp.h | TCP网络通信 |
| UDP | include/net/udp.h | UDP网络通信 |
| Config | include/util/config.h | 配置管理 |
| Error | include/util/error.h | 错误处理 |
| ThreadPool | include/util/thread_pool.h | 线程池 |
| Time | include/util/time.h | 时间工具 |

## 注意事项

1. 项目使用C++11标准
2. 不依赖第三方库（除标准库外）
3. 支持跨平台（Windows、Linux、macOS）
4. 所有模块都经过基本测试