// Proc.c
// Runs on LM4F120/TM4C123
// Standalone process example

#include <stdio.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "inc/tm4c123gh6pm.h"

#include "OS.h"
#include "Display.h"

#define PF2     (*((volatile uint32_t *)0x40025010))
#define PF3     (*((volatile uint32_t *)0x40025020))

unsigned int line = 0;

void PortF_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x20; // activate port F
  while((SYSCTL_PRGPIO_R&0x20)==0){}; // allow time for clock to start 
	GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
	GPIO_PORTF_CR_R |= 0x0C;
  GPIO_PORTF_DIR_R |= 0x0C;    // (c) make PF4 out (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x0C;  //     disable alt funct on PF0, PF4
  GPIO_PORTF_DEN_R |= 0x0C;     //     enable digital I/O on PF4
  GPIO_PORTF_PCTL_R &= ~0x0000FF00; //  configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R &= ~0x0C;  //    disable analog functionality on PF4
  GPIO_PORTF_PUR_R |= 0x0C;     //     enable weak pull-up on PF4
}

void thread(void)
{
	unsigned int id;
		id = OS_Id();
		Display_Message(0,line++, "Thread: ", id);
		OS_Sleep(2000);
		Display_Message(0,line++, "Thread dying ", id);
		OS_Kill();

}

int main(void)
{
	unsigned int id;
	unsigned long time;
	PortF_Init();
	id = OS_Id();
	Display_Message(0,line++, "Hello world: ", id);
	OS_AddThread(thread, 128, 1);
  time = OS_Time();
	OS_Sleep(1000);
	time = (((OS_TimeDifference(time, OS_Time()))/1000ul)*125ul)/10000ul;
	Display_Message(0,line++, "Sleep time: ", time);
	OS_Kill();
}
