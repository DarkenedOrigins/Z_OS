#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    int i;
    uint8_t buff[10];
    for (i = 0; i < 256; i++) {
        ece391_itoa(i, buff, 16);
        ece391_fdputs (1, buff);
        ece391_fdputs (1, (uint8_t*)": ");
        ece391_fdputs (1, (uint8_t*)&i);
        ece391_fdputs (1, (uint8_t*)"  ");
    }
    return 0;
}
