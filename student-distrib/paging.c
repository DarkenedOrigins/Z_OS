// paging.c
/*
 *
 *  Here I have to implement paging. To enable paging I must have a page directory of 1024 entries. Each maps to a page table of 1024 entries
 *  itself.
 *
 * All of the references below are from OS Dev.org
 *
 *
 *  Page Directory

   bits

   31                             11      10        9      8    7     6         5        4        3     2       1       0
   ______________________________________________________________________________________________________________________
   |                                |               |      | S   |    |         |        |       |       |      |       |
   |       Page table               |   Available   |   G  |Page |  0 |  A      |   D    |  W    |  U    |  R   |   P   |
   |       4-kb aligned address     |               |ignore|Size |    |         |Cache   |Write  | User/ | Read |Present|
   |                                |               |      |0 for|    |Accessed |Disabled|Through|Super  | Write|       |
   |________________________________|_______________|______|4kb__|____|_________|________|_______|_______|______|_______|


    The topmost paging structure is the page directory. It is essentially an array of page directory entries that take the following form.
 *
 * The page table address field represents the physical address of the page table that manages the four megabytes at that point.
 *  Please note that it is very important that this address be 4-KiB aligned. This is needed, due to the fact that
 * the last 12 bits of the 32-bit value are overwritten by access bits and such.

    S,  or 'Page Size' stores the page size for that specific entry. If the bit is set, then pages are 4 MiB in size.
             Otherwise, they are 4 KiB. Please note that 4-MiB pages require PSE to be enabled.
    A,  or 'Accessed' is used to discover whether a page has been read or written to.
            If it has, then the bit is set, otherwise, it is not. Note that, this bit will not be cleared by the CPU,
            so that burden falls on the OS (if it needs this bit at all).
    D,  is the 'Cache Disable' bit. If the bit is set, the page will not be cached. Otherwise, it will be.
    W,  the controls 'Write-Through' abilities of the page. If the bit is set, write-through caching is enabled.
        If not, then write-back is enabled instead.
    U, the 'User/Supervisor' bit, controls access to the page based on privilege level.
        If the bit is set, then the page may be accessed by all; if the bit is not set, however, only the supervisor can access it.
        For a page directory entry, the user bit controls access to all the pages referenced by the page directory entry.
        Therefore if you wish to make a page a user page, you must set the user bit in the relevant page directory entry
        as well as the page table entry.
    R, the 'Read/Write' permissions flag. If the bit is set, the page is read/write.
        Otherwise when it is not set, the page is read-only. The WP bit in CR0 determines if this is only applied to userland,
        always giving the kernel write access (the default) or both userland and the kernel (see Intel Manuals 3A 2-20).
    P, or 'Present'. If the bit is set, the page is actually in physical memory at the moment.
        For example, when a page is swapped out, it is not in physical memory and therefore not 'Present'.
        If a page is called, but not present, a page fault will occur, and the OS should handle it.

The remaining bits 9 through 11 are not used by the processor, and are free for the OS to store some of its own accounting information.
In addition, when P is not set, the processor ignores the rest of the entry and you can use all remaining 31 bits for extra information,
 like recording where the page has ended up in swap space.

Setting the S bit makes the page directory entry point directly to a 4-MiB page.
There is no paging table involved in the address translation. Note: With 4-MiB pages, bits 21 through 12 are reserved!
Thus, the physical address must also be 4-MiB-aligned.
 *
 */

/*
 *
 * Page Table
 *
 *
 * bits

   31                               11              9      8    7     6         5        4        3     2       1      0
   _____________________________________________________________________________________________________________________
   |                                |               |      |   |     |         |        |       |       |      |       |
   |       Physical Page            |   Available   |   G  | 0 |  D  |  A      |   C    |  W    |  U    |  R   |   P   |
   |       address                  |               |Global|   |     |         |Cache   |Write  | User/ | Read |Present|
   |                                |               |      |   |Dirty|Accessed |Disabled|Through|Super  | Write|       |
   |________________________________|_______________|______|__ |____ |_________|________|_______|_______|______|_______|


 * In each page table, as it is, there are also 1024 entries. These are called page table entries, and are very similar to
 * page directory entries.

Note: Only explanations of the bits unique to the page table are below.

The first item, is once again, a 4-KiB aligned physical address. Unlike previously, however,
the address is not that of a page table, but instead a 4 KiB block of physical memory that is then
mapped to that location in the page table and directory.

The Global, or 'G' above, flag, if set, prevents the TLB from updating the address in its cache if CR3 is reset.
 Note, that the page global enable bit in CR4 must be set to enable this feature.

If the Dirty flag ('D') is set, then the page has been written to.
This flag is not updated by the CPU, and once set will not unset itself.

The 'C' bit is 'D' bit above.
 */

/*  Enabling Paging
 * Enabling paging is actually very simple.
 * All that is needed is to load CR3 with the address of the page directory and to set the paging (PG) and protection (PE) bits of CR0.
 * Note: setting the paging flag when the protection flag is clear causes a general-protection exception.

 mov eax, page_directory
 mov cr3, eax

 mov eax, cr0
 or eax, 0x80000001
 mov cr0, eax

If you want to set pages as read-only for both userspace and supervisor, replace 0x80000001 above with 0x80010001,
 which also sets the WP bit.


To enable PSE (4 MiB pages) the following code is required.
 *
 *
 *   mov eax, cr4
 *   or eax, 0x00000010
 *   mov cr4, eax
 *
 *
 */

#include "paging.h"
uint32_t page_directory[ONE_KILOBYTE]__attribute__((aligned(FOUR_KILOBYTES))); // The actual page directory

uint32_t first_page_table[ONE_KILOBYTE]__attribute__((aligned(FOUR_KILOBYTES))); //page table for 0 to 4 MB

uint32_t vid_mem_page_table[ONE_KILOBYTE]__attribute__((aligned(FOUR_KILOBYTES))); //page table for vid mrm


//int some_variable __attribute__((aligned (BYTES_TO_ALIGN_TO)));

/*  init_paging
	description: Initializes paging
	inputs: none
	output: none
	side effect: writes to the cr0,cr3,cr4 registers.
*/

extern void init_paging(){

//set all page directories and page tables

    uint32_t i = 0;
    for(i=0;i<ONE_KILOBYTE;i++){
        page_directory[i]=0;                                                            //setting all page directory pointers to NULL
        first_page_table[i]=(i*FOUR_KILOBYTES)|ENABLE_SUPERVISOR_RW_NOT_PRESENT;        //Putting physical page address and setting bits
    }																					// for rw present
                                                                                        //video memory reserve here
    first_page_table[VID_ADDR]|=ENABLE_SUPERVISOR_RW_PRESENT;

    for (i = 1; i < MAX_TERMINALS+1; i++)													//for vid mem save
        first_page_table[VID_ADDR+i]|=ENABLE_USER_RW_PRESENT;


    page_directory[0]=(uint32_t)first_page_table|ENABLE_SUPERVISOR_RW_PRESENT;          //sets the first 4 MB to the first 4kb page tables
    page_directory[1]=KERN_MEM_START|ENABLE_PSE|ENABLE_SUPERVISOR_RW_PRESENT;           // should initialize a 4MB page this is page 1024
                                                                                        // 0xFFFFF 083 128 will make this a 4MB page
                                                                                        //then 3 will make this read write and present
                                                                                        // first 20 bits or 5 bytes are page address 1024
                                                                                        // 1024 in 20 bits or 5 bytes and then 083
                                                                                         // so x00400000 + x80 +x3

/*
The code below  does the following:
                1. Puts the address of the page directory into the cr3 register
                2. It then enables Page Size Extension by taking everything that
                    was present in cr4 then taking the OR ith the 32 bit
                    number that sets the bit for PSE. Thus enabling PSE.

                    orl $0x00000010,%%eax

                    This enables PSE by setting bit 5 in cr4 to 1.

                3.  It then enables Paging by setting the 31st bit in cr0 to 1.

                    so    orl $0x80000000,%%eax sets the 31st bit of cr0 to enable paging.



*/
 asm volatile   (

                "movl %0,%%eax;"
                "movl %%eax,%%cr3;"

                "movl %%cr4,%%eax;"
                "orl $0x00000010,%%eax;"
                "movl  %%eax,%%cr4;"

                "movl %%cr0,%%eax;"
                "orl $0x80000000,%%eax;"
                "movl %%eax,%%cr0;"

                :                       // Output Operands
                : "r"(page_directory)   // Input Operands This is %0
                : "eax"                 // Clobbered Registers *
);
    return;
}



extern void init_DMA_page(void * addr)
{
    page_directory[(uint32_t)addr >> FOUR_MB_PAGE_ALIGNMENT_SHIFT ]=(uint32_t)addr|ENABLE_PSE|ENABLE_USER_RW_PRESENT;

    flush_tlb();
}

// extern void change_current_process_addr(uint32_t addr){
//     page_directory[PDE_FOR_128MB] = addr;
//     flush_tlb();
//     return;
// }

// extern uint32_t get_current_process_addr(void){
//     return page_directory[PDE_FOR_128MB];
// }
/* add_process_page
 * description: redirect the page corresponding to the common executable virtual address to point to the first instruction of a new process
 * input:
 * 	pid - PID of the process we are switching to
 * output:
 *	None
 * side effects: Changes the page directory and flushes tlb
*/
extern void add_process_page(int32_t pid){
    
    int32_t addr=PROCESS_MEM_START_MB+(PROCESS_PAGE_SIZE_MB*(pid+1)); //address for the process based on pid number and then shift to match pde format
    addr=addr<<FOUR_MB_PAGE_ALIGNMENT_SHIFT;
    page_directory[PDE_FOR_128MB]=addr|ENABLE_PSE|ENABLE_USER_RW_PRESENT; //32 for 128MB, as it's mapping 4 MB per PDE
    //Flush TLB <---- greatest comment ever
    flush_tlb();
    return;
}

/* init_user_vidmem
 * description: initializes the page table for user level vid mem.
 * input:
 * 	    none
 * output:
 *	    None
 * side effects: Changes the page directory and flushes tlb and modifies a page table and an entry
*/
void init_user_vidmem() {
	page_directory[0]=(uint32_t)first_page_table|ENABLE_USER_RW_PRESENT;    //Set PD0 to USER to enable USER to enable Vidmap to write
	int i;
	for (i = -1; i < MAX_TERMINALS; i++) {
		uint32_t base = (uint32_t)get_vidmem_tty(i);                        // Get Pointer for vidmem for index
    	first_page_table[SHIFT_RIGHT_12(base)]|=ENABLE_USER_RW_PRESENT;     // Shift right by 12 for getting the index in the page table
                                                                            // This only works for vid_mem manipulation on our OS.
	}
	flush_tlb();
}

/* get_vidmem_tty
 * description: returns a pointer to the correct video memory for the terminal
 * input:
 * 	    tid: integer to determine which terminal
 * output:
 *	       base: pointer to video memory
 * side effects: none
*/
uint8_t* get_vidmem_tty(int32_t tid){
	if (tid == -1)
		return (uint8_t*) VIDEO-FOUR_KILOBYTES;
    uint32_t base=(VIDEO+FOUR_KILOBYTES)+(tid*FOUR_KILOBYTES);          //Gets pointer for vidmem by calculating the address we gave to it based on tid
    if(base<VIDEO||base>=IN_MB(4))
        return NULL;
    return (uint8_t*)base;
}

/* map_addr_to_addr
 * description: maps pages to specified pages only for video memory
 * input:
 * 	    from_addr:  address to map from
 *      to_addr  :  address to map to page of 4kb
 * output:
 *	    None
 * side effects: Changes the page table and flushes tlb and modifies a page table and an entry
*/
void map_addr_to_addr(void* from_addr, void* to_addr) {
	uint32_t base = SHIFT_RIGHT_12((uint32_t)from_addr);
	first_page_table[base] = (uint32_t)to_addr|ENABLE_USER_RW_PRESENT;      //Map from Page to to Page
	flush_tlb();
}


// to be added later for malloc

// extern int add_page(uint32_t virtual_addr,uint32_t physical_addr,uint8_t is_4MB_page){
//     if(is_4MB_page!=0||is_4MB_page!=1){
//         return -1;
//     }
//     if(is_4MB_page){
//         //4MB 10 bits 22 bits not need
//         uint32_t pd_index=SHIFT_RIGHT_22(virtual_addr);
//         page_directory[pd_index]=;

//     }
//     else{
//         //4kb page

//     }
//     flush_tlb();
//     return 0;
// }
/* flush_tlb()
 * description: flushes the Translation Lookaside Buffer by reloading cr3 with itself
 * input:
 * 	None
 * output:
 *	None
 * side effects: Clears the TLB
*/
extern void flush_tlb(){
        asm volatile (
                "movl %%cr3,%%eax;"
                "movl %%eax,%%cr3;"

                :                       // Output Operands
                : //"r"(page_directory)   // Input Operands This is %0
                : "eax"                 // Clobbered Registers *
    );
    return;
}
