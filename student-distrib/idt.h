#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include "x86_desc.h"



#ifndef ASM
#include "syscall.h"
// interrupt service routines
extern void irq00(void);
extern void irq01(void);
extern void irq02(void);
extern void irq03(void);
extern void irq04(void);
extern void irq05(void);
extern void irq06(void);
extern void irq07(void);
extern void irq08(void);
extern void irq09(void);
extern void irq0A(void);
extern void irq0B(void);
extern void irq0C(void);
extern void irq0D(void);
extern void irq0E(void);
extern void irq0F(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);
extern void irq16(void);
extern void irq17(void);
extern void irq18(void);
extern void irq19(void);
extern void irq1A(void);
extern void irq1B(void);
extern void irq1C(void);
extern void irq1D(void);
extern void irq1E(void);
extern void irq1F(void);
extern void irq20(void);
extern void irq21(void);
extern void irq22(void);
extern void irq23(void);
extern void irq24(void);
extern void irq25(void);
extern void irq26(void);
extern void irq27(void);
extern void irq28(void);
extern void irq29(void);
extern void irq2A(void);
extern void irq2B(void);
extern void irq2C(void);
extern void irq2D(void);
extern void irq2E(void);
extern void irq80(void);


#endif
#endif
