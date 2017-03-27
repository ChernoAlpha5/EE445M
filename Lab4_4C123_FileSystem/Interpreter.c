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
#include "UART2.h"
#include "ADC.h"
#include "ST7735.h"
#include "OS.h"
#include "string.h"
#include "eFile.h"

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
extern unsigned long MaxDITime;
extern unsigned long TotalDITime;


#define MAXCMDLENGTH 20

enum cmds{DITime, TotalDITimePercent, Time, TDiTime, DumpDongs, ClearDongs, ResetDongs,
					Format, Directory, PrintFile, DeleteFile, NUMCOMMANDS};

char* commands[NUMCOMMANDS] = {"DITime", "TotalDITimePercent", "Time", "TotalDITime", "DumpDongs", "ClearDongs", "ResetDongs",
															 "Format", "Directory", "PrintFile", "DeleteFile"};


int findCommand(char* command){
	for(int i=0; i<NUMCOMMANDS; i++){
		if(strncasecmp(command,commands[i], MAXCMDLENGTH) == 0){
			return i;
		}
	}
	return -1;
}

void Interpreter(void){
  char string[20];  // global to assist in debugging
	//heartBeatInit();
	//Output_Init();
	
  while(1){
    UART_OutString("> ");
    UART_InToken(string,19);
		switch(findCommand(string)){
			case DITime:
				UART_OutUDec(MaxDITime);
				break;
			case TotalDITimePercent:
				UART_OutUDec(TotalDITime*1000/OS_Time());
				break;
			case Time:
				UART_OutUDec(OS_Time());
				break;
			case TDiTime:
				UART_OutUDec(TotalDITime);
				break;
			case DumpDongs:
				OS_DumpDongs();
				break;
			case ClearDongs:
				OS_ClearDongs();
				break;
			case ResetDongs:
				OS_ResetDongs();
				break;
			case Format:
				eFile_Format();
				break;
			case Directory:
				OutCRLF();
				eFile_Directory(UART_OutChar);
				break;
			case PrintFile:
				UART_InToken(string,8);
				OutCRLF();
				eFile_Print(string, UART_OutChar);
				break;
			case DeleteFile:
				UART_InToken(string,8);
				eFile_Delete(string);
				break;
			default:
				UART_OutString("Errorection");
		}
		OutCRLF();
  }
}
