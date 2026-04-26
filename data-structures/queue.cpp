#include "../include/queue.h"
#include <stdexcept>

Queue::Queue(int capacity) : m_capacity(capacity), m_front(0), m_rear(-1), m_size(0) {
    m_data = new int[capacity];
}

Queue::~Queue() {
    delete[] m_data;
}

void Queue::enqueue(int value) {
    if (isFull()) {
        throw std::overflow_error("Queue is full");
    }
    m_rear = (m_rear + 1) % m_capacity;
    m_data[m_rear] = value;
    m_size++;
}

void Queue::dequeue() {
    if (isEmpty()) {
        throw std::underflow_error("Queue is empty");
    }
    m_front = (m_front + 1) % m_capacity;
    m_size--;
}

int Queue::front() const {
    if (isEmpty()) {
        throw std::underflow_error("Queue is empty");
    }
    return m_data[m_front];
}

bool Queue::isEmpty() const {
    return m_size == 0;
}

bool Queue::isFull() const {
    return m_size == m_capacity;
}

int Queue::size() const {
    return m_size;
}