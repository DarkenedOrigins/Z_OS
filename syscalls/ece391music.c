#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

#define PLAY_BUFFER_SIZE (60000)
#define ARG_BUFFER_SIZE  (1024)
#define WAV_ID_LEN       (4)

#define DMA_BUFFER       (0x0800000)

typedef struct {
    uint8_t ChunkID[WAV_ID_LEN];
    uint32_t ChunkSize;
    uint8_t Format[WAV_ID_LEN];
    uint8_t Subchunk1ID[WAV_ID_LEN];
    uint32_t Subchunk1Size;
    uint16_t AudioFormat;
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
    uint8_t Subchunk2ID[WAV_ID_LEN];
    uint32_t Subchunk2Size;
} wav_data_t;

//uint8_t playbuffer[PLAY_BUFFER_SIZE];
uint8_t* pos0;
uint8_t* pos1;
uint8_t* fillpos;

int main ()
{

    uint8_t* playbuffer = (uint8_t*)DMA_BUFFER;
    pos0 = playbuffer;
    pos1 = &playbuffer[PLAY_BUFFER_SIZE / 2];

    fillpos = pos0;

    int32_t fd, cnt, sb16_fd;
    uint8_t argbuf[ARG_BUFFER_SIZE];
    const uint8_t * master_ID = (uint8_t*)"RIFF";

    wav_data_t metadata;

    if (0 != ece391_getargs (argbuf, ARG_BUFFER_SIZE)) {
        ece391_fdputs (1, (uint8_t*)"could not read arguments\n");
	    return 1;
    }

    if (-1 == (fd = ece391_open (argbuf))) {
        ece391_fdputs (1, (uint8_t*)"file not found\n");
	    return 2;
    }

    cnt = ece391_read (fd, &metadata, sizeof(metadata));
    if (sizeof(metadata) != cnt) {
        ece391_fdputs (1, (uint8_t*)"Failed to get metadata\n");
        return 3;
	}

    if (ece391_strncmp(master_ID, metadata.ChunkID, WAV_ID_LEN )) {
        ece391_fdputs(1, (uint8_t*)"WAV Chunk ID not correct\n");
        return 4;
    }

    cnt = ece391_read (fd, playbuffer, PLAY_BUFFER_SIZE);
    if (-1 == cnt) {
        ece391_fdputs (1, (uint8_t*)"file read failed\n");
        return 5;
	}

    sb16_fd = ece391_open((uint8_t*)"sb16");

    // int i;
    // for (i = 0; i < PLAY_BUFFER_SIZE; i++) {
    //     playbuffer[i] = i %256;
    // }

    ece391_write(sb16_fd, playbuffer, PLAY_BUFFER_SIZE);

    ece391_fdputs (1, (uint8_t*)"Now playing ");
    ece391_fdputs (1, argbuf);
    ece391_fdputs (1, "\n");

    while(1) {

        ece391_read(sb16_fd, "", 0);
        //ece391_fdputs(1, (uint8_t*)"Wow!");

        if (ece391_read(fd, fillpos, PLAY_BUFFER_SIZE / 2) < PLAY_BUFFER_SIZE / 2) break;

        fillpos = (fillpos == pos0) ? pos1 : pos0;

    }

    ece391_fdputs(1, (uint8_t*)"Done!\n");
    ece391_close(sb16_fd);
    ece391_close(fd);
    return 0;
}


