#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    int32_t retval, tty;
    uint8_t buf[1024];

    if (0 != ece391_getargs (buf, 1024)) {
        ece391_fdputs (1, (uint8_t*)"could not read arguments\n");
		return 3;
    }

	if (ece391_strncmp(buf, (uint8_t*)"-tty=", 4) == 0) {
		ece391_fdputs (1, (uint8_t*)"Got TTY=");
		ece391_fdputs (1, &buf[5]);
		ece391_fdputs (1, (uint8_t*)"\n");
		tty = (int32_t)buf[5]-0x30;
		retval = ece391_run(&buf[6], tty);
	} else {
		tty = -1;
		retval = ece391_run(buf, tty);
	}

	if (retval == -1) {
		ece391_fdputs (1, (uint8_t*)"Invalid TTY or Command\n");
	}

    return 0;
}
