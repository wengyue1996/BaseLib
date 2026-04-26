#ifndef SMART_PTR_H
#define SMART_PTR_H

#include <atomic>
#include <mutex>

namespace base {
namespace memory {

template <typename T>
class shared_ptr;

template <typename T>
class weak_ptr;

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

private:
    T* m_ptr;
    std::atomic<size_t>* m_ref_count;
    std::mutex* m_mutex;

    void release();
    void increment_ref();

    template <typename U>
    friend class weak_ptr;
};

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

private:
    T* m_ptr;
};

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

private:
    T* m_ptr;
    std::atomic<size_t>* m_ref_count;
    std::mutex* m_mutex;
    bool m_expired;
};

template <typename T>
shared_ptr<T>::shared_ptr(T* ptr) : m_ptr(ptr), m_ref_count(nullptr), m_mutex(nullptr) {
    if (ptr) {
        m_ref_count = new std::atomic<size_t>(1);
        m_mutex = new std::mutex();
    }
}

template <typename T>
template <typename Deleter>
shared_ptr<T>::shared_ptr(T* ptr, Deleter deleter) : m_ptr(ptr), m_ref_count(nullptr), m_mutex(nullptr) {
    (void)deleter;
    if (ptr) {
        m_ref_count = new std::atomic<size_t>(1);
        m_mutex = new std::mutex();
    }
}

template <typename T>
shared_ptr<T>::shared_ptr(const shared_ptr& other) : m_ptr(other.m_ptr), m_ref_count(other.m_ref_count), m_mutex(other.m_mutex) {
    increment_ref();
}

template <typename T>
shared_ptr<T>::shared_ptr(shared_ptr&& other) noexcept : m_ptr(other.m_ptr), m_ref_count(other.m_ref_count), m_mutex(other.m_mutex) {
    other.m_ptr = nullptr;
    other.m_ref_count = nullptr;
    other.m_mutex = nullptr;
}

template <typename T>
shared_ptr<T>::~shared_ptr() {
    release();
}

template <typename T>
shared_ptr<T>& shared_ptr<T>::operator=(const shared_ptr& other) {
    if (this != &other) {
        release();
        m_ptr = other.m_ptr;
        m_ref_count = other.m_ref_count;
        m_mutex = other.m_mutex;
        increment_ref();
    }
    return *this;
}

template <typename T>
shared_ptr<T>& shared_ptr<T>::operator=(shared_ptr&& other) noexcept {
    if (this != &other) {
        release();
        m_ptr = other.m_ptr;
        m_ref_count = other.m_ref_count;
        m_mutex = other.m_mutex;
        other.m_ptr = nullptr;
        other.m_ref_count = nullptr;
        other.m_mutex = nullptr;
    }
    return *this;
}

template <typename T>
T* shared_ptr<T>::get() const {
    return m_ptr;
}

template <typename T>
T& shared_ptr<T>::operator*() const {
    return *m_ptr;
}

template <typename T>
T* shared_ptr<T>::operator->() const {
    return m_ptr;
}

template <typename T>
shared_ptr<T>::operator bool() const {
    return m_ptr != nullptr;
}

template <typename T>
size_t shared_ptr<T>::use_count() const {
    if (m_ref_count) {
        return m_ref_count->load();
    }
    return 0;
}

template <typename T>
void shared_ptr<T>::reset(T* ptr) {
    release();
    m_ptr = ptr;
    if (ptr) {
        m_ref_count = new std::atomic<size_t>(1);
        m_mutex = new std::mutex();
    }
}

template <typename T>
void shared_ptr<T>::swap(shared_ptr& other) {
    std::swap(m_ptr, other.m_ptr);
    std::swap(m_ref_count, other.m_ref_count);
    std::swap(m_mutex, other.m_mutex);
}

template <typename T>
void shared_ptr<T>::release() {
    if (m_ref_count) {
        m_ref_count->fetch_sub(1);
        if (m_ref_count->load() == 0) {
            delete m_ptr;
            delete m_ref_count;
            delete m_mutex;
        }
    }
    m_ptr = nullptr;
    m_ref_count = nullptr;
    m_mutex = nullptr;
}

template <typename T>
void shared_ptr<T>::increment_ref() {
    if (m_ref_count) {
        m_ref_count->fetch_add(1);
    }
}

template <typename T>
unique_ptr<T>::unique_ptr(T* ptr) : m_ptr(ptr) {}

template <typename T>
template <typename Deleter>
unique_ptr<T>::unique_ptr(T* ptr, Deleter deleter) : m_ptr(ptr) {
    (void)deleter;
}

template <typename T>
unique_ptr<T>::unique_ptr(unique_ptr&& other) noexcept : m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
}

template <typename T>
unique_ptr<T>::~unique_ptr() {
    delete m_ptr;
}

template <typename T>
unique_ptr<T>& unique_ptr<T>::operator=(unique_ptr&& other) noexcept {
    if (this != &other) {
        delete m_ptr;
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
    }
    return *this;
}

template <typename T>
T* unique_ptr<T>::get() const {
    return m_ptr;
}

template <typename T>
T& unique_ptr<T>::operator*() const {
    return *m_ptr;
}

template <typename T>
T* unique_ptr<T>::operator->() const {
    return m_ptr;
}

template <typename T>
unique_ptr<T>::operator bool() const {
    return m_ptr != nullptr;
}

template <typename T>
T* unique_ptr<T>::release() {
    T* old_ptr = m_ptr;
    m_ptr = nullptr;
    return old_ptr;
}

template <typename T>
void unique_ptr<T>::reset(T* ptr) {
    delete m_ptr;
    m_ptr = ptr;
}

template <typename T>
void unique_ptr<T>::swap(unique_ptr& other) {
    std::swap(m_ptr, other.m_ptr);
}

template <typename T>
weak_ptr<T>::weak_ptr() noexcept : m_ptr(nullptr), m_ref_count(nullptr), m_mutex(nullptr), m_expired(true) {}

template <typename T>
weak_ptr<T>::weak_ptr(const shared_ptr<T>& other) noexcept : m_ptr(other.m_ptr), m_ref_count(other.m_ref_count), m_mutex(other.m_mutex), m_expired(false) {
    if (m_ref_count) {
        m_ref_count->fetch_add(1);
    }
}

template <typename T>
weak_ptr<T>::weak_ptr(const weak_ptr& other) noexcept : m_ptr(other.m_ptr), m_ref_count(other.m_ref_count), m_mutex(other.m_mutex), m_expired(other.m_expired) {
    if (m_ref_count && !m_expired) {
        m_ref_count->fetch_add(1);
    }
}

template <typename T>
weak_ptr<T>::weak_ptr(weak_ptr&& other) noexcept : m_ptr(other.m_ptr), m_ref_count(other.m_ref_count), m_mutex(other.m_mutex), m_expired(other.m_expired) {
    other.m_ptr = nullptr;
    other.m_ref_count = nullptr;
    other.m_mutex = nullptr;
    other.m_expired = true;
}

template <typename T>
weak_ptr<T>::~weak_ptr() {
    if (m_ref_count && !m_expired) {
        m_ref_count->fetch_sub(1);
    }
}

template <typename T>
weak_ptr<T>& weak_ptr<T>::operator=(const shared_ptr<T>& other) noexcept {
    if (m_ref_count && !m_expired) {
        m_ref_count->fetch_sub(1);
    }
    m_ptr = other.m_ptr;
    m_ref_count = other.m_ref_count;
    m_mutex = other.m_mutex;
    m_expired = false;
    if (m_ref_count) {
        m_ref_count->fetch_add(1);
    }
    return *this;
}

template <typename T>
weak_ptr<T>& weak_ptr<T>::operator=(const weak_ptr& other) noexcept {
    if (this != &other) {
        if (m_ref_count && !m_expired) {
            m_ref_count->fetch_sub(1);
        }
        m_ptr = other.m_ptr;
        m_ref_count = other.m_ref_count;
        m_mutex = other.m_mutex;
        m_expired = other.m_expired;
        if (m_ref_count && !m_expired) {
            m_ref_count->fetch_add(1);
        }
    }
    return *this;
}

template <typename T>
weak_ptr<T>& weak_ptr<T>::operator=(weak_ptr&& other) noexcept {
    if (this != &other) {
        if (m_ref_count && !m_expired) {
            m_ref_count->fetch_sub(1);
        }
        m_ptr = other.m_ptr;
        m_ref_count = other.m_ref_count;
        m_mutex = other.m_mutex;
        m_expired = other.m_expired;
        other.m_ptr = nullptr;
        other.m_ref_count = nullptr;
        other.m_mutex = nullptr;
        other.m_expired = true;
    }
    return *this;
}

template <typename T>
shared_ptr<T> weak_ptr<T>::lock() const {
    if (expired()) {
        return shared_ptr<T>();
    }
    shared_ptr<T> sp;
    sp.m_ptr = m_ptr;
    sp.m_ref_count = m_ref_count;
    sp.m_mutex = m_mutex;
    if (sp.m_ref_count) {
        sp.m_ref_count->fetch_add(1);
    }
    return sp;
}

template <typename T>
size_t weak_ptr<T>::use_count() const {
    if (m_ref_count) {
        return m_ref_count->load();
    }
    return 0;
}

template <typename T>
bool weak_ptr<T>::expired() const {
    return m_expired || !m_ref_count || m_ref_count->load() == 0;
}

template <typename T>
void weak_ptr<T>::reset() noexcept {
    if (m_ref_count && !m_expired) {
        m_ref_count->fetch_sub(1);
    }
    m_ptr = nullptr;
    m_ref_count = nullptr;
    m_mutex = nullptr;
    m_expired = true;
}

template <typename T>
void weak_ptr<T>::swap(weak_ptr& other) noexcept {
    std::swap(m_ptr, other.m_ptr);
    std::swap(m_ref_count, other.m_ref_count);
    std::swap(m_mutex, other.m_mutex);
    std::swap(m_expired, other.m_expired);
}

} // namespace memory
} // namespace base

#endif // SMART_PTR_H