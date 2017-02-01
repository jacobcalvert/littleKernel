#ifndef __SPOS_TASK__
#define __SPOS_TASK__
#include <stdint.h>

#include "littleKernel/config.h"


#define TASK_RDY 		(1<<0)
#define TASK_DEAD		(1<<6)
#define TASK_NEW		(1<<7)

typedef struct task
{

	volatile uint8_t *sp;
	uint8_t stack[TASK_STACK_SIZE];
	uint8_t id;
	uint8_t priority;
	uint8_t flags;
	uint32_t delay_ticks;

	void (*entry)();

}task_t;

void scheduler_init(task_t *list, uint8_t n);
void scheduler_run() __attribute__ ((naked));

uint64_t get_ticks();


void task_sleep(uint32_t ticks);




#endif
