#ifndef QUEUE_H
#define QUEUE_H

class Queue {
private:
    int* m_data;
    int m_front;
    int m_rear;
    int m_capacity;
    int m_size;
public:
    Queue(int capacity = 10);
    ~Queue();
    void enqueue(int value);
    void dequeue();
    int front() const;
    bool isEmpty() const;
    bool isFull() const;
    int size() const;
};

#endif // QUEUE_H