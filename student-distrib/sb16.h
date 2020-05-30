#ifndef _SB16_H
#define _SB16_H

#include "lib.h"
#include "rtc.h"
#include "idt_common.h"
#include "types.h"
#include "fs.h"

#define CHECK_FLAG(flags, bit)   ((flags) & (1 << (bit)))


typedef struct {
    uint16_t left;
    uint16_t right;
} sample_t;

typedef struct {
    int8_t ChunkID[4];
    uint32_t ChunkSize;
    int8_t Format[4];
    int8_t Subchunk1ID[4];
    uint32_t Subchunk1Size;
    uint16_t AudioFormat;
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
    int8_t Subchunk2ID[4];
    uint32_t Subchunk2Size;
} wav_data_t;


// Reset constants
#define SB16_RESET_1    (0x01)
#define SB16_RESET_2    (0x00)
#define SB16_RESET_WAIT (1000) // Minimum wait possible
#define SB16_ACK        (0xAA)
#define SB16_OUTPUT_MODE    (0x41)

#define STATUS_BIT      (7)

// DSP Ports
#define SB16_BASE       (0x220)
#define SB16_RESET      (0x06)
#define SB16_READ       (0x0A)
#define SB16_WRITE      (0x0C)
#define SB16_RSTATUS    (0x0E)
#define SB16_WSTATUS    (0x0C)
#define SB16_16_IRQ_ACK (0x0F)    

// DSP Commands
#define SB16_GET_VER                        (0xE1)
#define SB16_TURN_ON_SPEAKER                (0xD1)
#define SB16_8BIT_DIRECT_SINGLE_BYTE        (0x10)
#define SB16_END_AI_16                      (0xD9)

// Mixer Ports
#define MIXER_ADDR      (0x04)
#define MIXER_CMD       (0x05)

// Mixer Registers and Commands
#define MIXER_RESET_REG (0x00)
#define MIXER_INTERRUPT_SETUP   (0x80)
#define MIXER_DMA_SETUP         (0x81)
#define MIXER_INTERRUPT_STATUS  (0x82)

// Settings Constants
#define DSP_IRQ10           (3)
#define DSP_IRQ7            (2)
#define DSP_IRQ5            (1)
#define DSP_IRQ2            (0)

// DMA Ports
#define DMA16_MASK_REG        (0xD4)
#define DMA16_WRITE_MODE_REG  (0xD6)
#define DMA16_CLEAR_BYTE_FF   (0xD8)

#define DMA5_ADDRESS          (0xC4)
#define DMA5_COUNT            (0xC6)
#define DMA5_PAGE             (0x8B)

// DMA Constants
#define DMA_MASK_OFF        (4)
#define NUM_DMA_CHANNELS    (4)
#define DMA_AI_PLAYBACK     (0x58)


// WAV Constants
#define WAV_ID_LEN          (4)
#define WAV_ID              ("RIFF")
#define WAV_DATA_OFFSET     (44)


uint16_t init_sound(void);
void sb16_reset(void);
uint8_t dsp_read(void);
void dsp_write(uint8_t data);
void sb16_beep(void);
void mixer_write(uint8_t reg, uint8_t data);
uint8_t mixer_read(uint8_t reg);
void init_DMA(void);
void init_mixer(void);
void sb16_handler(void);
int8_t sb16_play();

uint8_t dma_16;
uint8_t dma_8;
uint8_t sb_pic_line;

//uint8_t DMA_buffer_big[60000];

uint8_t * DMA_buffer;

uint32_t full_buffer;

volatile uint8_t got_dsp_int;

#endif
