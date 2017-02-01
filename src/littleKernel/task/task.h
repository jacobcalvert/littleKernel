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
	struct task *next;

	void (*entry)();

}task_t;

void tasks_print();
void scheduler_add_task(uint8_t priority, void (*entry)());
void scheduler_init();
uint16_t scheduler_get_sp();
task_t *scheduler_get_top();
void scheduler_begin() __attribute__((naked));
void scheduler_run() __attribute__ ((naked));

uint64_t get_ticks();


void task_sleep(uint32_t ticks);




#endif
