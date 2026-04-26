#ifndef BASE_SMART_PTR_H
#define BASE_SMART_PTR_H

#include <atomic>
#include <utility>

namespace base {
namespace memory {

struct ControlBlockBase {
    std::atomic<size_t> strong_ref;
    std::atomic<size_t> weak_ref;

    ControlBlockBase() : strong_ref(0), weak_ref(0) {}

    virtual ~ControlBlockBase() = default;
    virtual void destroyObject() = 0;
};

template <typename T>
struct ControlBlock : public ControlBlockBase {
    T* ptr;

    template <typename Deleter>
    ControlBlock(T* p, Deleter) : ptr(p) {}

    void destroyObject() override {
        delete ptr;
        ptr = nullptr;
    }
};

template <typename T>
class shared_ptr;

template <typename T>
class unique_ptr;

template <typename T>
class weak_ptr;

template <typename T>
class shared_ptr {
public:
    constexpr shared_ptr() noexcept : m_ptr(nullptr), m_block(nullptr) {}

    explicit shared_ptr(T* ptr)
        : m_ptr(ptr), m_block(ptr ? new ControlBlock<T>(ptr, [](T* p){ delete p; }) : nullptr) {
        if (m_block) {
            m_block->strong_ref.store(1);
        }
    }

    shared_ptr(const shared_ptr& other) noexcept
        : m_ptr(other.m_ptr), m_block(other.m_block) {
        if (m_block) {
            m_block->strong_ref.fetch_add(1);
        }
    }

    shared_ptr(shared_ptr&& other) noexcept
        : m_ptr(other.m_ptr), m_block(other.m_block) {
        other.m_ptr = nullptr;
        other.m_block = nullptr;
    }

    template <typename U>
    shared_ptr(const shared_ptr<U>& other) noexcept
        : m_ptr(other.m_ptr), m_block(other.m_block) {
        if (m_block) {
            m_block->strong_ref.fetch_add(1);
        }
    }

    template <typename U>
    shared_ptr(shared_ptr<U>&& other) noexcept
        : m_ptr(other.m_ptr), m_block(other.m_block) {
        other.m_ptr = nullptr;
        other.m_block = nullptr;
    }

    ~shared_ptr() {
        release();
    }

    shared_ptr& operator=(const shared_ptr& other) noexcept {
        if (this != &other) {
            release();
            m_ptr = other.m_ptr;
            m_block = other.m_block;
            if (m_block) {
                m_block->strong_ref.fetch_add(1);
            }
        }
        return *this;
    }

    shared_ptr& operator=(shared_ptr&& other) noexcept {
        if (this != &other) {
            release();
            m_ptr = other.m_ptr;
            m_block = other.m_block;
            other.m_ptr = nullptr;
            other.m_block = nullptr;
        }
        return *this;
    }

    T* get() const noexcept { return m_ptr; }
    T& operator*() const noexcept { return *m_ptr; }
    T* operator->() const noexcept { return m_ptr; }

    explicit operator bool() const noexcept { return m_ptr != nullptr; }

    size_t use_count() const noexcept {
        return m_block ? m_block->strong_ref.load() : 0;
    }

    bool unique() const noexcept {
        return use_count() == 1;
    }

    void reset() noexcept {
        release();
        m_ptr = nullptr;
        m_block = nullptr;
    }

    void swap(shared_ptr& other) noexcept {
        std::swap(m_ptr, other.m_ptr);
        std::swap(m_block, other.m_block);
    }

private:
    T* m_ptr;
    ControlBlockBase* m_block;

    void release() {
        if (m_block) {
            if (m_block->strong_ref.fetch_sub(1) == 1) {
                if (m_ptr) {
                    m_block->destroyObject();
                    m_ptr = nullptr;
                }
            }
            if (m_block->strong_ref.load() == 0 && m_block->weak_ref.load() == 0) {
                delete m_block;
            }
            m_block = nullptr;
        }
    }

    template <typename U>
    friend class weak_ptr;
};

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
    return shared_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
class unique_ptr {
public:
    constexpr unique_ptr() noexcept : m_ptr(nullptr) {}

    explicit unique_ptr(T* ptr) noexcept : m_ptr(ptr) {}

    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;

    unique_ptr(unique_ptr&& other) noexcept : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    unique_ptr& operator=(unique_ptr&& other) noexcept {
        if (this != &other) {
            delete m_ptr;
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    ~unique_ptr() {
        delete m_ptr;
    }

    T* get() const noexcept { return m_ptr; }
    T& operator*() const { return *m_ptr; }
    T* operator->() const noexcept { return m_ptr; }

    explicit operator bool() const noexcept { return m_ptr != nullptr; }

    T* release() noexcept {
        T* tmp = m_ptr;
        m_ptr = nullptr;
        return tmp;
    }

    void reset() noexcept {
        delete m_ptr;
        m_ptr = nullptr;
    }

    void swap(unique_ptr& other) noexcept {
        std::swap(m_ptr, other.m_ptr);
    }

private:
    T* m_ptr;
};

template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
class weak_ptr {
public:
    constexpr weak_ptr() noexcept : m_ptr(nullptr), m_block(nullptr) {}

    weak_ptr(const shared_ptr<T>& other) noexcept
        : m_ptr(other.m_ptr), m_block(other.m_block) {
        if (m_block) {
            m_block->weak_ref.fetch_add(1);
        }
    }

    weak_ptr(const weak_ptr& other) noexcept
        : m_ptr(other.m_ptr), m_block(other.m_block) {
        if (m_block) {
            m_block->weak_ref.fetch_add(1);
        }
    }

    weak_ptr(weak_ptr&& other) noexcept
        : m_ptr(other.m_ptr), m_block(other.m_block) {
        other.m_ptr = nullptr;
        other.m_block = nullptr;
    }

    weak_ptr& operator=(const weak_ptr& other) noexcept {
        if (this != &other) {
            release();
            m_ptr = other.m_ptr;
            m_block = other.m_block;
            if (m_block) {
                m_block->weak_ref.fetch_add(1);
            }
        }
        return *this;
    }

    weak_ptr& operator=(const shared_ptr<T>& other) noexcept {
        release();
        m_ptr = other.m_ptr;
        m_block = other.m_block;
        if (m_block) {
            m_block->weak_ref.fetch_add(1);
        }
        return *this;
    }

    weak_ptr& operator=(weak_ptr&& other) noexcept {
        if (this != &other) {
            release();
            m_ptr = other.m_ptr;
            m_block = other.m_block;
            other.m_ptr = nullptr;
            other.m_block = nullptr;
        }
        return *this;
    }

    ~weak_ptr() {
        release();
    }

    shared_ptr<T> lock() const noexcept {
        if (expired()) {
            return shared_ptr<T>();
        }
        shared_ptr<T> result;
        result.m_ptr = m_ptr;
        result.m_block = m_block;
        if (result.m_block) {
            result.m_block->strong_ref.fetch_add(1);
        }
        return result;
    }

    size_t use_count() const noexcept {
        return m_block ? m_block->strong_ref.load() : 0;
    }

    bool expired() const noexcept {
        return !m_block || m_block->strong_ref.load() == 0;
    }

    void reset() noexcept {
        release();
        m_ptr = nullptr;
        m_block = nullptr;
    }

    void swap(weak_ptr& other) noexcept {
        std::swap(m_ptr, other.m_ptr);
        std::swap(m_block, other.m_block);
    }

private:
    T* m_ptr;
    ControlBlockBase* m_block;

    void release() {
        if (m_block) {
            if (m_block->weak_ref.fetch_sub(1) == 1) {
                if (m_block->strong_ref.load() == 0) {
                    delete m_block;
                }
            }
            m_block = nullptr;
        }
    }
};

template <typename T>
void swap(shared_ptr<T>& a, shared_ptr<T>& b) noexcept {
    a.swap(b);
}

template <typename T>
void swap(unique_ptr<T>& a, unique_ptr<T>& b) noexcept {
    a.swap(b);
}

template <typename T>
void swap(weak_ptr<T>& a, weak_ptr<T>& b) noexcept {
    a.swap(b);
}

} // namespace memory
} // namespace base

#endif // BASE_SMART_PTR_H
