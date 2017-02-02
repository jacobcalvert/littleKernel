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


/*
 * so here's what's going on here
 * 1) we push all the context, including SREG
 *    a) note that we clear r1, this is because r1 is expected to always hold '0'
 * 2) we load up the address of pCurrentTask into r26, r27 which is just loading up the current tasks sp location
 *	  b) see RESTORE_CONTEXT() for more reasoning on this
 * 3) we put SPL and SPH into r0 and write that to this tasks sp
 * 4) donezo.
 */

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

/*
 * so here's what's going on here
 * 1) we load up the address of pCurrentTask into r26 and r27
 * 		a) because the tasks sp is the first member of our struct, we're really loading the sp address up (yay memory)
 * 2) we then load the VALUE from XL into r28, and write r28 out to SPL
 * 3) then the VALUE from XH into r29, and r29 out to SPH
 * 4) our stack pointer now points to our next task, so we pop off all the registers
 * 		a) we pop r0 twice because once before, we push'd SREG via r0.
 * 5) we reenable interrupts and we're donezo.
 */
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
 "sei \n\t" \
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
void scheduler_add_task(task_t* t, uint8_t priority, void (*entry)())
{
	task_t *node = t;
	if(t == 0)
	{
		node = (task_t *)malloc(sizeof(task_t));
	}
	if(taskList == 0)
	{
		taskList = node;
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
	while(p != 0)
	{
		addr = (uint16_t) p->entry;
		p->flags = TASK_NEW | TASK_RDY;
		p->delay_ticks = 0;
		p->stack[TASK_STACK_SIZE-1] = addr & 0xFF;
		p->stack[TASK_STACK_SIZE-2] = (addr >> 8) & 0xFF;
		p->sp = &(p->stack[TASK_STACK_SIZE-36]);
		/*
		 * TODO: add debug in this thing
		 */
		p = p->next;
	}
	pCurrentTask = taskList;
	TCCR1B = (1<<CS11) | (1<< WGM12); 	/* io_clk / 8  */
	OCR1A = TASK_CLOCK_PRD;					/* roughly 10kHz */
	TIFR1 = 1<<OCF1A; 				/* clear oc flag */
	TIMSK1 = 1<<OCIE1A;				/* enable the int */

}
void task_sleep(uint32_t ticks)
{
	/*
	 * we must disable interrupts right away
	 * or we could have pCurrentTask swept out
	 */
	cli();
	pCurrentTask->delay_ticks =  ticks -1;
	pCurrentTask->flags &= ~TASK_RDY;
	scheduler_run();
}
void tick_handler()
{
	++gTicks;
	task_t *p = taskList;
	while(p != 0)
	{
		if(p->delay_ticks != 0)
		{
			p->delay_ticks--;
		}
		else
		{
			p->flags |= TASK_RDY;
		}
		p = p->next;
	}
}

void context_switcher()
{
#if TASK_USE_PRIORITY == 1
	uint8_t ctxPri = 0;
	task_t *p = taskList;
	pCurrentTask = taskList; /* default to the idle task */
	while(p != 0)
	{
		if(p->flags & TASK_RDY)
		{
			if(p->priority > ctxPri)
			{
				ctxPri = p->priority;
				pCurrentTask = p;
			}
		}
		p = p->next;
	}


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
void scheduler_begin()
{
	/*
	 * explanation
	 * set the current task pointer to 3 below the end of the stack
	 * 	- we do this because the sp points to the next *open* location
	 * 	- when we do a `ret` or `reti` we pop the return address which I
	 * 	 preloaded the low and high bytes into into STACK[SZ-1] and STACK[SZ-2]
	 * 	- see scheduler_init() for this setup
	 * set the processors SP to our task sp
	 * enable interrupts
	 * jump to the new task and let scheduling begin
	 */
	pCurrentTask->sp = &(pCurrentTask->stack[TASK_STACK_SIZE - 3]);
	SP = (uint16_t)pCurrentTask->sp;
	sei();
	asm volatile ( "ret" );
}
void scheduler_run()
{
	/*
	 * explanation here
	 * 1) SAVE 				- saves all registers state and the current SP
	 * 2) tick_handler 		- does the tick countdowns, this handles sleeping tasks
	 * 3) context_switcher	- decides what pCurrentTask is set to next based on priority and readiness
	 * 4) RESTORE			- restores all registers from pCurrentTask
	 * 5) ret				- pops the return address off the stack and hops there, resuming where it left off
	 */
	SAVE_CONTEXT();

	tick_handler();

	context_switcher();

	RESTORE_CONTEXT();
	asm volatile ( "ret" );
}
