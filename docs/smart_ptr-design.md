# 智能指针模块 - 详细设计文档

## 1. 模块概述

### 1.1 模块名称
`base::memory::smart_ptr`

### 1.2 功能描述
实现C++智能指针，包括`shared_ptr`、`unique_ptr`和`weak_ptr`，提供自动内存管理和循环引用检测功能。

### 1.3 依赖项
- C++11 标准库
- `<atomic>` - 原子操作
- `<mutex>` - 互斥锁

## 2. 接口定义

### 2.1 shared_ptr

```cpp
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
```

### 2.2 unique_ptr

```cpp
template <typename T>
class unique_ptr {
public:
    unique_ptr(T* ptr = nullptr);
    template <typename Deleter>
    unique_ptr(T* ptr, Deleter deleter);
    unique_ptr(const unique_ptr& other) = delete;
    unique_ptr(unique_ptr&& other) noexcept;
    ~unique_ptr();

    unique_ptr& operator=(const unique_ptr& other) = delete;
    unique_ptr& operator=(unique_ptr&& other) noexcept;

    T* get() const;
    T& operator*() const;
    T* operator->() const;
    explicit operator bool() const;

    T* release();
    void reset(T* ptr = nullptr);
    void swap(unique_ptr& other);
};
```

### 2.3 weak_ptr

```cpp
template <typename T>
class weak_ptr {
public:
    weak_ptr() noexcept;
    weak_ptr(const shared_ptr<T>& other) noexcept;
    weak_ptr(const weak_ptr& other) noexcept;
    weak_ptr(weak_ptr&& other) noexcept;
    ~weak_ptr();

    weak_ptr& operator=(const shared_ptr<T>& other) noexcept;
    weak_ptr& operator=(const weak_ptr& other) noexcept;
    weak_ptr& operator=(weak_ptr&& other) noexcept;

    shared_ptr<T> lock() const;
    size_t use_count() const;
    bool expired() const;
    void reset() noexcept;
    void swap(weak_ptr& other) noexcept;
};
```

## 3. 实现细节

### 3.1 shared_ptr实现
- 使用引用计数机制管理内存
- `m_ref_count`: 指向原子引用计数器的指针
- `m_mutex`: 指向互斥锁的指针，用于线程安全
- 当引用计数为0时，自动删除管理的对象

### 3.2 unique_ptr实现
- 独占所有权模式
- 不可复制，只能移动
- 析构时自动删除管理的对象

### 3.3 weak_ptr实现
- 弱引用，不增加引用计数
- 通过`lock()`方法获取`shared_ptr`
- 用于解决循环引用问题

### 3.4 线程安全
- `shared_ptr`使用`std::atomic<size_t>`实现线程安全的引用计数
- `weak_ptr`的`lock()`方法使用`std::mutex`保证线程安全

## 4. 使用示例

### 4.1 shared_ptr使用
```cpp
#include "memory/smart_ptr.h"
using namespace base::memory;

void testSharedPtr() {
    shared_ptr<int> sp1(new int(10));
    shared_ptr<int> sp2 = sp1;

    assert(*sp1 == 10);
    assert(sp1.use_count() == 2);

    sp2.reset(new int(20));
    assert(*sp2 == 20);
}
```

### 4.2 unique_ptr使用
```cpp
void testUniquePtr() {
    unique_ptr<int> up1(new int(100));
    unique_ptr<int> up2 = std::move(up1);

    assert(up1.get() == nullptr);
    assert(*up2 == 100);
}
```

### 4.3 weak_ptr使用
```cpp
void testWeakPtr() {
    shared_ptr<int> sp1(new int(300));
    weak_ptr<int> wp1 = sp1;

    assert(!wp1.expired());
    assert(wp1.use_count() == 1);

    shared_ptr<int> sp2 = wp1.lock();
    assert(*sp2 == 300);

    sp1.reset();
    assert(wp1.expired());
}
```

## 5. 测试结果

### 5.1 功能测试
- ✅ 基本功能测试
- ✅ 线程安全测试
- ✅ 循环引用测试
- ✅ 自定义删除器测试

### 5.2 测试输出
```
Testing shared_ptr...
shared_ptr tests passed!
Testing unique_ptr...
unique_ptr tests passed!
Testing weak_ptr...
weak_ptr tests passed!
Testing thread safety...
Thread safety tests passed!

All smart pointer tests passed!
```

## 6. 交付物清单

- [x] `include/memory/smart_ptr.h` - 头文件
- [x] `tests/test_smart_ptr.cpp` - 测试代码
- [x] 测试通过验证

## 7. 后续工作

- [ ] 添加更多边界条件测试
- [ ] 添加性能基准测试
- [ ] 完善文档