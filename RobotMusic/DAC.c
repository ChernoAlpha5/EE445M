#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "DAC.h"

#define PD0  	(*((volatile uint32_t *)0x40007004))
#define PD1  	(*((volatile uint32_t *)0x40007008))
#define PD2  	(*((volatile uint32_t *)0x40007010))
#define PD3  	(*((volatile uint32_t *)0x40007020))

	
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

void DAC_Init(){
	volatile uint32_t delay;
	SYSCTL_RCGCGPIO_R |= 0x08;            // activate port D
	SYSCTL_RCGCSSI_R |= 0x02;		//enable SSI1
	
	while((SYSCTL_RCGCGPIO_R &0x08)==0){};
	
	GPIO_PORTD_DIR_R |= 0x0B;             // make PD output
  GPIO_PORTD_AFSEL_R |= 0x0B;          // enable alt funct 
  GPIO_PORTD_DEN_R |= 0x0B;    		// enable digital I/O 
	GPIO_PORTD_PCTL_R = (GPIO_PORTD_PCTL_R&0xFFFF0F00)+0x00002022;					//SSI1 alternate function select
  GPIO_PORTD_AMSEL_R = 0;               // disable analog functionality on PD*/

	//Init SSI
	
	
	SSI1_CR1_R = 0x00000000; 					//disable SSI, master mode
	SSI1_CPSR_R = 0x08;								//10 MHz SSIClk
	SSI1_CR0_R &= ~(0x0000FFF0);			//Set Freescale mode, SPH = 0, SPO = 1, SCR = 0;
	SSI1_CR0_R |= 0x4F;               // DSS = 16-bit data, SPO = 1
	SSI1_DR_R = 0;
		
	SSI1_CR1_R |= 0x02;					//enable SSI1
	
}


void DAC_Out(int outVal){
	SSI1_DR_R = ((~0xF000)&outVal)+0x0000;		
}
