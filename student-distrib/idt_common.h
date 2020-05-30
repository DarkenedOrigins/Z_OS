#ifndef _IRQ_H
#define _IRQ_H

#include "idt.h"
#include "lib.h"
#include "pit.h"
#include "keyboard.h"
#include "rtc.h"
#include "x86_desc.h"
#include "syscall.h"
#include "colors.h"
#include "scheduler.h"


#define NUM_EXCEPT      (32) // Number of exception spaces, Defined in the IA32 manual pg 5-1
#define IRQ_OFFSET      (0x20) // Where IRQ entries start on the IDT
#define NUM_PIC_VEC     (15)   // Number of IRQ lines associated with the PIC
#define SYSCALL_GATE    (0x80)
#define EXCEPT_RET_VAL  (255)   // Value passed to system_halt from the exception handler (arbitrary)

#define USER_PERMISSION  (3)
#define KERNEL_PERMISSION (0)

// The types of possible exceptions according to OSDEV
enum except_type{
    fault,
    trap,
    interrupt,
    abort,
    none
};

// IDT entry handlers that do not return and take no arguments
typedef void (*handler_t) (void);

// Values pushed by CPU on exception, plus the IRQ pushed by the ISR
typedef struct {
    uint32_t SS, ESP, EFLAGS, CS, EIP, Error, IRQ;
} except_args;

// Struct holding IRQ handlers, can be expanded later
typedef struct {
    handler_t handler;
} irq_desc;

// All of the fields defining an exception
typedef struct {
    const char* msg; // Exception Message
    enum except_type type; // Type of exception
    uint8_t has_error_code; // Boolean defining whether or not this has an error code.
} exception_t;

// general use functions
void setupIDT(void);
void setTrap(int num, handler_t routine);
void setInt(int num, handler_t routine);
void setIRQhandler(int vec, handler_t handler);

// Wrapper functions for Interrupts and Exceptions
void do_exception(except_args args);
unsigned int do_IRQ(int vec);








#endif
