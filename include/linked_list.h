#ifndef LINKED_LIST_H
#define LINKED_LIST_H

class ListNode {
private:
    int m_value;
    ListNode* m_next;
public:
    ListNode(int value);
    ~ListNode();
    int getValue() const;
    ListNode* getNext() const;
    void setNext(ListNode* next);
};

class LinkedList {
private:
    ListNode* m_head;
    int m_size;
public:
    LinkedList();
    ~LinkedList();
    void add(int value);
    void remove(int value);
    bool contains(int value) const;
    int size() const;
    void clear();
};

#endif // LINKED_LIST_H