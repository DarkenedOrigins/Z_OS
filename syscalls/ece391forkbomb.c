#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    unsigned int i;
    // uint8_t* buff[] = {"0: ", "1: ", "2: ", "3: ", "4: ", "5: ", "6: ", "7: ",
    //                 "8: ", "9: ", "10: ", "11: ", "12: ", "13: ", "14: ", "15: ",
    //                 "16: ", "17: ", "18: ", "19: ", "20: ", "21: ", "22: ",
    //                 "23: ", "24: "};
    for (i = 0; i < (uint32_t)30; i++) {
        // ece391_fdputs(1, buff[i]);
        ece391_execute((uint8_t*) "testprint");
    }
    return 0;
}
