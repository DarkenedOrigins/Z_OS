/* this is paging.h*/
#ifndef _PAGING_H
#define _PAGING_H

#include "x86_desc.h"
#include "lib.h"

#define SHIFT_RIGHT_22(number) ((number) >> 22)
#define SHIFT_LEFT_22(number) ((number) << 22)
#define SHIFT_RIGHT_12(number) ((number) >> 12)
#define SHIFT_LEFT_12(number) ((number) << 12)

#define IN_KB(number) ((number) << 10)

#define ONE_KILOBYTE 1024
#define FOUR_KILOBYTES (4*ONE_KILOBYTE)
#define VID_ADDR 0xB8
#define KERN_MEM_START 0x00400000
#define PROCESS_MEM_START (2*0x00400000)
#define ENABLE_PSE 0x80
#define ENABLE_SUPERVISOR_RW_PRESENT 0x3
#define ENABLE_SUPERVISOR_RW_NOT_PRESENT 0x2
#define ENABLE_USER_RW_PRESENT 0x7
#define PROCESS_MEM_START_MB (8)
#define PROCESS_PAGE_SIZE_MB (4)
#define FOUR_MB_PAGE_ALIGNMENT_SHIFT (22)
#define PDE_FOR_128MB (32)
#define PDE_FOR_256MB (64)
#define USER_VID_MEM  (0x10000000)


// extern void change_current_process_addr(uint32_t addr);
// extern uint32_t get_current_process_addr(void);
extern void init_paging();
// extern void add_process(int32_t num_processes);
extern void add_process_page(int32_t pid);
extern void flush_tlb();
void init_user_vidmem();
uint8_t* get_vidmem_tty(int32_t tid);
void map_addr_to_addr(void* from_addr, void* to_addr);

extern void init_DMA_page(void * addr);


// extern int add_page(uint32_t virtual_addr,uint32_t physical_addr,uint8_t is_4MB_page);



#endif
