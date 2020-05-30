#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "types.h"
#include "fs.h"
#include "lib.h"
#include "ext-lib.h"
#include "x86_desc.h"
#include "terminal.h"
#include "paging.h"

//defines
#define NUM_FILES			(8)
#define MAGIC_LEN			(4)
#define MAX_PIDS  			(8)
#define MAX_COMMAND_SIZE 	(128)
#define MAX_RTC_RATE 		(1024)
#define BINARY_ENTRY_MASK	(0xFF)
#define BINARY_ENTRY_MASK_1	(24)
#define BINARY_ENTRY_MASK_2	(16)
#define BINARY_ENTRY_MASK_3	(8)
#define MASK_THREE			(3)
#define MASK_TWO			(2)
#define DEFAULT_RTC_RATE	(2)
#define CRASH_RETURN		(256)
#define HEADLESS_TTY		(-1)
#define INHERIT_TTY			(-2)


// PCB - all process-specific information
typedef struct {
	int32_t process_id; // integer identifier
	int32_t parent_id;  // PID of this process' parent
	int32_t parent_esp; // Parent's ESP when this process was executed
	uint32_t rtc_rate;  // This process' chosen virtual RTC rate
	file_t files[NUM_FILES]; // Files open in this process
	uint32_t child_status;   // Set by the child just before returning to execute
	int8_t command[MAX_COMMAND_SIZE]; // The command that spawned this process
	int32_t command_size; // Size of above string
	uint8_t crashed; // TRUE if the program has crashed due to an exception
	int32_t tid; // Where putc and video stuff writes to, i.e: what terminal id
	uint8_t haltable;	// check if we call kill a process
} pcb_t;


// Get the PCB address corresponding to the current kernel stack
pcb_t* get_current_pcb();
// Get the PCB address corresponding to the process id n
pcb_t* get_nth_pcb(uint32_t pid);
// Get the next open fd for a given PCB
int32_t get_available_fd(pcb_t * pcb, uint32_t * ret);

// Syscall handlers
int32_t system_halt (uint8_t status);
int32_t system_execute (const uint8_t* command);
int32_t system_read (int32_t fd, void* buf, int32_t nbytes);
int32_t system_write (int32_t fd, const void* buf, int32_t nbytes);
int32_t system_open (const uint8_t* filename);
int32_t system_close (int32_t fd);
int32_t system_getargs (uint8_t* buf, int32_t nbytes);
int32_t system_vidmap (uint8_t** screen_start);
int32_t system_set_handler (int32_t signum, void* handler_address);
int32_t system_sigreturn (void);
int32_t system_run (const uint8_t* command, int32_t tty);

// Helpers
int32_t system_execute_helper (const uint8_t* command, int32_t tid, uint8_t has_parent, uint8_t haltable);

#endif
