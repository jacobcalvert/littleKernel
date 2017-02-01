#ifndef __LK_UTILS__
#define __LK_UTILS__
#include <stdint.h>

typedef struct
{
	uint8_t count,max_count;
}semaphore_t;

#define SEM_WAIT_FOREVER 0x0000


semaphore_t *sem_create(uint8_t max_count, uint8_t init_count);
void sem_destroy(semaphore_t *s);

uint8_t sem_take(semaphore_t * s, uint16_t wait);
void sem_give(semaphore_t *s);


#endif
