#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "ext-lib.h"
#include "idt_common.h"
#include "paging.h"
#include "terminal.h"
#include "fs.h"

#define PASS 1
#define FAIL 0
#define SKIP_FAIL 1

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	if (!SKIP_FAIL)
		asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Exceptions
 *
 * Asserts that the Trap gates in IDT are correct
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int test_idt_exceptions(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < IRQ_OFFSET; ++i){
		/* Test Handler pointer */
		if ((idt[i].offset_15_00 == NULL) &&
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
			break;
		}
		/* Test Segment selector */
		if (idt[i].seg_selector != KERNEL_CS) {
			assertion_failure();
			result = FAIL;
			break;
		}
		/* Test Reserved Fields and Size */
		if (idt[i].reserved1 != 1 || idt[i].reserved2 != 1 ||
			  idt[i].reserved3 != 1 || idt[i].reserved4 != 0 || idt[i].size != 1) {
			assertion_failure();
			result = FAIL;
			break;
		}
		/* Test Priviledge Level and Present */
		if (idt[i].dpl != 0 || idt[i].present == 0) {
			assertion_failure();
			result = FAIL;
			break;
		}
	}

	return result;
}


/* IDT Interrupts
 *
 * Asserts that the interrupt gates in IDT are correct
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int test_idt_interrupts(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = IRQ_OFFSET; i < NUM_VEC; i++){
		if (i != KEY_IRQ && i != RTC_IRQ && i != SYSCALL_GATE) {
			if (idt[i].present != 0) {
				assertion_failure();
				result = FAIL;
				break;
			}
		} else if (i == SYSCALL_GATE) {
						/* Test Handler pointer */
			if ((idt[i].offset_15_00 == NULL) &&
				(idt[i].offset_31_16 == NULL)){
				assertion_failure();
				result = FAIL;
				break;
			}
			/* Test Segment selector */
			if (idt[i].seg_selector != KERNEL_CS) {
				assertion_failure();
				result = FAIL;
				break;
			}
			/* Test Reserved Fields and Size */
			if (idt[i].reserved1 != 1 || idt[i].reserved2 != 1 ||
				  idt[i].reserved3 != 1 || idt[i].reserved4 != 0 || idt[i].size != 1) {
				assertion_failure();
				result = FAIL;
				break;
			}
			/* Test Priviledge Level and Present */
			if (idt[i].dpl != USER_PERMISSION || idt[i].present == 0) {
				assertion_failure();
				result = FAIL;
				break;
			}
		} else {
			/* Test Handler pointer */
			if ((idt[i].offset_15_00 == NULL) &&
				(idt[i].offset_31_16 == NULL)){
				assertion_failure();
				result = FAIL;
				break;
			}
			/* Test Segment selector */
			if (idt[i].seg_selector != KERNEL_CS) {
				assertion_failure();
				result = FAIL;
				break;
			}
			/* Test Reserved Fields and Size */
			if (idt[i].reserved1 != 1 || idt[i].reserved2 != 1 ||
				  idt[i].reserved3 != 0 || idt[i].reserved4 != 0 || idt[i].size != 1) {
				assertion_failure();
				result = FAIL;
				break;
			}
			/* Test Priviledge Level and Present */
			if (idt[i].dpl != KERNEL_PERMISSION || idt[i].present == 0) {
				assertion_failure();
				result = FAIL;
				break;
			}
		}
	}

	return result;
}


/* Test page faults
 *
 * This should pagefault by dereferencing NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Page faults
 * Files: paging.h/c
 */
int test_pagefault(){
	TEST_HEADER;

	int i, result = PASS;
	int* test = 0;
	*test = i;
	return result;
}


/* Test page faults by referencing vmem
 *
 * This should reference successfully
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Page faults
 * Files: paging.h/c
 */
int test_pagefault_vmem(){
	TEST_HEADER;

	//clear();
	int result = PASS;
	char* video_mem = (char *)VIDEO;
	*(uint8_t *)(video_mem) = (uint8_t) 'A';
	return result;
}

/* Tests the syscall handler by performing a syscall
 *
 * This should call the dummy handler successfully
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Syscalls, IDT
 * Files: IRQ.{c.h}, interrupt.{S,h}
 */
int test_syscall_handler(){
	TEST_HEADER;

	int result = PASS;
	asm volatile ("int $0x80"); // Make a system call
	return result;
}

/* Tests the IDT by attempting to call a nonpresent IDT segment
 *
 * This should throw a nonpresent segment fault
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Interrupts, Exceptions, IDT
 * Files: IRQ.{c.h}, interrupt.{S,h}
 */
int test_invalid_int(){
	TEST_HEADER;
	int result = PASS;
	asm volatile ("int $0x90");
	return result;
}

/* Tests the keyboard driver by simulating all possible scancodes
 *
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: terminal.h/c
 * Files: IRQ.{c.h}, interrupt.{S,h}
 */
int test_spam_keyboard(){
	TEST_HEADER;
	int result = PASS;
	int i = 0;
	for (i = 0; i < 256; i++) {
		outb(i, KEYBOARD_SCAN_CODE_PORT);
		asm volatile ("int $0x21");
	}
	return result;
}

/* Checkpoint 2 tests */

/* Ask user if the test was passed.
 *
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: prompts user
 */
int test_manual(){
	int i;
	uint32_t size = 200;
	int8_t read_buff[size];
	while (1) {
			for (i = 0; i < size; i++)
					read_buff[i] = '\0';
			printf("Did this test pass? (y/n) ");
			terminal_read(0, read_buff, size);
			if (strncmp("y", read_buff, 1) == 0 || strncmp("yes", read_buff, 3) == 0)
					return PASS;
			if (strncmp("n", read_buff, 1) == 0 || strncmp("no", read_buff, 2) == 0)
					return FAIL;
	}
}

/* Tests the stdout
 *
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: writes to vmem
 * Coverage: terminal.h/c
 */
int test_terminal_write(){
	TEST_HEADER;
	int32_t i, retval;
	uint32_t size;
	size = 256;
	int8_t buff[size];
	for (i = 0; i < size-1; i++) {
		buff[i] = (int8_t)i+1;
	}
	buff[size-1] = '\0';
	printf("\n");
	retval = terminal_write(0, NULL, size);
	if (retval != -1)
			return FAIL;
	retval = terminal_write(0, buff, 0);
	if (retval != 0)
			return FAIL;
	retval = terminal_write(0, buff, size);
	printf("\n(%d, %d)\n", size, retval);
	if (size == retval)
			return PASS;
	return FAIL;
}

/* Tests the stdin
 *
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: writes to vmem
 * Coverage: terminal.h/c
 */
int test_terminal_read(){
	TEST_HEADER;
	int i;
	uint32_t size = 200;
	int8_t read_buff[size];
	printf("Type quit to exit this test.\n");
	while (1) {
			for (i = 0; i < size; i++)
					read_buff[i] = '\0';
			printf(">>>> ");
			int read_bytes = terminal_read(0, read_buff, size);
			printf("GOT: %sLEN: %d\n", read_buff, read_bytes);
			if (strncmp("quit", read_buff, 4) == 0) break;
	}
	return test_manual();
}

// /*
// description: checks to see if the rtc rejects an invalid id
// input: none
// output: pass or fail
// side effects: none
// */
// int test_rtc_invalid_id(void){
// 	TEST_HEADER;
// 	int result = PASS;
// 	int retval = set_RTC(35,2); // invalid id valid freq
// 	retval = read_RTC(35);		//invalid id
// 	if(retval == 0){
// 		result = FAIL;
// 	}
// 	return result;
// }
//
// /*
// description: checks to see if the rtc rejects an invalid input
// input: none
// output: pass or fail
// side effects: none
// */
// int test_rtc_invalid_freq(void){
// 	TEST_HEADER;
// 	int result = PASS;
// 	int retval = set_RTC(5,35);	//valid id invalid freq
// 	if(retval == 0){
// 		result = FAIL;
// 	}
// 	return result;
// }
//
// /*
// description: shows the different rtc rates
// input: none
// output: none
// side effects: changes rtc freq
// */
// int test_rtc_rates(void){
// 	int rate, i;
// 	/*
// 		this for loops starts at 2 and goes through the powers of 2 till 1024
// 		which is the max Hz of the rtc it prints 50 chars at the rate that the
// 		rtc interrupts occur
// 	*/
// 	for(rate = 2; rate <= 1024; rate *= 2){
// 		printf("Waiting 1 sec\n");
// 		delay(1000);
// 		printf("rate: %d\n", rate);
// 		set_RTC(1,rate);
// 		for(i=0;i<32;i++){
// 			read_RTC(1);
// 			putc((char)(i%9 + 1));	//the chars from 1 to 9
// 		}
// 		printf("\n");
// 	}
// 	return test_manual();
// }


/* Checkpoint 3 tests */

/* Tests the strstrip
 *
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: strstrip in lib
 */
int test_strstrip(void){
	TEST_HEADER;
	int result = PASS;

	char s[] = "   helloworld";
	if (strstrip(s) != 3) return FAIL;
	if (strncmp(s, "helloworld", 1000) != 0) return FAIL;

	char ss[] = "helloworld   ";
	if (strstrip(ss) != 3) return FAIL;
	if (strncmp(ss, "helloworld", 1000) != 0) return FAIL;

	char sss[] = "    helloworld   ";
	if (strstrip(sss) != 7) return FAIL;
	if (strncmp(sss, "helloworld", 1000) != 0) return FAIL;

	char ssss[] = "";
	if (strstrip(ssss) != 0) return FAIL;
	if (strncmp(ssss, "", 1000) != 0) return FAIL;

	char sssss[] = " ";
	if (strstrip(sssss) != 1) return FAIL;
	if (strncmp(sssss, "", 1000) != 0) return FAIL;

	char ssssss[] = " a b c d e f ";
	if (strstrip(ssssss) != 2) return FAIL;
	if (strncmp(ssssss, "a b c d e f", 1000) != 0) return FAIL;

	char* ptr = NULL;
	if (strstrip(ptr) != -1) return FAIL;
	// if (strncmp(ptr, "", 1000) != 0) return FAIL;

	return result;
}

/* Tests the strsplit
 *
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: strsplit in lib
 */
int test_strsplit(void){
	TEST_HEADER;

	char s[] = "a  bc d";
	if (strsplit(s) != 3) return FAIL;

	char ss[] = "   a  bc d";
	if (strsplit(ss) != 3) return FAIL;

	char sss[] = "a  bc d   ";
	if (strsplit(sss) != 3) return FAIL;

	char ssss[] = "       ";
	if (strsplit(ssss) != 0) return FAIL;

	return PASS;
}

/* Tests the strgetword
 *
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: strgetword in lib
 */
int test_strgetword(void){
	TEST_HEADER;
	int len = 12;
	char s[] = "  a  bc d   ";

	if (strsplit(s) != 3) return FAIL;

	if (strncmp(strgetword(0, s, len), "a", 1000) != 0) return FAIL;
	if (strncmp(strgetword(1, s, len), "bc", 1000) != 0) return FAIL;
	if (strncmp(strgetword(2, s, len), "d", 1000) != 0) return FAIL;
	if (strgetword(0, NULL, len) != NULL) return FAIL;
	if (strgetword(10, s, len) != NULL) return FAIL;

	return PASS;
}

void test_min_heap(void){
	// int array[] = {7,111,69,420,64,-151,27,2,0};
	// int size =9;
	// int i, retval, popval;
	// printf("before: ");
	// for(i=0;i<size;i++){
	// 	printf("%d, ", array[i]);
	// }
	// create_min_heap(array, size);
	// printf("\n after: " );
	// print_array(array, size);
	// heap_sort(array, size, 0);
	// printf("\n after sort: " );
	// print_array(array, size);
	// printf("\ntesting insert\n");
	// //testing insert
	// int array2[] = {INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX};
	// for(i=0; i<10; i++){
	// 	printf("error code: %d\n", heap_insert(i, array2, 10) );
	// }
	// // print_array(array2, 10);
	// // printf("\n");
	// // create_min_heap(array2, 10);
	// print_array(array2, 10);
	// printf("\ntesting pop and its error\n");
	// while(!is_heap_empty(array2)){
	// 	retval = heap_pop(array2, 10, &popval);
	// 	printf("error code: %d popval: %d \n", retval, popval);
	// }
	// printf("done\n");
	int array[8]={0,1,2,3,4,5,6,7};
	int size =8;
	int retval,i,popval;
	for(i=0;i<25;i++){
		popval=heap_pop(array, size, &retval);	
		heap_pop(array, size, &retval);
		retval = heap_insert(1,array,size);
		heap_pop(array, size, &retval);
		retval = heap_insert(1,array,size);
		retval = heap_insert(0,array,size);

	}
}
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	clear();
	setcursor(0,0);
	//TEST_OUTPUT("test_pagefault", test_pagefault()); // Should throw a page fault
	//TEST_OUTPUT("test_invalid_int", test_invalid_int()); // Should throw a GP fault

	// all are PASS/FAIL, shouldn't fault
	TEST_OUTPUT("test_idt_exceptions", test_idt_exceptions());
	TEST_OUTPUT("test_idt_interrupts", test_idt_interrupts());
	TEST_OUTPUT("test_pagefault_vmem", test_pagefault_vmem());
	TEST_OUTPUT("test_syscall_handler", test_syscall_handler());
	TEST_OUTPUT("test_terminal_write", test_terminal_write());
	TEST_OUTPUT("test_terminal_read", test_terminal_read());
		
	TEST_OUTPUT("test_strstrip", test_strstrip());
	TEST_OUTPUT("test_strsplit", test_strsplit());
	TEST_OUTPUT("test_strgetword", test_strgetword());
	//all are PASS/FAIL, shouldn't fault
	test_min_heap();
	terminal_read(0,"",0);

}
