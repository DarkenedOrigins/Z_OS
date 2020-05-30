#ifndef EXT_LIB_H
#define EXT_LIB_H

#include "rtc.h"	//udelay is a wrapper for rtc udelay
#include "types.h"


//code for finding if a number is the power of 2
int is_power_2(unsigned int number);
//a millisec delay func
int delay(unsigned long msecs);


int create_min_heap(int* arr, int NumElems);
int heap_pop(int* arr, int NumElems, int* retval);
int heap_insert(int val, int* arr, int NumElems);
int heap_sort(int* arr, int NumElems, int reverse);
void reverse_array(int* arr, int start, int end);
int is_heap_empty(int* arr);
int is_heap_full(int* arr, int NumElems);
int print_array(int* arr, int NumElems);
#endif
