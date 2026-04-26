#include "../include/linked_list.h"

// ListNode implementation
ListNode::ListNode(int value) : m_value(value), m_next(nullptr) {}

ListNode::~ListNode() {}

int ListNode::getValue() const {
    return m_value;
}

ListNode* ListNode::getNext() const {
    return m_next;
}

void ListNode::setNext(ListNode* next) {
    m_next = next;
}

// LinkedList implementation
LinkedList::LinkedList() : m_head(nullptr), m_size(0) {}

LinkedList::~LinkedList() {
    clear();
}

void LinkedList::add(int value) {
    ListNode* newNode = new ListNode(value);
    if (m_head == nullptr) {
        m_head = newNode;
    } else {
        ListNode* current = m_head;
        while (current->getNext() != nullptr) {
            current = current->getNext();
        }
        current->setNext(newNode);
    }
    m_size++;
}

void LinkedList::remove(int value) {
    if (m_head == nullptr) {
        return;
    }
    
    if (m_head->getValue() == value) {
        ListNode* temp = m_head;
        m_head = m_head->getNext();
        delete temp;
        m_size--;
        return;
    }
    
    ListNode* current = m_head;
    while (current->getNext() != nullptr) {
        if (current->getNext()->getValue() == value) {
            ListNode* temp = current->getNext();
            current->setNext(temp->getNext());
            delete temp;
            m_size--;
            return;
        }
        current = current->getNext();
    }
}

bool LinkedList::contains(int value) const {
    ListNode* current = m_head;
    while (current != nullptr) {
        if (current->getValue() == value) {
            return true;
        }
        current = current->getNext();
    }
    return false;
}

int LinkedList::size() const {
    return m_size;
}

void LinkedList::clear() {
    while (m_head != nullptr) {
        ListNode* temp = m_head;
        m_head = m_head->getNext();
        delete temp;
    }
    m_size = 0;
}