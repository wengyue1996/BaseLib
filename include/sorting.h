#ifndef SORTING_H
#define SORTING_H

class Sorting {
public:
    static void bubbleSort(int arr[], int size);
    static void selectionSort(int arr[], int size);
    static void insertionSort(int arr[], int size);
    static void quickSort(int arr[], int low, int high);
    static void mergeSort(int arr[], int low, int high);
};

#endif // SORTING_H