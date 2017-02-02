#ifndef __LK_TASK__
#define __LK_TASK__
#include <stdint.h>

#include "littleKernel/config.h"


#define TASK_RDY 		(1<<0)
#define TASK_DEAD		(1<<6)
#define TASK_NEW		(1<<7)

#define task_sleep_ms(ms) task_sleep((F_CPU/TASK_CLOCK_PRE/TASK_CLOCK_PRD/1000)*ms)
#define task_sleep_s(s)	  task_sleep((F_CPU/TASK_CLOCK_PRE/TASK_CLOCK_PRD)*s)

typedef struct task
{

	volatile uint8_t *sp;
	uint8_t stack[TASK_STACK_SIZE];
	uint8_t id;
	uint8_t priority;
	uint8_t flags;
	uint8_t exit_val;
	uint32_t delay_ticks;
	struct task *next;

	void (*entry)();

}task_t;



void tasks_print();
void scheduler_add_task(task_t* t, uint8_t priority, void (*entry)());
void scheduler_init();
uint16_t scheduler_get_sp();
task_t *scheduler_get_top();
void scheduler_begin() __attribute__((naked));
void scheduler_run() __attribute__ ((naked));

uint64_t get_ticks();


void task_sleep(uint32_t ticks);
void task_exit(uint8_t exit_val);




#endif
