#include "littleKernel/utils/utils.h"
#include "littleKernel/task/task.h"
#include <avr/interrupt.h>

#include <stdlib.h>

semaphore_t *sem_create(uint8_t max_count, uint8_t init_count)
{
	semaphore_t *s = (semaphore_t*)malloc(sizeof(semaphore_t));
	s->max_count = max_count;
	s->count = init_count;
	return s;
}
void sem_destroy(semaphore_t *s)
{
	free(s);
}
void sem_give(semaphore_t *s)
{
	cli();
	if(s->count > 0) s->count--;
	sei();
}
uint8_t sem_take(semaphore_t * s, uint16_t wait)
{
	/*
	 * check if we can take it or not
	 */
	cli();
	if(s->count < s->max_count)
	{
		/*
		 * we can take it now, take it
		 */

		s->count++;
		sei();
		return 1;

	}
	else
	{
		/*
		 * we can't take it at this moment, do we wait or not?
		 */
		if(SEM_WAIT_FOREVER==wait)
		{
			while(1)
			{
				cli();
				if(s->count < s->max_count)
				{
					s->count++;
					sei();
					return 1;
				}
				sei();
			}
		}
		else
		{
			task_sleep(wait);
			cli();
			if(s->count < s->max_count)
			{
				s->count++;
				sei();
				return 1;
			}
			sei();
			return 0;
		}
	}

	return 0;
}

