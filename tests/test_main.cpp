#include <iostream>
#include "../include/linked_list.h"
#include "../include/stack.h"
#include "../include/queue.h"
#include "../include/sorting.h"
#include "../include/searching.h"

using namespace std;

void testLinkedList() {
    cout << "=== Testing LinkedList ===" << endl;
    
    LinkedList list;
    
    // Test add
    list.add(1);
    list.add(2);
    list.add(3);
    cout << "Size after add: " << list.size() << endl;
    
    // Test contains
    cout << "Contains 2: " << (list.contains(2) ? "true" : "false") << endl;
    cout << "Contains 4: " << (list.contains(4) ? "true" : "false") << endl;
    
    // Test remove
    list.remove(2);
    cout << "Size after remove: " << list.size() << endl;
    cout << "Contains 2: " << (list.contains(2) ? "true" : "false") << endl;
    
    // Test clear
    list.clear();
    cout << "Size after clear: " << list.size() << endl;
    cout << "Contains 1: " << (list.contains(1) ? "true" : "false") << endl;
    cout << "" << endl;
}

void testStack() {
    cout << "=== Testing Stack ===" << endl;
    
    Stack stack(3);
    
    // Test push
    stack.push(1);
    stack.push(2);
    stack.push(3);
    cout << "Size after push: " << stack.size() << endl;
    cout << "Top element: " << stack.top() << endl;
    
    // Test pop
    stack.pop();
    cout << "Size after pop: " << stack.size() << endl;
    cout << "Top element: " << stack.top() << endl;
    
    // Test empty
    stack.pop();
    stack.pop();
    cout << "Is empty: " << (stack.isEmpty() ? "true" : "false") << endl;
    cout << "" << endl;
}

void testQueue() {
    cout << "=== Testing Queue ===" << endl;
    
    Queue queue(3);
    
    // Test enqueue
    queue.enqueue(1);
    queue.enqueue(2);
    queue.enqueue(3);
    cout << "Size after enqueue: " << queue.size() << endl;
    cout << "Front element: " << queue.front() << endl;
    
    // Test dequeue
    queue.dequeue();
    cout << "Size after dequeue: " << queue.size() << endl;
    cout << "Front element: " << queue.front() << endl;
    
    // Test empty
    queue.dequeue();
    queue.dequeue();
    cout << "Is empty: " << (queue.isEmpty() ? "true" : "false") << endl;
    cout << "" << endl;
}

void testSorting() {
    cout << "=== Testing Sorting ===" << endl;
    
    int arr[] = {3, 1, 4, 1, 5, 9, 2, 6};
    int size = sizeof(arr) / sizeof(arr[0]);
    
    cout << "Original array: ";
    for (int i = 0; i < size; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;
    
    // Test bubble sort
    int bubbleArr[] = {3, 1, 4, 1, 5, 9, 2, 6};
    Sorting::bubbleSort(bubbleArr, size);
    cout << "Bubble sort: ";
    for (int i = 0; i < size; i++) {
        cout << bubbleArr[i] << " ";
    }
    cout << endl;
    
    // Test quick sort
    int quickArr[] = {3, 1, 4, 1, 5, 9, 2, 6};
    Sorting::quickSort(quickArr, 0, size - 1);
    cout << "Quick sort: ";
    for (int i = 0; i < size; i++) {
        cout << quickArr[i] << " ";
    }
    cout << endl;
    
    // Test merge sort
    int mergeArr[] = {3, 1, 4, 1, 5, 9, 2, 6};
    Sorting::mergeSort(mergeArr, 0, size - 1);
    cout << "Merge sort: ";
    for (int i = 0; i < size; i++) {
        cout << mergeArr[i] << " ";
    }
    cout << endl;
    cout << "" << endl;
}

void testSearching() {
    cout << "=== Testing Searching ===" << endl;
    
    int arr[] = {3, 1, 4, 1, 5, 9, 2, 6};
    int sortedArr[] = {1, 1, 2, 3, 4, 5, 6, 9};
    int size = sizeof(arr) / sizeof(arr[0]);
    
    // Test linear search
    int linearIndex = Searching::linearSearch(arr, size, 5);
    cout << "Linear search for 5: index " << linearIndex << endl;
    int linearIndexNotFound = Searching::linearSearch(arr, size, 7);
    cout << "Linear search for 7: index " << linearIndexNotFound << endl;
    
    // Test binary search
    int binaryIndex = Searching::binarySearch(sortedArr, size, 5);
    cout << "Binary search for 5: index " << binaryIndex << endl;
    int binaryIndexNotFound = Searching::binarySearch(sortedArr, size, 7);
    cout << "Binary search for 7: index " << binaryIndexNotFound << endl;
    cout << "" << endl;
}

int main() {
    testLinkedList();
    testStack();
    testQueue();
    testSorting();
    testSearching();
    
    cout << "All tests completed!" << endl;
    return 0;
}