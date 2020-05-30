#include "rtc.h"


/*****RTC Global Vars*****/
volatile static unsigned long Global_RTC_Clock;
//static unsigned int FreqArr[MAX_IDS];


/*
    init_RTC
	description: Initializes the Real Time Clock to send periodic interrupts
	inputs: none
	output: none
	side effect: Enables RTC interrupts and changes their frequency, also
                 unmasks IRQ8 (Master IRQ2 and Slave IRQ0)
*/
void init_RTC(void)
{
    uint8_t reg; // General purpose register storage

    // Enable periodic Interrupts

	// Disable interrupts to avoid interference on RTC_PORT and CMOS_PORT
    disable_irq(RTC_PIC_LINE);
	// Disable nonmaskable interrupts and select register B
    outb(RTC_REG_B | RTC_DIS_NMI, RTC_PORT);
    reg = inb(CMOS_PORT); // Get the value of register B
    outb(RTC_REG_B | RTC_DIS_NMI, RTC_PORT); // Select Register B again
    outb(reg | RTC_INT_EN, CMOS_PORT);	// Write to B with bit 6 (periodic interrupts) enabled

    // set the frequency to lowest setting (32768 >> (RTC_RATE-1)) Hz

    // Select Register A and disable NMI
    outb(RTC_REG_A | RTC_DIS_NMI, RTC_PORT);
    reg = inb(CMOS_PORT); // Read Register A
    outb(RTC_REG_A | RTC_DIS_NMI, RTC_PORT); // Select Register A again
    outb(RTC_RATE | (reg & RTC_RATE_MASK), CMOS_PORT); // Set the lower four bits of reg A to rate

	//set the global clock to zero and fill the FreqArr with max freq
	Global_RTC_Clock = 0;

    // Re-enable interrupts
    enable_irq(RTC_PIC_LINE);


}

/*
    RTCHandler
	description: Handles periodic inputs from the Real-time clock
	inputs: none
	output:	none
	side effect: (Currently) writes to screen, read from RTC
*/
void RTCHandler(void)
{
	Global_RTC_Clock++; //inc the global clk to show an interrupt has occured
    // Read from RTC register C and discard the result
    // This read serves as an acknowledge to the RTC, so it will send another interrupt
    outb(RTC_REG_C, RTC_PORT);
    inb(CMOS_PORT);
}
/*
description: "sets" the rtc frequency for a priticular process
input:
	process id : to identify which process is asking for the freq to be set
	frequency : the frequency to set the rtc to in Hz
output:
	0: on success
	-1: if frequncy is higher than rtc max or not a power of 2 or 0
sideeffct: none
*/
int32_t set_RTC(int32_t fd, int8_t* buf, uint32_t size){
	// freq must be less than the max be a power of 2 and not be zero
	int32_t temp = *(int32_t*)buf;
	if( temp > MAX_FREQ || !is_power_2( temp ) || temp == 0 ){
		return -1;
	}
	// go to id in array and insert freq
	pcb_t* PcbPtr = get_current_pcb();
	PcbPtr->rtc_rate = temp;
	return 0;
}
/*
description: returns when after an appropriate frequncy wait
input:
	process id : to identify which process is asking for the freq
				 if id has no set freq value default (MAX_FREQ) is used
	frequency : the frequency to wait for in Hz
output:
	0: on success or on bad fd
sideeffct: none
*/
int32_t read_RTC(int32_t fd, int8_t* buf, uint32_t nbytes){
	sti();

	//check fd
	if(fd<0 || fd>NUM_FILES){
		return 0;
	}
	//get rate from pcb block
	uint32_t rate = (get_current_pcb())->rtc_rate;
	unsigned long PrevClk = Global_RTC_Clock;
	// these are the number of interrupts that must happen v
	unsigned long TicksToElaspe = (unsigned long) ( MAX_FREQ/rate);
	while(1){
		if( ( Global_RTC_Clock - PrevClk ) >= TicksToElaspe ){
			// if ticks have elasped break the while loop
			break;
		}
	}
	return 0;
}
/*
description: gets the rtc frequency
input: process id to see which process wants which id
output: the freq that the p.id had set
side effect: none
*/
int get_RTC_freq( void ){
	//id fits in array
	// if( ProcessId >= MAX_IDS ){
	// 	return -1;
	// }
	// return that freq
	return (get_current_pcb())->rtc_rate;
}
/*
description: this returns the global clk
input: none
output: cur glb clk var
side effect: none
*/
unsigned long get_Global_RTC_Clock(void){
	return Global_RTC_Clock;
}

/*
description: waits for a given number of usecs to pass
input: int of mico"u" seconds to wait that is higher than PERIOD_USEC
output: -1 on error 0 on regular completion
sideeffect: reads global clock variable
*/
int RTC_udelay(unsigned long usecs){
	if(usecs < PERIOD_USEC){
		return -1;
	}
	unsigned long ticks = usecs/PERIOD_USEC;	//how many intrps must happen
	//curr clock so we can diff it as it progresses
	unsigned long PrevClk = Global_RTC_Clock;
	// wait until the diff is equal to the ticks needed
	while( (Global_RTC_Clock - PrevClk) != ticks){}
	return 0;
}
