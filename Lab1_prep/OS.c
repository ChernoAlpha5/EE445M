// OS.c
// Runs on TM4C123
// Andrew Wong and Clint Simpson
// LPOS-1 (Low-Profile Operating System, v1)

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

#define PF2   			(*((volatile uint32_t *)0x40025010))
#define PF1         (*((volatile uint32_t *)0x40025008))
	
void (*PeriodicTask)(void);   // user function
uint32_t osCounter = 0;

// ***************** Timer5_Init ****************
// Activate Timer4 interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in units (1/clockfreq)
// Outputs: none
int OS_AddPeriodicThread(void(*task)(void), uint32_t period, uint32_t priority){
  SYSCTL_RCGCTIMER_R |= 0x20;      // 0) activate timer5                    // wait for completion
	PeriodicTask = task;          // user function
  TIMER5_CTL_R &= ~0x00000001;     // 1) disable timer5A during setup
  TIMER5_CFG_R = 0x00000004;       // 2) configure for 16-bit timer mode
  TIMER5_TAMR_R = 0x00000002;      // 3) configure for periodic mode, default down-count settings
  TIMER5_TAILR_R = ((80000000/period)-1);       // 4) reload value
  TIMER5_TAPR_R = 49;              // 5) 1us timer5A
  TIMER5_ICR_R = 0x00000001;       // 6) clear timer5A timeout flag
  TIMER5_IMR_R |= 0x00000001;      // 7) arm timeout interrupt
  NVIC_PRI23_R = (NVIC_PRI23_R&0xFFFFFF00)|(0x20 << priority); // 8) priority 2
  NVIC_EN2_R |= 0x10000000;        // 9) enable interrupt 19 in NVIC
  // vector number 108, interrupt number 92
  TIMER5_CTL_R |= 0x00000001;      // 10) enable timer5A
// interrupts enabled in the main program after all devices initialized
	return 0;
}

void Timer5A_Handler(void){
	PF1 = 0x2;							// used for measuring ISR time
  TIMER5_ICR_R = TIMER_ICR_TATOCINT;// acknowledge timer0A timeout
	osCounter++; 												//increment global counter
  (*PeriodicTask)();                // execute user task
	PF1 = 0x0;
}

void OS_ClearPeriodicTime(void){
	osCounter = 0;
}

uint32_t OS_ReadPeriodicTime(void){
	return osCounter;
}
