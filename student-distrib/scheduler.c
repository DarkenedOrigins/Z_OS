#include "scheduler.h"
#include "syscall.h"

/*
 *	Struct and Global Variables
 *
 * 	running_t	:	This struct encapsulated information about the process in order to be able to switch between jobs, thus is holds stack and process information
 *
 * 	pending_t	:	This struct encapsulated information about the pending jobs in order to be able to execute them later
 *
 * 	running_jobs:	Array of jobs that are currently running, each points to an individual running_t
 *
 *  pending_jobs:	Array of jobs that are scheduled to run, each points to an individual pending_t

 */

typedef struct {
	uint32_t pid;
	uint32_t ebp;
    uint32_t esp;
    uint32_t esp0;
    uint32_t ss0;
	bool in_use;
	int32_t* return_status;
} running_t;

typedef struct{
    uint8_t command[MAX_COMMAND_SIZE];
	int32_t* return_status;
	int32_t tid;
	bool haltable;
	bool in_use;
} pending_t;

running_t running_jobs[MAX_PIDS];
pending_t pending_jobs[MAX_PIDS];

running_t* curr_running;
int running_size; 	//Amount of Running and Pending Jobs
int pending_size;

// Helper Functions

/*  pending_job_helper
	description: finds next pending job that is free or in use
	inputs: used - tells the function whether to find the first
				   job that is in_use or not
	output: the desired pending job
	side effect: none
*/
pending_t* pending_job_helper(bool used) {
	int i;
	for (i = 0; i < MAX_PIDS; i++) {
		if (pending_jobs[i].in_use == used){
			return &pending_jobs[i];
		}
	}
	return (pending_t*) NULL;
}

/*  get_next_pending_job
	description: finds next pending job that needs to be executed
	inputs: none
	output: next pending job to be executed
	side effect: none
*/
pending_t* get_next_pending_job(void) {
	return pending_job_helper(TRUE);
}

/*  get_available_pending_job
	description: finds next pending job that is free to overwrite
	inputs: none
	output: next job that can be overwritten
	side effect: none
*/
pending_t* get_available_pending_job(void) {
	return pending_job_helper(FALSE);
}

/*  get_next_running_job
	description: read the function name dammit
	inputs: none
	output: next job to run... duh
	side effect: none
*/
running_t* get_next_running_job(void) {
	static int runningIdx = NO_JOBS;
	int i;
	for(i = 0; i < MAX_PIDS; i++) {
		runningIdx++;
		runningIdx %= MAX_PIDS;
		if(running_jobs[runningIdx].in_use == TRUE) {
			return &running_jobs[runningIdx];
		}
	}
	return (running_t*) NULL;
}

/*  get_available_running_job
	description: gets next free job, i.e: we can overwrite it with a new job
	inputs: none
	output: job we can overwrite
	side effect: none
*/
running_t* get_available_running_job(void) {
	int i;
	for (i = 0; i < MAX_PIDS; i++) {
		if (running_jobs[i].in_use == FALSE){
			return &running_jobs[i];
		}
	}
	return (running_t*) NULL;
}


// Core Functions

/*  init_scheduling
	description: initializes scheduling by setting all global variables to zero as no jobs have been scheduled yet
	inputs: none
	output: none
	side effect: none
*/
void init_scheduling(void) {
	curr_running = NULL;
	running_size = 0;
	pending_size = 0;
	uint32_t i;
	for (i = 0; i < MAX_PIDS; i++) {
		running_jobs[i].in_use = FALSE;
		pending_jobs[i].in_use = FALSE;
	}
}

/*  execute_pending_job
	description: takes a job off of pending_jobs and tries to execute it.
	inputs: none
	output: 0 if success, -1 on error
	side effect: changs both queues
*/
int32_t execute_pending_job(void) {
	// Get pendng job and get a free running job
	pending_t* to_execute = get_next_pending_job();
	running_t* next_running = get_available_running_job();
	if (to_execute == NULL) return -1;
	if (next_running == NULL) return -1;
	// If possible execute the pending job
	curr_running = next_running;
	running_size++;
	pending_size--;
	to_execute->in_use = FALSE;
	curr_running->return_status = to_execute->return_status;
	curr_running->in_use = TRUE;
	int32_t retval = system_execute_helper(to_execute->command, to_execute->tid, FALSE, to_execute->haltable);
	// Return the exit status code to the process that scheduled this job
	if (curr_running->return_status != NULL)
		*(curr_running->return_status) = retval;
	// Remove from running
	curr_running->in_use = FALSE;
	running_size--;

	// Context switch to next running
	curr_running = get_next_running_job();
	add_process_page(curr_running->pid);
	set_vidmem(get_nth_pcb(curr_running->pid)->tid);
	tss.esp0 = curr_running->esp0;
	tss.ss0 = curr_running->ss0;
	asm volatile(
		"movl %1, %%ebp;"
		"movl %0, %%esp;"
		"sti;"
		"leave;"
		"ret;"
		:
		:"r"(curr_running->esp), "r"(curr_running->ebp)
	);

	return 0;
}

/*  schedulerHandler
	description: The function that executes on receiving a PIT interrupt, it performs a context switch and executes the next process until an interrupt
	inputs: none
	output: none
	side effect: switches between processes which involves manipulating the stack and paging
*/

void schedulerHandler(void) {
	/* Assume that the current running job is at index curr_running */
	/* Assume that sys_execute for root shells never returns, i.e: shell can't exit */

	// if no running and no pending return.
	if (pending_size == 0 && running_size == 0) return;
	// if curr_running == NULL: nothing ever ran,
	cli();
	send_eoi(PIT_PIC_LINE);
	if (curr_running == NULL) {
		// see if pending, sys_execute pending
		execute_pending_job();
		return;
	} else {
		// Save job data at curr_running, assume we're in a process.
		pcb_t* pcb = get_current_pcb();
		get_vidmem(pcb->tid);
		curr_running->pid = pcb->process_id;
		curr_running->in_use = TRUE;
		curr_running->esp0 = tss.esp0;
		curr_running->ss0 = tss.ss0;
		// save esp and ebp into the structs
		asm volatile(
			"movl %%esp, %0;"
			"movl %%ebp, %1;"
			:"=rm"(curr_running->esp),"=rm"(curr_running->ebp)
		);
		// if there are pending task and we can schedule them do it
		if (running_size < MAX_PIDS && pending_size > 0) {
			// Try to execute a pending job
			execute_pending_job();
			return;
		} else {
			// Context switch to next running
			curr_running = get_next_running_job();
			add_process_page(curr_running->pid);
			set_vidmem(get_nth_pcb(curr_running->pid)->tid);
			tss.esp0 = curr_running->esp0;
			tss.ss0 = curr_running->ss0;
			asm volatile(
				"movl %1, %%ebp;"
				"movl %0, %%esp;"
				"sti;"
				"leave;"
				"ret;"
				:
				:"r"(curr_running->esp), "r"(curr_running->ebp)
			);
		}
	}
}


/*  schedule_job
	description: adds a job to pending jobs to be executed later
	inputs: arguments that are taken by system_execute_helper, go see that
	output: 0 if success, -1 on error
	side effect: changes pending_jobs
*/
int32_t schedule_job(const uint8_t* command, int32_t* retval, int32_t tid, uint8_t haltable) {
	pending_t* to_schedule = get_available_pending_job();
	if (to_schedule == NULL || tid < 0 || tid >= MAX_TERMINALS) return -1;
	// Add to pending_jobs to be executed later
	strncpy((int8_t*)to_schedule->command, (int8_t*)command, MAX_COMMAND_SIZE);
	to_schedule->in_use = TRUE;
	to_schedule->return_status = retval;
	to_schedule->tid = tid;
	to_schedule->haltable = haltable;
	pending_size++;
	return 0;
}
