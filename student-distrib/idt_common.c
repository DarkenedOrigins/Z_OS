#include "idt_common.h"

// Holds the interrupt handlers corresponding to each IDT entry
static irq_desc IRQhandlers[NUM_VEC];

static handler_t serviceRoutines[] = {
    irq00, irq01, irq02, irq03, irq04, irq05, irq06, irq07, irq08, irq09,
    irq0A, irq0B, irq0C, irq0D, irq0E, irq0F, irq10, irq11, irq12, irq13,
    irq14, irq15, irq16, irq17, irq18, irq19, irq1A, irq1B, irq1C, irq1D,
    irq1E, irq1F, irq20, irq21, irq22, irq23, irq24, irq25, irq26, irq27,
    irq28, irq29, irq2A, irq2B, irq2C, irq2D, irq2E};

/*
    setupInterrupts
	description: Initialize the premade interrupt service routines, and set the
               Interrupt Handlers that need to be present at boot
	inputs: none
	output: none
	side effect: changes the IDT and IRQhandlers array
*/
void setupIDT(void)
{
    int i; // Iterator

    // First initialize the IRQhandler array and mark all IDT entries as
    // not present by default
    for (i=0; i < NUM_VEC; i++) {
        setIRQhandler(i, NULL);
    }

    // Set the first 32 entries to th IDT as trap gates to their corresponding
    // Exception routines
    for (i=0; i < IRQ_OFFSET; i++) {
        setTrap(i, serviceRoutines[i]);
    }

    // Set the next 15 entries to the IDT as int gates to their interrupt routines
    for (i= 0; i < NUM_PIC_VEC; i++) {
        setInt(IRQ_OFFSET + i, serviceRoutines[IRQ_OFFSET + i]);
    }

    // Set the syscall gate to be a trap gate callable from user space
    setTrap(SYSCALL_GATE, irq80);
    idt[SYSCALL_GATE].dpl = USER_PERMISSION;

    // For the PIT, RTC, and keyboard, reference their specific handlers
	setIRQhandler(PIT_IRQ, &schedulerHandler);
    setIRQhandler(RTC_IRQ, &RTCHandler);
    setIRQhandler(KEY_IRQ, &keyboardHandler);
}


/*
	setupExceptions()
	description: Sets up exception handlers for the first 32 IDT entreis,
				 defined by Intel to be reserved for exceptions
	inputs: None
	output:	None
	side effect: Edits the first 32 entries in the IDT
*/
void setTrap(int num, handler_t routine)
{
    /* For reference: use IA32 Manual Vol 3 Page 5-14 */
  	/* general exception handler  */
  	idt_desc_t ge;
  	ge.seg_selector = KERNEL_CS; // Execute exceptions in Kernel Code Segment
  	ge.reserved4 = 0; // All set to 0 (reference manual)
  	// For 32-bit trap gates, type is 0b1111
  	ge.reserved3 = 1;
  	ge.reserved2 = 1;
  	ge.reserved1 = 1;
  	ge.size = 1;
  	ge.reserved0 = 0; // Always 0 for exceptions
  	ge.dpl = KERNEL_PERMISSION; // Don't want users to be able to generate exceptions so
  	// set the lowest possible privilege level
  	ge.present = 1; // Mark our exception as present

  	// Set the handler to the one passed
  	SET_IDT_ENTRY(ge, routine);
  	idt[num] = ge; // put our entry in the IDT
}

/*
    setInt
	description: Set the entry num + IRQ_OFF in the idt to be a valid selector,
                 with interrupt service routine 'routine'
	inputs: vec: Vector into the IDT (with the offset of 0x20 to skip exceptions)
            routine: ISR to be inserted
	output: none
	side effect: changes the IDT
*/
void setInt(int num, handler_t routine)
{
   /* For reference: use IA32 Manual Vol 3 Page 5-14 */
   /* general interrupt handler */
   idt_desc_t gi; // our idt entry
   gi.seg_selector = KERNEL_CS; // Want CPL 0, so use Kernel code segment
   gi.reserved4 = 0; // All set to 0 (see IA32 manual)
   // For 32-bit interrupt gates, type is 0b1110
   gi.reserved3 = 0;
   gi.reserved2 = 1;
   gi.reserved1 = 1;
   gi.size = 1;
   gi.reserved0 = 0; // Always 0 for interrupts
   gi.dpl = KERNEL_PERMISSION; // For hardware interrupts, they can have DPL 0
   gi.present = 0; // Mark our interrupt as not present by default
   // Segments will be marked present when a handler is added in setIRQhandler

   // SET_IDT_ENTRY populates the offset field with the given ISR
  SET_IDT_ENTRY(gi, routine);
  idt[num] = gi; // Add our new entry to the IDT
}

/*
    setIRQhandler
	description: Set the IRQ handler in our array to the specified handler
	inputs: vec: Vector into the IDT (with the offset of 0x20 to skip exceptions)
            handler: IRQ handler to insert into the array
	output: none
	side effect: changes the IRQhandlers array
*/
extern void setIRQhandler(int vec, handler_t handler)
{
  // Insert our handler
  IRQhandlers[vec].handler = handler;

  // If we've been given a NULL handler, we want to mark our idt entry as non-present
  if (handler != NULL) {
      idt[vec].present = 1;
  } else {
      idt[vec].present = 0;
  }
}

/*
    do_IRQ
	description: Execute a specific IRQ handlers
	inputs: vec: Vector into the IDT (with the offset of 0x20 to skip exceptions)
	output: -1 on failure, 0 on success
	side effect: Performs an interrupt handler, and sends EOI to PIC
*/
extern unsigned int do_IRQ(int vec)
{
    // Since each ISR pushed the exact bitflip of its number, bitflip it back
    // To restore the original IRQ number
    vec = ~vec;
    // Should be impossible but bounds check the handler just in case
    if (vec < 0 || vec >= NUM_VEC) return -1;

    // If we've accidently marked an entry present with no handler, quietly exit
    if (IRQhandlers[vec].handler == NULL) {
        return -1;
    }
    // If we have a valid handler, execute it
    (*IRQhandlers[vec].handler)();

    // If our interrupt came from the PIC, send it an EOI
    if (vec < NUM_PIC_VEC + IRQ_OFFSET && vec >= IRQ_OFFSET) {
        send_eoi(vec - IRQ_OFFSET);
    }
    return 0;
}

/*
	do_exception
	description: Performs a system exception with a given message
	inputs: args - Data pushed to stack by CPU
	output:	None
	side effect: Freezes the system and turns the screen blue, will halt current program if occurred in user space
*/
void do_exception(except_args args)
{

    // Data according to OSDev

    // Messages indexed by IDT index
    const static char* msgs[] = {"Divide-by-zero Error", "Debug", "Non-maskable Interrupt", "Breakpoint", "Overflow",
                                 "Bound Range Exceeded", "Invalid Opcode", "Device Not Available", "Double Fault", "Coprocessor Segment Overrun",
                                 "Invalid TSS", "Segment Not Present", "Stack-Segment Fault", "General Protection Fault", "Page Fault",
                                 "Assertion Failure", "x87 Floating-Point Exception", "Alignment Check", "Machine Check", "SIMD Floating-Point Exception",
                                 "Virtualization Exception", "Reserved", "Reserved", "Reserved", "Reserved",
                                 "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
                                 "Security Exception", "Reserved", "Triple Fault", "FPU Error Interrupt"};

    // Exception types indexed by IDT index
    const static enum except_type types[] = {fault, trap, interrupt, trap, trap,
                                        fault, fault, fault, abort, fault,
                                        fault, fault, fault, fault, fault,
                                        none, fault, fault, abort, fault,
                                        fault, none, none, none, none,
                                        none, none, none, none, none,
                                        none, none, none, interrupt};
    // Booleans representing an error code indexed by IDT index
    const static uint8_t has_error_code[] = {0,0,0,0,0,
                                             0,0,0,1,0,
                                             1,1,1,1,1,
                                             0,0,0,0,0,
                                             0,0,0,0,0,
                                             1,0,0,0};


    int i;

    // All of the possible exceptions, indexed by their IDT index
    exception_t exceptions[NUM_EXCEPT];

    // Fill each exception struct with its data
    for (i = 0; i < NUM_EXCEPT; i++) {
        exceptions[i].msg = msgs[i];
        exceptions[i].type = types[i];
        exceptions[i].has_error_code = has_error_code[i];
    }

    // Decide what exception has occurred
    args.IRQ = ~args.IRQ;
    exception_t* exc = &exceptions[args.IRQ];

	// Clear the screen and Reset the cursor to the top
	if (args.CS == KERNEL_CS || exc->type == abort) {
		// Fatal exception, BSOD to vidmem
		set_vidmem(tid);
	}
    clear();
    setcursor(0,0);

	// Print our exception message
    printf("Exception Occurred: %s\n", exc->msg);

    // For faults with Error Codes
    // arg[6] - ~(IRQ)
    // arg[5] - Error Code
    // arg[4] - Return EIP
    // arg[3] - cs
    // arg[2] - eflags
    // arg[1] - esp
    // arg[0] - ss

    // For faults without Error Codes
    // arg[6] - ~(IRQ)
    // arg[5] - Return EIP
    // arg[4] - cs
    // arg[3] - eflags
    // arg[2] - esp
    // arg[1] - ss
    // arg[0] - garbagio

    if (!exc->has_error_code) {
        // Oof. This is rough. If the hardware didn't actually push an error code,
        // Most of the values in our struct are off by one. So we have to shift them.
        args.SS = args.ESP;
        args.ESP = args.EFLAGS;
        args.EFLAGS = args.CS;
        args.CS = args.EIP;
        args.EIP = args.Error;
    } else {
        // If we did get an error code, print it
        printf("Error code: %x\n", args.Error);
    }

    // Print out the rest of the exception information
    printf("SS: %x\n", args.SS);
    printf("ESP: %x\n", args.ESP);
    printf("ELAGS: %x\n", args.EFLAGS);
    printf("CS: %x\n", args.CS);
    printf("EIP: %x\n", args.EIP);

    uint32_t cr2;
    asm volatile( "mov %%cr2, %0" : "=r" ( cr2 ));
    printf("CR2: %x\n", cr2);


    // If we excepted in user space, print data related to the process that caused the exception
    if (args.CS == USER_CS) {
        pcb_t* pcb = get_current_pcb();
        printf("PID: %d\nParent ID: %d\nParent ESP: %x\n",
        pcb->process_id,
        pcb->parent_id,
        pcb->parent_esp
        );
    }


    // Note: In the case that we return from an exception, we never go back to the assembly linkage, we simply
    // Call halt, which will return control to the parent. Under normal circumstances, one might worry about a stack
    // Imbalance over many excepted processes. However, This process ends immediately, and a new one starts in its place.
    // When this PID is recycled, the stack pointer is reset to the bottom, and the original information may be overwritten.


    // We can only return from exceptions that aren't aborts, and occurred in user space
    if (args.CS == USER_CS && exc->type != abort) {

        // Set the screen blue and prompt the user
        printf("Press enter to return\n");
        setbg(BLUE);
        sti();

        // Block until the user hits enter (read 0 bytes into a non-existant buffer)
        terminal_read(0, "", 0);
        cli(); // We will STI when execute is finished

        // Reset the screen
        clear();
        setbg(BLACK);
        setcursor(0,0);

        // The argument to system_halt is actually arbitrary, what's important is that we mark our process as crashed
        get_current_pcb()->crashed = TRUE;
        system_halt(EXCEPT_RET_VAL);
    } else {
        // If we cannot recover, let the user know.
        printf("This exception was fatal. Please Restart the System.\n");
        setbg(BLUE);
        /* Spin forever */
        asm volatile (".1: hlt; jmp .1;");
    }
}
