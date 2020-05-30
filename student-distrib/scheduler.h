#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "types.h"
#include "lib.h"
#include "paging.h"
#include "i8259.h"
#include "terminal.h"

#define NO_JOBS		(-1)
#define PIT_PIC_LINE	(0)

// Core Functions
void init_scheduling(void);
void schedulerHandler(void);
int32_t schedule_job(const uint8_t* command, int32_t* retval, int32_t tid, uint8_t haltable);

#endif
