#include "sb16.h"



// Set the DSP Transfer Sampling Rate
uint32_t sampling_rate = 44100;//44100;

uint16_t init_sound(void)
{
    got_dsp_int = 0;
    // Reset the SB16
    sb16_reset();

    // Get the SB16 Version
    uint8_t major = 0;
    uint8_t minor = 0;

    dsp_write(SB16_GET_VER);
    major = dsp_read();
    minor = dsp_read();
    //printf("Sound blaster version is %d.%d\n", major, minor );

    // Reset the mixer and set the global variables
    //init_mixer();





    // Initialize the DMA for Auto-Init Playback
    //init_DMA();


    // printf("16 bit sound transfer on DMA%d\n", dma_16);
    // printf("8 bit sound transfer on DMA%d\n", dma_8);
    // printf("Sound Blaster is Connected to IRQ%d on PIC\n", sb_pic_line);


    // return the major version in the upper byte, minor in the lower
    return (major << ONE_BYTE) | minor;
}

void sb16_reset(void)
{
    outb(SB16_RESET_1, SB16_BASE | SB16_RESET);
    //RTC_udelay(SB16_RESET_WAIT);
    outb(SB16_RESET_2, SB16_BASE | SB16_RESET);

    while (dsp_read() != SB16_ACK);
}

uint8_t dsp_read(void)
{
    while (!CHECK_FLAG(inb(SB16_BASE | SB16_RSTATUS), STATUS_BIT));
    return inb(SB16_BASE | SB16_READ);
}

void dsp_write(uint8_t data)
{
    while(CHECK_FLAG(inb(SB16_BASE | SB16_WSTATUS), STATUS_BIT));
    outb(data, SB16_BASE | SB16_WRITE);
}

void mixer_write(uint8_t reg, uint8_t data)
{
    outb(reg, SB16_BASE | MIXER_ADDR);
    //RTC_udelay(1000);
    outb(data, SB16_BASE | MIXER_CMD);
    //RTC_udelay(1000);
}

uint8_t mixer_read(uint8_t reg)
{
    outb(reg, SB16_BASE | MIXER_ADDR);
    //RTC_udelay(1000);
    return inb(SB16_BASE | MIXER_CMD);
}

void init_DMA(void)
{
    // Since segmentation isn't used on this system, the buffer's 
    // Linear and Virtual Addresses are the same
    uint32_t bp = (uint32_t)DMA_buffer; 

    // Offset of the buffer in a 128kB page
    uint32_t buf_offset = (bp / 2) % KB_128;  


    // Get the 128kB page the buffer is in
    uint32_t buf_page = bp / KB_128;

    // int buf_offset = (int)(((((long)bp)&0xFFFF0000L)>>12) +(((long)bp)&0xFFFF));

    // int buf_page = (int)(((((long)bp)&0xFFFF0000L)+((((long)bp)&0xFFFF)<<12))>>16);



    // Mask our sound card's connection to the DMA
    outb(DMA_MASK_OFF + (dma_16 % NUM_DMA_CHANNELS), DMA16_MASK_REG);

    // Send Arbitrary Data to the flip flop
    outb(0xCE, DMA16_CLEAR_BYTE_FF); 

    // Set the DMA for auto-initialized playback on our 16 channel
    outb(DMA_AI_PLAYBACK + (dma_16 % NUM_DMA_CHANNELS), DMA16_WRITE_MODE_REG);

    // Send the page offset of the buffer
    outb(NTH_BYTE(buf_offset, 0), DMA5_ADDRESS);
    outb(NTH_BYTE(buf_offset, 1), DMA5_ADDRESS);


    uint16_t transfer_len = (full_buffer / 2 ) - 1;
    // Send transfer length, number of words - 1
    outb(NTH_BYTE(transfer_len, 0), DMA5_COUNT);
    outb(NTH_BYTE(transfer_len, 1), DMA5_COUNT);

    // Write the 128 kB page
    outb(NTH_BYTE(buf_page,0), DMA5_PAGE);

    // Unmask the sound card
    outb((dma_16 % NUM_DMA_CHANNELS), DMA16_MASK_REG);
}



void init_mixer(void)
{
    mixer_write(MIXER_RESET_REG, 0xCE); // Send arbitrary data to reset
    delay(1);

    // Get Interrupt and DMA values
    uint8_t int_byte = mixer_read(MIXER_INTERRUPT_SETUP);
    uint8_t dma_byte = mixer_read(MIXER_DMA_SETUP);

    if (CHECK_FLAG(int_byte, DSP_IRQ10)) {
        sb_pic_line = 10;
    } else if (CHECK_FLAG(int_byte, DSP_IRQ7)) {
        sb_pic_line = 7;
    } else if (CHECK_FLAG(int_byte, DSP_IRQ5)) {
        sb_pic_line = 5;
    } else if (CHECK_FLAG(int_byte, DSP_IRQ2)) {
        sb_pic_line = 2;
    }

    int i;
    for (i = 0; i < ONE_BYTE; i++) {
        if (i < HALF_BYTE) {
            if (CHECK_FLAG(dma_byte, i)) dma_8 = i;
        } else {
            if (CHECK_FLAG(dma_byte, i)) dma_16 = i;
        }
    }

}

void sb16_handler(void)
{
    got_dsp_int = 1;
    inb(SB16_BASE | SB16_16_IRQ_ACK); // Acknowledge the interrupt
}

int8_t sb16_play()
{
    // These are just the upper & lower bytes of 44100Hz
    dsp_write(SB16_OUTPUT_MODE);
    dsp_write(NTH_BYTE(sampling_rate,1));
    dsp_write(NTH_BYTE(sampling_rate,0));

    // Program the DSP for Unsigned 16-bit Stereo Auto-Initialize DMA
    // Bits 7-4 are B always
    // Bit 3 is 0 for DAC (1 for ADC)
    // Bit 2 is 1 for AI (0 for Single Cycle)
    // Bit 1 is 1 for FIFO on
    // Bit 0 is 0 always
    uint8_t Command = 0xB6;

    // Bits 7,6, and 3-0 are 0 always

    // Bit 5 is 1 for stereo
    // Bit 4 is 1 for signed values
    uint8_t Mode = 0x30;

    // Length of one half-buffer in samples
    uint16_t Length = (full_buffer / 4) - 1;

    dsp_write(Command);
    dsp_write(Mode);
    dsp_write(NTH_BYTE(Length,0));
    dsp_write(NTH_BYTE(Length,1));

    // // Now will start playback

    return 0;
}
