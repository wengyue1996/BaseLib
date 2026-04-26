#include "../include/stack.h"
#include <stdexcept>

Stack::Stack(int capacity) : m_capacity(capacity), m_top(-1) {
    m_data = new int[capacity];
}

Stack::~Stack() {
    delete[] m_data;
}

void Stack::push(int value) {
    if (isFull()) {
        throw std::overflow_error("Stack is full");
    }
    m_data[++m_top] = value;
}

void Stack::pop() {
    if (isEmpty()) {
        throw std::underflow_error("Stack is empty");
    }
    m_top--;
}

int Stack::top() const {
    if (isEmpty()) {
        throw std::underflow_error("Stack is empty");
    }
    return m_data[m_top];
}

bool Stack::isEmpty() const {
    return m_top == -1;
}

bool Stack::isFull() const {
    return m_top == m_capacity - 1;
}

int Stack::size() const {
    return m_top + 1;
}