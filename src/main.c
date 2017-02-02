#include <avr/io.h>
#include <string.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "littleKernel/task/task.h"
#include "littleKernel/utils/utils.h"
semaphore_t *uart_sem;
volatile int k = 0;

//task_t tasks[6];

#define BAUD 57600
#define BAUD_RATE ((F_CPU/16/BAUD) - 1)
void uart_init()
{
	UBRR0H = (BAUD_RATE >> 8);
	UBRR0L = BAUD_RATE;
	UCSR0B = (1 << TXEN0)| (1 << TXCIE0) | (1 << RXEN0) | (1 << RXCIE0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  //8 bit data format
}

char get_c()
{
	 while( ( UCSR0A & ( 1 << RXC0 ) ) == 0 ){}
	 char cache_c = UDR0;
	 return cache_c;
}

void put_c(char c)
{
	while (( UCSR0A & (1<<UDRE0))  == 0){};
	UDR0 = c;
}

void putstr(char *str)
{
	if(str == 0)return;
	sem_take(uart_sem, SEM_WAIT_FOREVER);
	while(*str != 0)
	{
		put_c(*str);
		str++;
	}
	sem_give(uart_sem);
}
ISR(__vector_default){
}
volatile uint16_t counter = 0;

void idle_task()
{
	while(1)
	{

		counter ++;
	}
}
char temp[60];
void counterLoop()
{
	while(1)
	{
		sprintf(temp, "-->Idle has run %u times\r\n", counter);
		putstr(temp);
		task_sleep_s(1); //1s
	}
}
void testA()
{

	while(1)
	{
		putstr("In testA\r\n");
		task_sleep_s(5); //5s
	}

}
void testB()
{
		while(1)
		{
			putstr("Here in the highest one!\r\n");
			task_sleep_s(2); //2s;
		}
}
void ledBlink()
{
	while(1)
	{
		PORTB ^= (1<<PB5);
		task_sleep_ms(100); //
	}
}
const char *BANNER = "-----------------------------------------------\r\n"
					 "------------------Hello World -----------------\r\n"
					 "-----------------------------------------------\r\n";

int main(void)
{

	uint8_t reset_cause = (MCUSR & 0xF);
	MCUSR = 0;
	uart_sem = sem_create(1, 0);
	uart_init();
	if(reset_cause & (1<<PORF))
	{
		putstr("[Power on reset occurred]\r\n");
	}
	if(reset_cause & (1<<EXTRF))
	{
		putstr("[External reset occurred]\r\n");
	}
	if(reset_cause & (1<<BORF))
	{
		putstr("[Brown out reset occurred]\r\n");
	}
	if(reset_cause & (1<<WDRF))
	{
		putstr("[WDT reset occurred]\r\n");
	}
	putstr(BANNER);
	DDRB |= (1<<PB5 | 1 << PB4);
	PORTB &= ~(1 << PB5 | 1 << PB4);
	/*
	 * let me explain scheduler_add_task() calls below
	 * letting the first param (of type task_t*) be zero
	 * we tell the routine to dynamically allocate memory
	 * for the task structure. if we pass in valid memory,
	 * then we will use that and not allocate memory. we might
	 * want to do that in case malloc is not working as expected
	 * or is unavailable
	 */
	scheduler_add_task(0,0, idle_task);
	scheduler_add_task(0,10, testA);
	scheduler_add_task(0,12, testB);
	scheduler_add_task(0,2, ledBlink);
	scheduler_add_task(0,1, counterLoop);
	scheduler_init();
	scheduler_begin();
	while(1)
	{
	}
}
