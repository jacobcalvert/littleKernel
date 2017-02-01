#include "littleKernel/task/task.h"
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static uint16_t taskIdCounter = 0;
task_t *taskList = 0;
static task_t *pCurrentTask = 0;

volatile uint8_t fInISR = 0;
volatile uint32_t gTicks = 0;



#define SAVE_CONTEXT() \
asm volatile ( \
 "push r0 \n\t" \
 "in r0, __SREG__ \n\t" \
 "cli \n\t" \
 "push r0 \n\t" \
 "push r1 \n\t" \
 "clr r1 \n\t" \
 "push r2 \n\t" \
 "push r3 \n\t" \
 "push r4 \n\t" \
 "push r5 \n\t" \
 "push r6 \n\t" \
 "push r7 \n\t" \
 "push r8 \n\t" \
 "push r9 \n\t" \
 "push r10 \n\t" \
 "push r11 \n\t" \
 "push r12 \n\t" \
 "push r13 \n\t" \
 "push r14 \n\t" \
 "push r15 \n\t" \
 "push r16 \n\t" \
 "push r17 \n\t" \
 "push r16 \n\t" \
 "push r19 \n\t" \
 "push r20 \n\t" \
 "push r21 \n\t" \
 "push r22 \n\t" \
 "push r23 \n\t" \
 "push r24 \n\t" \
 "push r25 \n\t" \
 "push r26 \n\t" \
 "push r27 \n\t" \
 "push r28 \n\t" \
 "push r29 \n\t" \
 "push r30 \n\t" \
 "push r31 \n\t" \
 "lds r26, pCurrentTask \n\t" \
 "lds r27, pCurrentTask + 1 \n\t" \
 "in r0, __SP_L__ \n\t" \
 "st x+, r0 \n\t" \
 "in r0, __SP_H__ \n\t" \
 "st x+, r0 \n\t" \
);


#define RESTORE_CONTEXT() \
asm volatile ( \
 "lds r26, pCurrentTask \n\t" \
 "lds r27, pCurrentTask + 1 \n\t" \
 "ld r28, x+ \n\t" \
 "out __SP_L__, r28 \n\t" \
 "ld r29, x+ \n\t" \
 "out __SP_H__, r29 \n\t" \
 "pop r31 \n\t" \
 "pop r30 \n\t" \
 "pop r29 \n\t" \
 "pop r28 \n\t" \
 "pop r27 \n\t" \
 "pop r26 \n\t" \
 "pop r25 \n\t" \
 "pop r24 \n\t" \
 "pop r23 \n\t" \
 "pop r22 \n\t" \
 "pop r21 \n\t" \
 "pop r20 \n\t" \
 "pop r19 \n\t" \
 "pop r18 \n\t" \
 "pop r17 \n\t" \
 "pop r16 \n\t" \
 "pop r15 \n\t" \
 "pop r14 \n\t" \
 "pop r13 \n\t" \
 "pop r12 \n\t" \
 "pop r11 \n\t" \
 "pop r10 \n\t" \
 "pop r9 \n\t" \
 "pop r8 \n\t" \
 "pop r7 \n\t" \
 "pop r6 \n\t" \
 "pop r5 \n\t" \
 "pop r4 \n\t" \
 "pop r3 \n\t" \
 "pop r2 \n\t" \
  "pop r1 \n\t" \
 "pop r0 \n\t" \
 "out __SREG__, r0 \n\t" \
 "pop r0 \n\t" \
);
void TIMER1_COMPA_vect (void) __attribute__ ((signal, naked)) ;

void TIMER1_COMPA_vect(void)
{
	scheduler_run();
	asm volatile ( "reti" );


}
void tasks_print()
{
	char temp[100];
	task_t * p = taskList;
	while(p != 0)
	{
		sprintf(temp,"Task ID: %u\r\n-->%p\r\nFlags=0x%x\r\n",p->id, p->entry, p->flags);
		putstr(temp);
		p = p->next;
	}
}
void scheduler_add_task(uint8_t priority, void (*entry)())
{
	if(taskList == 0)
	{
		taskList = (task_t *)malloc(sizeof(task_t));
		taskList->priority = priority;
		taskList->entry = entry;
		taskList->next = 0;
		taskList->id = taskIdCounter++;
	}
	else
	{
		task_t *p = taskList;
		while(p->next != 0)
		{
			p = p->next;
		}
		task_t *node = (task_t *)malloc(sizeof(task_t));
		node->priority = priority;
		node->entry = entry;
		node->next = 0;
		node->id = taskIdCounter++;
		p->next = node;
	}
}
void scheduler_init()
{
	uint16_t addr;
	task_t * p = taskList;
	putstr("inside init\r\n");
	char temp[64];
	while(p != 0)
	{
		putstr("Top");
		addr = (uint16_t) p->entry;
		p->flags = TASK_NEW | TASK_RDY;
		p->delay_ticks = 0;
		p->stack[TASK_STACK_SIZE-1] = addr & 0xFF;
		p->stack[TASK_STACK_SIZE-2] = (addr >> 8) & 0xFF;
		p->sp = &(p->stack[TASK_STACK_SIZE-36]);
		/*
		 * TODO: add debug in this thing
		 */
		putstr(".");
		p = p->next;
	}
	pCurrentTask = taskList;
	TCCR1B = (1<<CS11) | (1<< WGM12); 	/* io_clk / 1024  */
	OCR1A = 200;					/* roughly 10kHz */
	TIFR1 = 1<<OCF1A; 				/* clear oc flag */
	TIMSK1 = 1<<OCIE1A;				/* enable the int */
	putstr("about to exit init\r\n");

}
uint16_t scheduler_get_sp()
{
	return (uint16_t) pCurrentTask->sp;
}
void task_sleep(uint32_t ticks)
{
	pCurrentTask->delay_ticks =  ticks -1;
	pCurrentTask->flags &= ~TASK_RDY;
	scheduler_run();
}
void tick_handler()
{
	task_t *p = taskList;

	while(p != 0)
	{
		if(p->delay_ticks != 0)
		{
			--(p->delay_ticks);
		}
		else
		{
			p->flags |= TASK_RDY;
		}
	}
}

void context_switcher()
{
#if TASK_USE_PRIORITY == 1
	uint8_t ctxPri = 0;
	task_t *p = taskList, *savedTask = 0;
	while(p != 0)
	{
		if(p->flags & TASK_RDY)
		{
			if(p->priority > ctxPri)
			{
				ctxPri = p->priority;
				savedTask = p;
			}
		}
		p = p->next;
	}

	pCurrentTask = savedTask;

#else
		if(pCurrentTask->next != 0)
		{
			pCurrentTask = pCurrentTask->next;
		}
		else
		{
			pCurrentTask = taskList;
		}

#endif
}
uint64_t get_ticks()
{
	return gTicks;
}
void scheduler_run()
{
	SAVE_CONTEXT();

	tick_handler();

	context_switcher();

	RESTORE_CONTEXT();
	sei();
	asm volatile ( "ret" );
}
