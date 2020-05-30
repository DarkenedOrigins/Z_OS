#ifndef _PIT_H_
#define	_PIT_H_

#include "lib.h"
#include "i8259.h"
#include "idt_common.h"
#include "ext-lib.h"
#include "syscall.h"


#define PIT_PIC_LINE	(0)
#define PIT_IRQ			(PIT_PIC_LINE + IRQ_OFFSET)
#define COMMAND_REG		(0x43)
#define CHL_0			(0x40)

#define MAX_RATE (65536)
#define MODIFIER_1 (21845)
#define MODIFIER_2  (397721)
#define TUNE_RATE   (42000)


/*
Pit commands
Bits         Usage
 6 and 7      Select channel :
                 0 0 = Channel 0
                 0 1 = Channel 1
                 1 0 = Channel 2
                 1 1 = Read-back command (8254 only)
 4 and 5      Access mode :
                 0 0 = Latch count value command
                 0 1 = Access mode: lobyte only
                 1 0 = Access mode: hibyte only
                 1 1 = Access mode: lobyte/hibyte
 1 to 3       Operating mode :
                 0 0 0 = Mode 0 (interrupt on terminal count)
                 0 0 1 = Mode 1 (hardware re-triggerable one-shot)
                 0 1 0 = Mode 2 (rate generator)
                 0 1 1 = Mode 3 (square wave generator)
                 1 0 0 = Mode 4 (software triggered strobe)
                 1 0 1 = Mode 5 (hardware triggered strobe)
                 1 1 0 = Mode 2 (rate generator, same as 010b)
                 1 1 1 = Mode 3 (square wave generator, same as 011b)
 0            BCD/Binary mode: 0 = 16-bit binary, 1 = four-digit BCD
 from osdev
*/
#define PIT_COMMAND		(0x34)
#define PIT_CONST_1		(3000)
#define PIT_CONST_2		(3579545)
#define PIT_SPEED_HZ	(50)
#define R_V_LOW_MASK	(0x00FF)
#define R_V_HIGH_MASK	(0xFF00)
#define SHIFT_VALUE		(8)
//time in ms = reload value * 3000(pit_cont_1) / 3579545(pit_const_2)
//solve for reload value
#define R_V 			((PIT_CONST_2 * PIT_SPEED_HZ) / PIT_CONST_1)
#define R_V_LOW 		(R_V & R_V_LOW_MASK)
#define R_V_HIGH_SHF_8	( (R_V&R_V_HIGH_MASK) >> SHIFT_VALUE )	


void init_pit(void);
void pit_handler(void);
#endif
