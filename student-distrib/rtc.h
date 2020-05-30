#ifndef _RTC_H
#define _RTC_H
#include "lib.h"
#include "i8259.h"
#include "idt_common.h"
#include "ext-lib.h"
#include "syscall.h"


#define RTC_PIC_LINE (0x08) // RTC PIC line
#define RTC_IRQ     (RTC_PIC_LINE + IRQ_OFFSET)

#define RTC_PORT    (0x70)  // Port to read and write registers
#define CMOS_PORT   (0x71)  // Port to select registers and change NMI

// Three RTC chip register select codes
#define RTC_REG_A   (0x0A)
#define RTC_REG_B   (0x0B)
#define RTC_REG_C   (0x0C)

#define RTC_DIS_NMI (0x80)  // Disable NMI on the RTC
#define RTC_INT_EN  (0x40)  // Enable Periodic Interrupts on the RTC

// Actual interrupt rate is given by (32768 >> (RTC_RATE-1)) Hz
// Valid values are 2-15, we choose the slowest possible
#define RTC_RATE    (6)
#define MAX_FREQ	(32768 >> (RTC_RATE-1))
#define PERIOD_USEC	(976)

// Used to set the rate when writing to RTC register A
#define RTC_RATE_MASK (0xF0)

//maximum processes that the rtc can keep track of
#define MAX_IDS		(25)

#define NUM_FILES	(8)


void init_RTC(void);
void RTCHandler(void);
int32_t read_RTC(int32_t fd, int8_t* buf, uint32_t nbytes);
int32_t set_RTC(int32_t fd, int8_t* buf, uint32_t size);
int get_RTC_freq(void);
unsigned long get_Global_RTC_Clock(void);
int RTC_udelay(unsigned long usecs);


#endif
