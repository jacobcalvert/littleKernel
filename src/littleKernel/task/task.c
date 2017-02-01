#include "littleKernel/task/task.h"
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
volatile uint8_t numTasks = 0, currentTaskIdx = 0;
volatile task_t *taskList = 0;
volatile static task_t *pCurrentTask = 0;

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
void scheduler_init(task_t *list, uint8_t n)
{
	taskList = list;
	numTasks = n;
	uint16_t addr;
	char temp[100];
	for(uint8_t i = 0; i < n; i++)
	{
		addr = (uint16_t) taskList[i].entry;
		taskList[i].flags = TASK_NEW | TASK_RDY;
		taskList[i].id = i;
		taskList[i].delay_ticks = 0;
		taskList[i].stack[TASK_STACK_SIZE-1] = addr & 0xFF;
		taskList[i].stack[TASK_STACK_SIZE-2] = (addr >> 8) & 0xFF;
		if(i == 0)
		{

			taskList[i].sp = &taskList[i].stack[TASK_STACK_SIZE-3];

		}
		else
		{
			taskList[i].sp = &taskList[i].stack[TASK_STACK_SIZE-36];
		}
		sprintf(temp, "For task at %p, the stack is (sp+2) = 0x%02x, (sp+1) = 0x%02x\r\n",taskList[i].entry,*(taskList[i].sp+2), *(taskList[i].sp+1) );
		putstr(temp);
	}
	pCurrentTask = taskList;
	TCCR1B = (1<<CS11) | (1<< WGM12); 	/* io_clk / 1024  */
	OCR1A = 200;					/* roughly 10kHz */
	TIFR1 = 1<<OCF1A; 				/* clear oc flag */
	TIMSK1 = 1<<OCIE1A;				/* enable the int */

}

void task_sleep(uint32_t ticks)
{
	pCurrentTask->delay_ticks =  ticks -1;
	pCurrentTask->flags &= ~TASK_RDY;
	scheduler_run();
}
void tick_handler()
{
	for(uint8_t i = 0; i < numTasks; i++)
	{
		/*
		 * if we're not yet zero, decrement
		 */
		if(taskList[i].delay_ticks != 0)
		{
			--(taskList[i].delay_ticks);
		}
		else
		{
			/* if we are zero now, make sure the run flag is set */
			taskList[i].flags |= TASK_RDY;
		}
	}
}

void context_switcher()
{
#if TASK_USE_PRIORITY == 1
	uint8_t ctxIdx, ctxPri, ctxSave;
		/*
		 * this will be where a priority task switch
		 * scheme will be implemented. for now,
		 * i'll will only use round-robin
		 */
		ctxPri = 0;
		ctxSave = 0;
		for(ctxIdx = 0; ctxIdx < numTasks; ctxIdx++)
		{
			if(taskList[ctxIdx].flags & TASK_RDY)
			{
				if(taskList[ctxIdx].priority > ctxPri)
				{
					ctxPri = taskList[ctxIdx].priority;
					ctxSave = ctxIdx;
				}
			}
		}
		pCurrentTask = &taskList[ctxSave];

#else
		currentTaskIdx++;
		if(currentTaskIdx == numTasks)
		{
			currentTaskIdx = 0;
		}
		pCurrentTask = &taskList[currentTaskIdx];

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
