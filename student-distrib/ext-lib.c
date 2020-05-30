#include "ext-lib.h"
/*
description: says if a number is the power of 2
input: number
output 0 if no 1 if yes
sideeffct: none
*/
int is_power_2( unsigned int number){
	if (number == 0){
		return 0;
	}
	while (number != 1){
		if ( (number&1) != 0){	//&1 is the same as mod 2
			return 0;
		}
		number >>= 1;		//equivelent to dividing by 2
	}
	return 1;
}
/*
description: wrapper function for rtc_udelay
input: millisecs to wait
output: look at RTC_udelay
sideeffects: reads rtc global clk
*/
int delay(unsigned long msecs){
	return RTC_udelay(msecs*1000);	// this converts it to micro secs
}

/*
description: helper for min heap
*/
int smaller_child(int* arr, int idx, int NumElems){
	//check if child is within bounds
	if(2*idx+2 < NumElems){
		// return which ever child is smaller
		if( arr[2*idx+1] < arr[2*idx+2]){
			return 2*idx+1;
		}else{
			return 2*idx+2;
		}
	}
	return 2*idx+1;
}

/*
description: helper for min heap
*/
void heapify_down(int CurIdx, int* arr, int NumElems){
	int MinChildIdx, temp;
	// while a child exists 
	while( (CurIdx*2 + 1) < NumElems ){
		// get the smaller child
		MinChildIdx = smaller_child(arr, CurIdx, NumElems);
		// if child is smaller swap 
		if(arr[MinChildIdx] < arr[CurIdx]){
			temp = arr[CurIdx];
			arr[CurIdx] = arr[MinChildIdx];
			arr[MinChildIdx] = temp;
			CurIdx = MinChildIdx;
		}else{
			CurIdx = NumElems;
		}
	}
}

/*
description: helper for min heap
*/
void heapify_up(int idx, int* arr, int NumElems){
	int ParentIdx, temp;
	// while we arnt at the root
	while( idx != 0){
		ParentIdx =  (idx-1)/2;
		// if parent is larger than child swap
		if( arr[idx] < arr[ParentIdx]){
			temp = arr[idx];
			arr[idx] = arr[ParentIdx];
			arr[ParentIdx] = temp;
			idx = ParentIdx;
		}else{
			break;
		}
	}
	return;
}

/*
description: a min heap implemenation
input: an array to sort and number of elements it has
output:
	0 on success
	-1 on null ptr pass
	-2 size incorrect
side effect: none
*/
int create_min_heap(int* arr, int NumElems){
	if( arr == NULL) return -1;
	if( NumElems < -1 ) return -2;
	int idx;
	for(idx= NumElems-1; idx >= 0; idx-- ){
		heapify_down(idx, arr, NumElems);
	}
	// for(idx=0; idx < NumElems; idx++){
	// 	heapify_up(idx, arr, NumElems);
	// }
	return 0;
}

//checks if the array is empty
int is_heap_empty(int* arr){
	return arr[0] == INT_MAX;
}

int is_heap_full(int* arr, int NumElems){
	// int i, temp;
	// for(i=(NumElems - NumElems/2); i < NumElems; i++){
	// 	if(arr[i] == INT_MAX){
	// 		temp =  arr[i];
	// 		arr[i] = arr[NumElems-1];
	// 		arr[NumElems-1]=temp;
	// 		return 0;
	// 	}
	// }
	// return 1;
	return arr[NumElems-1] != INT_MAX;
}
/*
description: a min heap implemenation
input: an heap array to pop off and number of elements it has
output:
	0 on success
	-1 on null ptr pass
	-2 size incorrect
	-3 heap is empty
side effect: int max is placed into to the heap to indicated the val and deleted
*/
int heap_pop(int* arr, int NumElems, int* retval){
	if( arr == NULL) return -1;
	if( NumElems < 1 ) return -2;
	if( is_heap_empty(arr) ) return -3;
	int tail = NumElems-1;
	*retval = arr[0];
	if( is_heap_full(arr,NumElems) ){
		arr[0] = arr[NumElems-1];
		arr[NumElems-1] = INT_MAX;
		heapify_down(0, arr, NumElems);
	}else{
		while(arr[tail] == INT_MAX){
			tail--;
		}
		arr[0] = arr[tail];
		arr[tail]=INT_MAX;
		heapify_down(0,arr,NumElems);
	}
	return 0;
}

/*
description: a min heap implemenation
input:
	val to insert
	array ptr
	size of array
output:
	0 on success
	-1 on null ptr pass
	-2 size incorrect
	-3 array full
side effect: heap has a new val in it
*/
int heap_insert(int val, int* arr, int NumElems){
	if( arr == NULL) return -1;
	if( NumElems < -1 ) return -2;
	if( is_heap_full(arr, NumElems) )	return -3;
	int i;
	for(i=NumElems-1; i>=0; i--){
		if(arr[i] != INT_MAX || i == 0){
			arr[i+1] = val;
			heapify_up(i+1, arr, NumElems);
			break;
		}
	}
	return 0;
}

/*
description: sorts a given array
input: array and its size
	reverse:
		1: max to min
		0: min to max
output:
	0 on succ
	-1 on null
	-2 on wrong size
side effect: sorts array hopefully
*/
int heap_sort(int* arr, int NumElems, int reverse){
	if( arr == NULL) return -1;
	if( NumElems < -1 ) return -2;
	int i, temp;
	create_min_heap(arr, NumElems);
	for(i=NumElems-1; i>=0; i--){
		temp = arr[i];
		arr[i] = arr[0];
		arr[0] = temp;
		heapify_down(0, arr, i);
	}
	if(reverse == 0){
		reverse_array(arr, 0, NumElems-1);
	}
	return 0;
}

/*
description: reverses a given array
input: array, start index and end index
output: none
sideffect: reverses array between specified index
*/
void reverse_array(int* arr, int start, int end){
	// taken from geeks for geeks not my code
	int temp;
	while (start < end){
		temp = arr[start];
		arr[start] = arr[end];
		arr[end] = temp;
		start++;
		end--;
	}
}

/*
description: prints given array
input: array and size of array
output: 
	-1: null array
	0: success
side effect: prints to screen
*/
int print_array(int* arr, int NumElems){
	if(arr ==  NULL)	return -1;
	int i;
	// go through array and print it out
	for(i=0; i<NumElems; i++){
		printf(" %d", arr[i]);
	}
	return 0;
}
