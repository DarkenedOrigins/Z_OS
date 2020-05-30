/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
/*
	description: Initializes the 8259 Master and Slave PICS
	inputs: none
	output:	none
	side effect: writes to pic ports sets all interrupts to masked
*/
void i8259_init(void) {

	// mask all interupts for the time being
	master_mask = ALL_IRQ_OFF;
	slave_mask = ALL_IRQ_OFF;
	outb(master_mask, MASTER_DATA_PORT);
	outb(slave_mask, SLAVE_DATA_PORT);

	//************ INITIALIZE MASTER PIC **************/
	outb(ICW1, MASTER_8259_PORT); 	// tells pic it is being initialized

	/* in the idt table interupts are given 0x20 to 0x27 for master
	icw2 tells the pic the ports for this
	0x21 is keyboard and 0x28 is rtc*/
	outb(ICW2_MASTER, MASTER_DATA_PORT);

	// tell pic it has a slave at irq2
	outb(ICW3_MASTER, MASTER_DATA_PORT);
	outb(ICW4, MASTER_DATA_PORT); 	//master expects normal eoi

	//************ INITIALIZE SLAVE PIC **************/
	outb(ICW1, SLAVE_8259_PORT);	// tells pic it is being initialized

	/*in idt table interrupts are given 0x28 to 0x2f for the slave*/
	outb(ICW2_SLAVE, SLAVE_DATA_PORT);

	//tell slave it is on master irq2 port
	outb(ICW3_SLAVE, SLAVE_DATA_PORT);
	outb(ICW4, SLAVE_DATA_PORT); 	// slave support for eoi

	//reset the pic to mask all interrupts
	outb(master_mask, MASTER_DATA_PORT);
	outb(slave_mask, SLAVE_DATA_PORT);

}

/* Enable (unmask) the specified IRQ */
/*
	description: enables irq line on specified pic
	inputs: irq line number
	output:	0 on success and -1 on fail
	side effect: writes to pic and enables irq line
*/
int enable_irq(uint32_t irq_num) {
	uint16_t port;	//which port will be accessed
	uint8_t mask;	//mask to be passed
	// check if irq num is in bounds
	if(irq_num > 15 || irq_num < 0 ){
		//do this exception
		printf("Error: IRQ Line %d does not exist", irq_num);
		return -1;
	}
	//establishes which port and offsets the irq num correctly
	if(irq_num > MAX_MASTER_IRQ_LINE){
		//slave irq line
		port = SLAVE_DATA_PORT;			//sets port to slave port
		irq_num -= SLAVE_IRQ_OFFSET;	//offsets irq num to correct slave irq
	}else{
		//master irq line
		port = MASTER_DATA_PORT;		//sets port to master port
	}
	mask = inb(port); 					//current masking in specifed pic
	mask &= ~(1 << irq_num);			//sets irq_num in mask to zero
	outb(mask, port);
	if (port == SLAVE_DATA_PORT) {
		mask = inb(MASTER_DATA_PORT);		//mask is current mask in pic
		mask &= ~(1 << SLAVE_IRQ_LINE_NUM);	//sets corresponding bit to 0 in mask
		outb(mask, MASTER_DATA_PORT);
	}
	return 0;
}

/* Disable (mask) the specified IRQ */
/*
	description: disables irq line on complementary pic
	inputs: irq line number
	output:	0 on success and -1 on fail
	side effect: writes to pic mask register and masks a specifed irq line
*/
int disable_irq(uint32_t irq_num) {
	uint16_t port;	//which port will be accessed
	uint8_t mask;	//mask to be passed
	// check if irq num is in bounds
	if(irq_num > 15 || irq_num < 0 ){
		//do this exception
		printf("Error: IRQ Line %d does not exist", irq_num);
		return -1;
	}
	//establishes which port and offsets the irq num correctly
	if(irq_num > MAX_MASTER_IRQ_LINE){
		//slave irq line
		port = SLAVE_DATA_PORT;	//sets port to slave port
		irq_num -= SLAVE_IRQ_OFFSET;	//offsets irq num to correct slave irq
	}else{
		//master irq line
		port = MASTER_DATA_PORT;	//sets port to master port
	}
	mask = inb(port); //current masking in specifed pic
	mask |= 1 << irq_num; // set irq_num in mask to 1 to disable
	outb(mask, port);
	if(port == SLAVE_DATA_PORT && mask == ALL_IRQ_OFF){
		mask =  inb(MASTER_DATA_PORT);		//get mask from pic
		mask |= 1 << SLAVE_IRQ_LINE_NUM;	//sets corresponding bit to 1 in mask
		outb(mask, MASTER_DATA_PORT);
	}
	return 0;
}

/* Send end-of-interrupt signal for the specified IRQ */
/*
	description: sends an end of interrupt command to corresponding pic
	inputs: irq line number that is issuing the eoi
	output: 0 on success and -1 on fail
	side effect: writes eoi "0x60" OR'ed with irq num to corresponding pic
*/
int send_eoi(uint32_t irq_num) {
	// check if irq num is in bounds
	if(irq_num > 15 || irq_num < 0 ){
		//do this exception
		printf("Error: IRQ Line %d does not exist", irq_num);
		return -1;
	}
	//determind if irq is for master(0 to 7) or slave (8 to 15)
	uint8_t SlaveIRQ;
	if(irq_num > MAX_MASTER_IRQ_LINE){
		//slave irq
		SlaveIRQ = irq_num - SLAVE_IRQ_OFFSET;	//irq line number on the slave pic
		outb(EOI | SlaveIRQ, SLAVE_8259_PORT);	//send eoi to the slave pic
		//send eoi to irq line 2 on the master pic to let it know the slave is done
		outb(EOI | SLAVE_IRQ_LINE_NUM, MASTER_8259_PORT);
	}else{
		//master irq
		//send eoi to specifed irq line on the master pic
		outb(EOI | irq_num, MASTER_8259_PORT);
	}
	return 0;
}
