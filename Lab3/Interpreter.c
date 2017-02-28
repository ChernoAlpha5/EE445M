// UARTIntsTestMain.c
// Runs on LM4F120/TM4C123
// Tests the UART0 to implement bidirectional data transfer to and from a
// computer running HyperTerminal.  This time, interrupts and FIFOs
// are used.
// Daniel Valvano
// September 12, 2013

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
   Program 5.11 Section 5.6, Program 3.10

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1

#include <stdint.h>
#include "UART.h"
#include "ADC.h"
#include "ST7735.h"
#include "OS.h"
#include "../inc/tm4c123gh6pm.h"

#define PF2             (*((volatile uint32_t *)0x40025010))
#define PF1             (*((volatile uint32_t *)0x40025008))

//---------------------OutCRLF---------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
void OutCRLF(void){
  UART_OutChar(CR);
  UART_OutChar(LF);
}

void heartBeatInit(){
	SYSCTL_RCGCGPIO_R |= 0x20;            // activate port F
	while(!(SYSCTL_RCGCGPIO_R&0x20)){;}
	GPIO_PORTF_DIR_R |= 0x06;             // make PF2, PF1 out (built-in LED)
  GPIO_PORTF_AFSEL_R &= ~0x06;          // disable alt funct on PF2, PF1
  GPIO_PORTF_DEN_R |= 0x06;             // enable digital I/O on PF2, PF1
                                        // configure PF2 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFFF00F)+0x00000000;
  GPIO_PORTF_AMSEL_R = 0;               // disable analog functionality on PF
  PF2 = 0;                      // turn off LED
}

void toggleLed(void){
	PF2^=0x04;
}
//debug code
void Interpreter(void){
  char string[20];  // global to assist in debugging
	heartBeatInit();
	//Output_Init();
	
  while(1){
    UART_OutString("> ");
    UART_InToken(string,19);
		switch(string[0]){
			case 'A':
			case 'a':
				if(string[3] =='t' || string[3] == 'T'){
					ADC_Open(0);
					UART_OutString(" ");
					UART_OutUDec(ADC_In());
				}
				break;
			case 'U':
			case 'u':
				UART_OutString(" ");
				UART_InString(string,19);
				ST7735_Message(ST7735_UP, 0, string, 0);
				break;
			case 'D':
			case 'd':
				UART_OutString(" ");
				UART_InString(string,19);
				ST7735_Message(ST7735_DOWN, 0, string, 0);
				break;
			default:
				UART_OutString("Error");
		}
		OutCRLF();
  }
}
