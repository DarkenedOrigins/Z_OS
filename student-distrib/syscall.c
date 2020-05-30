#include "syscall.h"

// Heap of available PIDs
int pids[MAX_PIDS] = {0, 1, 2, 3, 4, 5, 6, 7};

// Count of the current number of running processes
int32_t open_processes = 0;

/* get_current_pcb
 * description: Gets the PCB corresponding to the current kernel stack
 * input:
 * 	None (Does use esp)
 * output:
 *	Pointer to PCB on current kernel stack
 * side effects: Dangerous if called from a user stack or the initial kernel stack
 */
pcb_t* get_current_pcb()
{
    #define ESP_PCB_MASK    (0xFFFFE000)
	//get address in esp
    register uint32_t sp asm("sp");
	//get rid of the lower 13 bits since pcb 8kb aligned
    return (pcb_t*)(sp & ESP_PCB_MASK);
}

/* get_nth_pcb
 * description: Gets the PCB corresponding to a given PID
 * input:
 * 	pid - Some valid PID
 * output:
 *	Pointer to PCB on current kernel stack
 * side effects: Possibly dangerous if called with large PIDs, as we do not know the max allowable
 */
pcb_t* get_nth_pcb(uint32_t pid)
{
    if (pid > MAX_PIDS) return NULL;
    // 8 MB - n(8 KB) where n = pid+1
    return (pcb_t*)((FOUR_MBYTES*2)-(FOUR_KBYTES*2)*(pid+1));
}

/* get_available_fd
 * description: Gets the next available fd in the given pcb
 * input:
 * 	pcb - pointer to some pcb
 *  ret - pointer in which to place the found fd
 * output:
 *	Indicates success or failure
 * side effects: Fills whatever is at ret pointer
 */
int32_t get_available_fd(pcb_t * pcb, uint32_t * ret)
{
    if (ret == NULL || pcb == NULL) return -1;
    uint32_t i;
	//loop over indexs in fd array to find open fd index
    for (i = 0; i < NUM_FILES; i++) {
        if (pcb->files[i].flags == 0) {
            break;
        }
        // if last index has been checked to be closed return neg 1
        if (i == NUM_FILES - 1) return -1;
    }
	//insert i "index" into ret
    *ret = i;
    return 0;
}


/* stdio_init
 * description: Initialize stdin and stdout for a new process
 * input:
 * 	pcb - pointer to some pcb
 * output:
 *	Indicates success
 * side effects: Initialize fd 0 and 1 on the given PCB
 */
#define STDIN_FD    (0)
#define STDOUT_FD   (1)
int32_t stdio_init(pcb_t* pcb) {
    file_t stdin, stdout;

    // Values for STDIN - only want to be able to read
    stdin.f_pos = 0;
    stdin.flags = 1;
    stdin.inode = 0;
    stdin.file_ops.close = NULL;
    stdin.file_ops.open = NULL;
    stdin.file_ops.read = terminal_read;
    stdin.file_ops.write = NULL;


    // Values for STDOUT - only want to be able to write
    stdout.f_pos = 0;
    stdout.flags = 1;
    stdout.inode = 0;
    stdout.file_ops.close = NULL;
    stdout.file_ops.open = NULL;
    stdout.file_ops.read = NULL;
    stdout.file_ops.write = terminal_write;

    // Set the files in the PCB
    pcb->files[STDIN_FD] = stdin;
    pcb->files[STDOUT_FD] = stdout;
    return 0;
}

/* system_execute
 * description: Execute a given process
 * input:
 * 	command - name of the binary to be executed
 * output:
 *	Indicates success or exit value of the given process
 * side effects: Creates a new kernel and user stack, a new PCB, increases the number of processes, and takes up a PID
 */
int32_t system_execute (const uint8_t* command) {
	return system_execute_helper(command, INHERIT_TTY, TRUE, TRUE);
}


/* system_execute_helper
 * description: Execute a given process
 * input:
 * 	command - name of the binary to be executed
 *  tid - terminal id associated with the process. if invalid id, it gets
 *		  inherited from the parent process
 * output:
 *	Indicates success or exit value of the given process
 * side effects: Creates a new kernel and user stack, a new PCB, increases the number of processes, and takes up a PID
 */
int32_t system_execute_helper (const uint8_t* command, int32_t tid, uint8_t has_parent, uint8_t haltable){
    // Don't want interrupts until we hand over control
    cli();

    // Parse command, remove extra whitespace and split args
    // command is a const so first, make a copy
    int8_t command_cpy[MAX_COMMAND_SIZE];
    strcpy(command_cpy, (int8_t*)command);
    unsigned int command_size = strlen((int8_t*)command_cpy);
    if (command_size == 0) return -1;
    strstrip((int8_t*)command_cpy);
    int words = strsplit(command_cpy);
    if (words == -1 || words == 0) return -1;

    // Check if valid file
    int8_t* filename = strgetword(0, command_cpy, command_size);
    if (filename == NULL) return -1;
    dentry_t entry;
    if (read_dentry_by_name((uint8_t*)filename, &entry) == -1) return -1;   // Make sure dentry exists
    if (entry.filetype != DENTRY_TYPE_FILE) return -1;                      // Make sure dentry is a file
    inode_t* inode_ptr = get_inode_ptr(entry.inode_num);                    // Get the inode of the binary

    // To enable shell history (up/down) we need to know if we're in a shell
    in_shell = (strncmp("shell", filename, strlen("shell")) == 0) ? TRUE : FALSE;

    // Make sure the binary is an ok size
    if (inode_ptr->length <= MAGIC_LEN || inode_ptr->length >= FOUR_MBYTES - PROGRAM_OFFSET) return -1;

    // Check Magic Numbers to make sure the file is an executable
    char magic_nums[MAGIC_LEN] = {0x7f, 0x45, 0x4c, 0x46};
    int8_t buf[MAGIC_LEN];
    if (read_data(entry.inode_num, 0, buf, MAGIC_LEN) != MAGIC_LEN) return -1;
    if (strncmp(magic_nums, buf, 4) != 0) return -1;    // Not magic number it's literally for 4 magic numbers, kinda meta if you ask me

    // Get an available PID from the heap
    int pid;
    if (heap_pop(pids, MAX_PIDS, &pid) != 0) return -2;
    if (pid < 0 || pid >= MAX_PIDS) return -2;
    // printf("Starting process %d\n", pid);

    // Set up paging (Redirect page for executable to point to our binary's new physical location)
    add_process_page(pid);

    // Read data to get start point and user stack esp
    int8_t start_pt[MAGIC_LEN];
    read_data(entry.inode_num, BINARY_ENTRY_MASK_1, start_pt, MAGIC_LEN);

    int32_t mask1 = ((start_pt[MASK_THREE] & BINARY_ENTRY_MASK) << BINARY_ENTRY_MASK_1);
    int32_t mask2 = ((start_pt[MASK_TWO] & BINARY_ENTRY_MASK) << BINARY_ENTRY_MASK_2);
    int32_t mask3 = ((start_pt[1] & BINARY_ENTRY_MASK) << BINARY_ENTRY_MASK_3);
    int32_t mask4 = ((start_pt[0] & BINARY_ENTRY_MASK) << NULL);

    // Find our binary start address and new ESP
    int32_t start_addr = mask1 | mask2 | mask3 | mask4;
    int32_t user_esp = (FOUR_MBYTES)+(PROGRAM_PAGE)-sizeof(int32_t);

    // Copy program image
    int8_t* program_image_start = (int8_t*)(PROGRAM_PAGE | PROGRAM_OFFSET);
    read_data(entry.inode_num, 0, program_image_start, inode_ptr->length);

    // Point of no return - we are for sure going to execute, so increment running processes
    open_processes++;

    // Create PCB for our new PID
	pcb_t* curr_pcb = get_current_pcb();
    pcb_t* pcb = get_nth_pcb(pid);
    pcb->process_id = pid;
    pcb->rtc_rate = DEFAULT_RTC_RATE;
    pcb->parent_id = (has_parent == FALSE) ? pid : curr_pcb->process_id;
    pcb->crashed = FALSE;
	if (tid >= 0 && tid < MAX_TERMINALS) {
		pcb->tid = tid;
		set_vidmem(pcb->tid);
	} else if (tid == INHERIT_TTY) {
		pcb->tid = curr_pcb->tid;
	} else if (tid == HEADLESS_TTY) {
		pcb->tid = HEADLESS_TTY;
		set_vidmem(pcb->tid);
	}
	pcb->haltable = haltable;

    // Copy filtered command to pcb
    // i.e: ...cat.some...stuffs....here...
    //      becomes: cat.some.stuff.here
    //      (dots representing spaces here)
    int start_idx = 0;
    int8_t* word;
    int i;
    for (i = 0; i < words; i++) {
        word = strgetword(i, command_cpy, command_size);
        strcpy(&(pcb->command)[start_idx], word);
        start_idx += strlen(word);
        if (i != words-1) {
            pcb->command[start_idx] = ' ';
            start_idx++;
        }
    }
    pcb->command_size = start_idx;

    // Initialize all files to nonpresent status
    for (i = 0; i < NUM_FILES; i++) {
        pcb->files[i].flags = 0;
    }

    // Initialize stdin and stdout (fd's 0 and 1)
    stdio_init(pcb);

    // Prepare for Context Switch
    // Save our current stack pointer
    uint32_t sp;
    asm volatile( "mov %%esp, %0" : "=r" ( sp ));
    pcb->parent_esp = sp;

    // Set the kernel-level esp and stack segment
    tss.esp0 = (FOUR_MBYTES*2)-(FOUR_KBYTES*2)*(pid)-sizeof(int32_t);
    tss.ss0 = KERNEL_DS;

    // Push IRET args, then IRET into program
    asm volatile ( "pushl %0;"
                   "pushl %1;"
                   "pushf;"
                   "popl %%eax;"
                   "orl $0x00000200,%%eax;" // Set interrupts
                   "pushl %%eax;"
                   "pushl %2;"
                   "pushl %3;"
                   "iret;"
                   :                       // Output Operands
                   : "r"(USER_DS), "r"(user_esp), "r"(USER_CS), "r"(start_addr) // Input
                   : "eax"                 // Clobbered Registers *
    );

    // Label to jump back to from system_halt
    asm volatile ("system_execute_end:;");

    // To enable shell history (up/down) we need to know if we're in a shell
    in_shell = (strncmp("shell", get_current_pcb()->command, 5) == 0) ? TRUE : FALSE;

    // Return the status of the executed process
    return get_current_pcb()->child_status;
}

/* system_halt
 * description: Halt a process
 * input:
 * 	status - exit status of process
 * output:
 *	None - jumps before return
 * side effects: Restores control to the parent of the process
*/
int32_t system_halt(uint8_t status) {
	cli();
  	int i; // Iterator

	// process ending so reduce number of open processes
    open_processes--;

    // get PCB for halting process
  	pcb_t* pcb = get_current_pcb();

    // If the program crashes, mark the exit value as such
    // If not, take whatever was passed
    get_nth_pcb(pcb->parent_id)->child_status =(pcb->crashed) ? CRASH_RETURN : status;

    // Clear the files for the next process
  	for(i = 0; i < NUM_FILES; i++){
      // set fd array flag to zero
  		pcb->files[i].flags = 0;
  	}

    // put PID back into min heap
    heap_insert(pcb->process_id, pids, MAX_PIDS);

    // Return parent paging
    add_process_page(pcb->parent_id);

    // This esp0 is just in case some weird stuff happens between here and the execute ending assembly linkage
    tss.esp0 = pcb->parent_esp;
    tss.ss0 = KERNEL_DS;

	// Execute again if not haltable
	if (!pcb->haltable)
		system_execute_helper((uint8_t*)pcb->command, pcb->tid, pcb->process_id!=pcb->parent_id, pcb->haltable);

    // Restore the stack pointer to what it was when this process gained control
    asm volatile (
        "movl %0, %%esp"
        :
        :"r"(pcb->parent_esp)
    );
    // Jump to the end of execute
  	asm volatile ("jmp system_execute_end;");
    // sadly this return will never be reached
    return 0;
}

/* system_read
 * description: Read from a file
 * input:
 * 	fd - file descriptor of file to read from
 *  buf - buffer we write data to
 *  nbytes - number of bytes to to write
 * output:
 *	Indicates success of read
 * side effects: May increment given file position if applicable
 */
int32_t system_read (int32_t fd, void* buf, int32_t nbytes) {

    // Make sure our fd is valid
    if (fd < 0 || fd >= NUM_FILES) return -1;

    // Get the PCB on the current stack
    pcb_t * pcb = get_current_pcb();

    int32_t ret; // Return value

    // Make sure our file is open and has a valid read function
    if (pcb->files[fd].file_ops.read == NULL || pcb->files[fd].flags == 0) return -1;

    // Execute the read function and return its value
    ret = pcb->files[fd].file_ops.read(fd, (int8_t*)buf, nbytes);
    return ret;
}

/* system_write
 * description: Write to a file
 * input:
 * 	fd - file descriptor of file to write to
 *  buf - buffer we read data from
 *  nbytes - number of bytes to to read
 * output:
 *	Indicates success of write
 * side effects: Will affect given file
 */
int32_t system_write (int32_t fd, const void* buf, int32_t nbytes) {
    // Make sure our fd is valid
    if (fd < 0 || fd >= NUM_FILES) return -1;

    // Get the PCB on the current stack
    pcb_t * pcb = get_current_pcb();
    int32_t ret;

    // Make sure our file is open and has a valid write function
    if (pcb->files[fd].file_ops.write == NULL || pcb->files[fd].flags == 0) return -1;

    // Execute the write function and return its value
    ret = pcb->files[fd].file_ops.write(fd, (int8_t*)buf, nbytes);
    return ret;
}

/* system_open
 * description: Open a file
 * input:
 * 	filename - name of file to open
 * output:
 *	fd of new file, or -1 on failure
 * side effects: Will populate a new file object in the PCB
 */
int32_t system_open (const uint8_t* filename) {
    dentry_t dentry;
    int32_t ret;

    const int8_t* sb16_name = (int8_t*)"sb16";
    // Find the dentry with the given filename (and make sure it exists)
    if (read_dentry_by_name(filename, &dentry) == -1) return -1;

    // Run the corresponding open function and save the return value
    switch(dentry.filetype) {
        case DENTRY_TYPE_DIRECTORY:
            ret = dir_open(filename);
            break;
        case DENTRY_TYPE_FILE:
            if (!strncmp((int8_t*)filename, sb16_name, strlen(sb16_name))) {
                ret = sb16_open(filename);
            } else {
                ret = file_open(filename);
            }
            break;
        case DENTRY_TYPE_RTC:
            ret =  rtc_open(filename);
            break;
        default:
            break;
    };
    // Return passed return value
    return ret;
}

/* system_close
 * description: Close a file
 * input:
 * 	fd - file descriptor of file to close
 * output:
 *	Indicates success of close
 * side effects: Frees the given fd
 */
int32_t system_close (int32_t fd) {
    // Make sure our fd is valid
    if (fd < 0 || fd >= NUM_FILES) return -1;

    // Get the PCB on the current stack
    pcb_t * pcb = get_current_pcb();
    int32_t ret;

    // Make sure our file is open and has a valid close function
    if (pcb->files[fd].file_ops.close == NULL || pcb->files[fd].flags == 0) return -1;

    // Execute the close function and return its value
    ret = pcb->files[fd].file_ops.close(fd);

    // Double check to make sure flags is set to 0
    pcb->files[fd].flags = 0;
    return ret;
}


/* system_getargs
 * description: get arguments from command that created this process
 * input:
 * 	    buf - user-level char buffer
 *      nbytes - suze of buf
 * output:
 *	    success:0, -1 on error
 * side effects: fills buf
 */
int32_t system_getargs (uint8_t* buf, int32_t nbytes) {
    if (buf == NULL || nbytes < 0) return -1;
    int i;
    pcb_t* pcb = get_current_pcb();
    for (i = 0; (i < pcb->command_size) && (pcb->command[i] != ' '); i++);
    strncpy((int8_t*)buf, &(pcb->command)[i+1], (uint32_t)nbytes);
    if (pcb->command_size - i-1 > 0)
        return 0;
    return -1;
}

/* system_vidmap
 * description: map a user level 4kb page to vidmem and then display
 * input:
 * 	    screen_start:   double pointer to start of screen data
 * output:
 *	    success:0, -1 on error
 * side effects: writes to vid mem
 */
int32_t system_vidmap (uint8_t** screen_start) {
    if((uint32_t)screen_start < IN_MB(8) || screen_start == NULL)
        return -1;                              // returns -1 if address is in the first 8MB of mem. (or if a bad pointer)

    // If valid, get the current process' vidmemory pointer and set screen start
    *screen_start =  ttys[get_current_pcb()->tid].vidmem_ptr;
    return 0;
}

/* system_set_handler
 * description: Changes the action when a signal is recieved
 * input:
 * 	    sig_num: Signal number to change
 *      handler_address: User-level function pointer to run when signal is recieved
 * output:
 *	    success:0, -1 on error
 * side effects: None
 */
int32_t system_set_handler (int32_t signum, void* handler_address) {

    return -1; // For CP4 and no signal support
}

/* system_sigreturn
 * description: Return from a signal
 * input:
 * 	    None
 * output:
 *	    success:0, -1 on error
 * side effects: Copies hardware context from stack into processor
 */
int32_t system_sigreturn (void) {

    return -1;// For CP4 and no signal support
}

int32_t system_run (const uint8_t* command, int32_t tty) {
	if (tty < -1 || tty >= MAX_TERMINALS) return -1;
	schedule_job(command, NULL, tty, TRUE);
	return 0;
}
