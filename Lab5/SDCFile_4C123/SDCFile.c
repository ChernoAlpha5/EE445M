// SDCFile.c
// Runs on TM4C123
// This program is a simple demonstration of the SD card,
// file system, and ST7735 LCD.  It will read from a file,
// print some of the contents to the LCD, and write to a
// file.
// Daniel Valvano
// Feb 22, 2016

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2016
   Program 4.6, Section 4.3
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2016
   Program 2.10, Figure 2.37

 Copyright 2016 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

// hardware connections
// **********ST7735 TFT and SDC*******************
// ST7735
// 1  ground
// 2  Vcc +3.3V
// 3  PA7 TFT reset
// 4  PA6 TFT data/command
// 5  PD7/PB0 SDC_CS, active low to enable SDC
// 6  PA3 TFT_CS, active low to enable TFT
// 7  PA5 MOSI SPI data from microcontroller to TFT or SDC
// 8  PA2 Sclk SPI clock from microcontroller to TFT or SDC
// 9  PA4 MISO SPI data from SDC to microcontroller
// 10 Light, backlight connected to +3.3 V

// **********wide.hk ST7735R*******************
// Silkscreen Label (SDC side up; LCD side down) - Connection
// VCC  - +3.3 V
// GND  - Ground
// !SCL - PA2 Sclk SPI clock from microcontroller to TFT or SDC
// !SDA - PA5 MOSI SPI data from microcontroller to TFT or SDC
// DC   - PA6 TFT data/command
// RES  - PA7 TFT reset
// CS   - PA3 TFT_CS, active low to enable TFT
// *CS  - PD7/PB0 SDC_CS, active low to enable SDC
// MISO - PA4 MISO SPI data from SDC to microcontroller
// SDA  – (NC) I2C data for ADXL345 accelerometer
// SCL  – (NC) I2C clock for ADXL345 accelerometer
// SDO  – (NC) I2C alternate address for ADXL345 accelerometer
// Backlight + - Light, backlight connected to +3.3 V

#include "OS.h"
#include "../inc/tm4c123gh6pm.h"
#include "ST7735.h"
#include "UART2.h"
#include "diskio.h"
#include <string.h> 
#include <stdio.h>
#include <stdint.h>
#include "ff.h"
#include "PLL.h"
#include "heap.h"

void EnableInterrupts(void);

#define PF0  (*((volatile unsigned long *)0x40025004))
#define PF1  (*((volatile unsigned long *)0x40025008))
#define PF2  (*((volatile unsigned long *)0x40025010))
#define PF3  (*((volatile unsigned long *)0x40025020))
#define PF4  (*((volatile unsigned long *)0x40025040))
  
#define PD0  (*((volatile unsigned long *)0x40007004))
#define PD1  (*((volatile unsigned long *)0x40007008))
#define PD2  (*((volatile unsigned long *)0x40007010))
#define PD3  (*((volatile unsigned long *)0x40007020))

void PortD_Init(void){ 
  SYSCTL_RCGCGPIO_R |= 0x08;       // activate port D
  while((SYSCTL_PRGPIO_R&0x08)==0){};      
  GPIO_PORTD_DIR_R |= 0x0F;    // make PE3-0 output heartbeats
  GPIO_PORTD_AFSEL_R &= ~0x0F;   // disable alt funct on PD3-0
  GPIO_PORTD_DEN_R |= 0x0F;     // enable digital I/O on PD3-0
  GPIO_PORTD_PCTL_R = ~0x0000FFFF;
  GPIO_PORTD_AMSEL_R &= ~0x0F;;      // disable analog functionality on PD
}

unsigned long NumCreated;   // number of foreground threads created
#define TIMESLICE 2*TIME_1MS  // thread switch time in system time units

static FATFS g_sFatFs;
FRESULT MountFresult;



void IdleTask(void){
	while(1){
		PD3 ^= 0x08;
	}
}

//******** Interpreter **************
// your intepreter from Lab 4 
// foreground thread, accepts input from UART port, outputs to UART port
// inputs:  none
// outputs: none
extern void Interpreter(void); 
// add the following commands, remove commands that do not make sense anymore
// 1) format 
// 2) directory 
// 3) print file
// 4) delete file
// execute   eFile_Init();  after periodic interrupts have started

//*******************lab 4 main **********
int main(void){        // lab 4 real main
  OS_Init();           // initialize, disable interrupts
  PortD_Init();  // user debugging profile
  
//********initialize communication channels
  OS_Fifo_Init(256);    

  NumCreated = 0 ;
// create initial foreground threads
  NumCreated += OS_AddThread(&Interpreter,128,2); 
  NumCreated += OS_AddThread(&IdleTask,128,7);  // runs when nothing useful to do
	
	MountFresult = f_mount(&g_sFatFs, "", 0);
  if(MountFresult){
    ST7735_DrawString(0, 0, "f_mount error", ST7735_Color565(0, 0, 255));
    while(1){};
  }
	Heap_Init();
 
  OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
  return 0;             // this never executes
}


