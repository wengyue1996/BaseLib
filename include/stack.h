#ifndef STACK_H
#define STACK_H

class Stack {
private:
    int* m_data;
    int m_top;
    int m_capacity;
public:
    Stack(int capacity = 10);
    ~Stack();
    void push(int value);
    void pop();
    int top() const;
    bool isEmpty() const;
    bool isFull() const;
    int size() const;
};

#endif // STACK_H