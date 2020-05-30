# #include "x86_desc.S"
#define ASM   1
# When we want to link to the C function after pushing all the registers,
# this is how far down the stack we look
#define IRQ_ARG_OFFSET1	(44)
#define IRQ_ARG_OFFSET2	(48)
#define IRQ_ARG_OFFSET3	(52)
#define IRQ_ARG_OFFSET4	(56)
#define IRQ_ARG_OFFSET5	(60)
#define IRQ_ARG_OFFSET6	(64)
#define IRQ_ARG_OFFSET7	(68)
#define ESP0_STACK_ADD  (60)
#define TSS_ESP0_OFF    (4)
#define EXECUTE_SYSCALL_NO (2)


.data
# Share the interrupt service routine with IRQ.c, who enters them into the IDT
.globl irq00, irq01, irq02, irq03, irq04, irq05, irq06, irq07, irq08, irq09
.globl irq0A, irq0B, irq0C, irq0D, irq0E, irq0F, irq10, irq11, irq12, irq13
.globl irq14, irq15, irq16, irq17, irq18, irq19, irq1A, irq1B, irq1C, irq1D
.globl irq1E, irq1F, irq20, irq21, irq22, irq23, irq24, irq25, irq26, irq27
.globl irq28, irq29, irq2A, irq2B, irq2C, irq2D, irq2E, irq80

# jumptable of syscalls indexed by their number in syscalls/ece391sysnum.h
# These are consecutive in data, Multiple longs just used for readability
syscalls_jumptable:
    .long 0, system_halt, system_execute, system_read, system_write, system_open
    .long system_close, system_getargs, system_vidmap, system_set_handler, system_sigreturn, system_run

# Minimum and maximum syscalls allowable
min_syscall_no: .long 1
max_syscall_no: .long 11
.text

# common_interrupt
# description: Function linking Assembly interrupt service routines to the
#              C function do_IRQ
# inputs: Bit flipped IRQ line requested (on stack)
# output: none
# side effects: Saves all registers on the stack and calls do_IRQ
common_interrupt:
	# save all registers
    cli
    pushl   %fs
    pushl   %es
    pushl   %ds
    pushal
    # Repush the initially pushed argument to link it to the C function
    pushl   IRQ_ARG_OFFSET1(%esp)
    call    do_IRQ
    jmp     ret_from_intr


# ret_from_intr
# description: Restore register values pushed by common_interrupt
# inputs: Saved Register values on stack
# output: none
# side effects: Restores all registers
ret_from_intr:

    addl   $4, %esp   # pop off the second argument push
    # restore all of our registers
    popal
    popl   %ds
    popl   %es
    popl   %fs
    addl   $4, %esp # Pop off the original argument
    iret   # Return from interrupt



# common_exception
# description: Function linking Assembly interrupt service routines to the
#              C function do_exception
# inputs: Bit flipped IRQ line requested (on stack)
# output: none
# side effects: Saves all registers on the stack and calls do_exception
common_exception:
	# save all registers
    cli # Get rid of later if using interrupt gates instead of traps
    pushl   %fs
    pushl   %es
    pushl   %ds
    pushal

    movl    %esp, %eax
    # Repush the initially pushed argument to link it to the C function
    pushl   IRQ_ARG_OFFSET1(%eax)
    pushl   IRQ_ARG_OFFSET2(%eax)
    pushl   IRQ_ARG_OFFSET3(%eax)
    pushl   IRQ_ARG_OFFSET4(%eax)
    pushl   IRQ_ARG_OFFSET5(%eax)
    pushl   IRQ_ARG_OFFSET6(%eax)
    pushl   IRQ_ARG_OFFSET7(%eax)

    call    do_exception
    jmp     ret_from_except


# ret_from_except
# description: Restore register values pushed by common_exception
# inputs: Saved Register values on stack
# output: none
# side effects: Restores all registers
ret_from_except:

    # 28 is 7*4 (we push 7 arguments of 4 bytes each)
    addl   $28, %esp   # pop off the second argument push
    # restore all of our registers
    popal
    popl   %ds
    popl   %es
    popl   %fs
    addl   $8, %esp # Pop off the 4 byte error code and 4 byte IRQ vector
    iret   # Return from interrupt




common_syscall:
    # Save registers
    pushl   %fs
    pushl   %es
    pushl   %ds
    pushl   %ebx
    pushl   %ecx
    pushl   %edx
    pushl   %esp
    pushl   %ebp
    pushl   %esi
    pushl   %edi


    # syscall number stored in EAX

    # Make sure the syscall number is valid
    cmpl    (min_syscall_no), %eax
    jl      syscall_bad_arg

    cmpl    (max_syscall_no), %EAX
    jg      syscall_bad_arg

    # When we return, we want to know what syscall we came from (just for halt / end-of-execute)
    pushl   %eax


    # push arguments to syscall
    pushl   %edx
    pushl   %ecx
    pushl   %ebx
    # Call our syscall
    call    *syscalls_jumptable(,%eax, 4)
    addl    $12, %esp # pop off edx + ecx + ebx (4+4+4 = 12)

    jmp     ret_from_syscall

syscall_bad_arg:
    # Show a return of -1 and return
    movl    $-1, %EAX
    jmp     ret_from_syscall_no_halt

ret_from_syscall:

    # Remember what syscall we are returning from
    popl    %edx

    # see if we came from execute. If so, we have extra work to do.
    cmpl    $EXECUTE_SYSCALL_NO, %edx
    jne     ret_from_syscall_no_halt

    # We need to reset tss.esp0 back to what the esp was before this call. At this point, all that's left on the stack
    # Is 10 gp registers and an iret context ( 5 longs). So we add 4(10+5) = 60 bytes to the stack pointer to balance esp0
    movl    %esp, %edx
    addl    $ESP0_STACK_ADD, %edx

    # Get the tss
    movl    $tss, %ecx
    # Get tss.esp0
    addl    $TSS_ESP0_OFF,   %ecx
    # Put our new value (esp + 60) into esp0
    movl    %edx, (%ecx)
    # Done!
    jmp     ret_from_syscall_no_halt

ret_from_syscall_no_halt:

    # Restore registers
    popl    %edi
    popl    %esi
    popl    %ebp
    popl    %esp
    popl    %edx
    popl    %ecx
    popl    %ebx
    popl    %ds
    popl    %es
    popl    %fs
    # return control to parent process
    iret



# interrupt service routines
# push negative minus one so the do irq can just not the arg and get irq line
# this is becuase the lecture notes told us to do so

# irq00
# Description: Call an interrupt with vector -1
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq00:
    pushl   $-1
    jmp    common_exception

# irq01
# Description: Call an interrupt with vector -2
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq01:
    pushl   $-2
    jmp    common_exception

# irq02
# Description: Call an interrupt with vector -3
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq02:
    pushl   $-3
    jmp    common_exception

# irq03
# Description: Call an interrupt with vector -4
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq03:
    pushl   $-4
    jmp    common_exception

# irq04
# Description: Call an interrupt with vector -5
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq04:
    pushl   $-5
    jmp    common_exception

# irq05
# Description: Call an interrupt with vector -6
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq05:
    pushl   $-6
    jmp    common_exception

# irq06
# Description: Call an interrupt with vector -7
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq06:
    pushl   $-7
    jmp    common_exception

# irq07
# Description: Call an interrupt with vector -8
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq07:
    pushl   $-8
    jmp    common_exception

# irq08
# Description: Call an interrupt with vector -9
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq08:
    pushl   $-9
    jmp    common_exception

# irq09
# Description: Call an interrupt with vector -10
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq09:
    pushl   $-10
    jmp    common_exception

# irq0A
# Description: Call an interrupt with vector -11
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq0A:
    pushl   $-11
    jmp    common_exception

# irq0B
# Description: Call an interrupt with vector -12
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq0B:
    pushl   $-12
    jmp    common_exception

# irq0C
# Description: Call an interrupt with vector -13
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq0C:
    pushl   $-13
    jmp    common_exception

# irq0D
# Description: Call an interrupt with vector -14
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq0D:
    pushl   $-14
    jmp    common_exception

# irq0E
# Description: Call an interrupt with vector -15
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq0E:
    pushl   $-15
    jmp    common_exception

# irq0F
# Description: Call an interrupt with vector -16
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq0F:
    pushl   $-16
    jmp    common_exception

# irq10
# Description: Call an interrupt with vector -17
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq10:
    pushl   $-17
    jmp    common_exception

# irq11
# Description: Call an interrupt with vector -18
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq11:
    pushl   $-18
    jmp    common_exception

# irq12
# Description: Call an interrupt with vector -19
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq12:
    pushl   $-19
    jmp    common_exception

# irq13
# Description: Call an interrupt with vector -20
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq13:
    pushl   $-20
    jmp    common_exception

# irq14
# Description: Call an interrupt with vector -21
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq14:
    pushl   $-21
    jmp    common_exception

# irq15
# Description: Call an interrupt with vector -22
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq15:
    pushl   $-22
    jmp    common_exception

# irq16
# Description: Call an interrupt with vector -23
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq16:
    pushl   $-23
    jmp    common_exception

# irq17
# Description: Call an interrupt with vector -24
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq17:
    pushl   $-24
    jmp    common_exception

# irq18
# Description: Call an interrupt with vector -25
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq18:
    pushl   $-25
    jmp    common_exception

# irq19
# Description: Call an interrupt with vector -26
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq19:
    pushl   $-26
    jmp    common_exception

# irq1A
# Description: Call an interrupt with vector -27
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq1A:
    pushl   $-27
    jmp    common_exception

# irq1B
# Description: Call an interrupt with vector -28
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq1B:
    pushl   $-28
    jmp    common_exception

# irq1C
# Description: Call an interrupt with vector -29
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq1C:
    pushl   $-29
    jmp    common_exception

# irq1D
# Description: Call an interrupt with vector -30
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq1D:
    pushl   $-30
    jmp    common_exception

# irq1E
# Description: Call an interrupt with vector -31
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq1E:
    pushl   $-31
    jmp    common_exception

# irq1F
# Description: Call an interrupt with vector -32
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs exception
irq1F:
    pushl   $-32
    jmp    common_exception

# irq20
# Description: Call an interrupt with vector -33
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq20:
    pushl   $-33
    jmp    common_interrupt

# irq21
# Description: Call an interrupt with vector -34
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq21:
    pushl   $-34
    jmp    common_interrupt

# irq22
# Description: Call an interrupt with vector -35
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq22:
    pushl   $-35
    jmp    common_interrupt

# irq23
# Description: Call an interrupt with vector -36
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq23:
    pushl   $-36
    jmp    common_interrupt

# irq24
# Description: Call an interrupt with vector -37
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq24:
    pushl   $-37
    jmp    common_interrupt

# irq25
# Description: Call an interrupt with vector -38
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq25:
    pushl   $-38
    jmp    common_interrupt

# irq26
# Description: Call an interrupt with vector -39
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq26:
    pushl   $-39
    jmp    common_interrupt

# irq27
# Description: Call an interrupt with vector -40
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq27:
    pushl   $-40
    jmp    common_interrupt

# irq28
# Description: Call an interrupt with vector -41
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq28:
    pushl   $-41
    jmp    common_interrupt

# irq29
# Description: Call an interrupt with vector -42
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq29:
    pushl   $-42
    jmp    common_interrupt

# irq2A
# Description: Call an interrupt with vector -43
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq2A:
    pushl   $-43
    jmp    common_interrupt

# irq2B
# Description: Call an interrupt with vector -44
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq2B:
    pushl   $-44
    jmp    common_interrupt

# irq2C
# Description: Call an interrupt with vector -45
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq2C:
    pushl   $-45
    jmp    common_interrupt

# irq2D
# Description: Call an interrupt with vector -46
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq2D:
    pushl   $-46
    jmp    common_interrupt

# irq2E
# Description: Call an interrupt with vector -47
# Input: None
# Output: Negative Vector on Stack
# Side Effect: Performs interrupt
irq2E:
    pushl   $-47
    jmp    common_interrupt


# irq80
# Description: Call the do_syscall function
# Input: None
# Output: None
# Side Effect: Performs syscall
irq80:
    jmp common_syscall




.GLOBL user_stupid_call
user_stupid_call:
    PUSHL	%EBX
	MOVL	20(%ESP),%EAX # syscall number
	MOVL	8(%ESP),%EBX
	MOVL	12(%ESP),%ECX
	MOVL	16(%ESP),%EDX
	INT	    $0x80
	POPL	%EBX
	RET
