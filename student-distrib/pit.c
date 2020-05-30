#include "pit.h"

/*
description: returns reload value for given frequency in hz
input: frequency in hz
putput: reload val in 16bit
sfx: none
*/
uint16_t pit_get_reload_val(unsigned long hz){
	// return (uint16_t)(26.065.436.666/397721-21845*hz/397721);
	return ( (uint16_t)(MAX_RATE-MODIFIER_1*hz/MODIFIER_2) )-TUNE_RATE;
}

/*
description: initializes the pit
input: none
output: none
sfx: enables irq line on pic and sets the pit to generate ints at PIT_SPEED_HZ
*/
void init_pit(void){
	disable_irq(PIT_PIC_LINE);
	uint16_t reload_val = pit_get_reload_val(PIT_SPEED_HZ);
	//tell the pit it is being programmed to be a square wave generator with rate of PIT_SPEED_MS
	outb(PIT_COMMAND, COMMAND_REG);
	//send lower 8 bits of the reload_val to the channel zero
	outb((uint8_t)(reload_val&R_V_LOW_MASK), CHL_0);
	//now send the upper 8 bits left aligned
	outb((uint8_t)((reload_val&R_V_HIGH_MASK)>>SHIFT_VALUE),CHL_0);
	enable_irq(PIT_PIC_LINE);
	return;
}
