// ST7735TestMain.c
// Runs on LM4F120/TM4C123
// Test the functions in ST7735.c by printing basic
// patterns to the LCD.
//    16-bit color, 128 wide by 160 high LCD
// Daniel Valvano
// March 30, 2015

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014

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

// hardware connections
// **********ST7735 TFT and SDC*******************
// ST7735
// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) unconnected
// Data/Command (pin 4) connected to PA6 (GPIO), high for data, low for command
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

// **********wide.hk ST7735R with ADXL345 accelerometer *******************
// Silkscreen Label (SDC side up; LCD side down) - Connection
// VCC  - +3.3 V
// GND  - Ground
// !SCL - PA2 Sclk SPI clock from microcontroller to TFT or SDC
// !SDA - PA5 MOSI SPI data from microcontroller to TFT or SDC
// DC   - PA6 TFT data/command
// RES  - PA7 TFT reset
// CS   - PA3 TFT_CS, active low to enable TFT
// *CS  - (NC) SDC_CS, active low to enable SDC
// MISO - (NC) MISO SPI data from SDC to microcontroller
// SDA  – (NC) I2C data for ADXL345 accelerometer
// SCL  – (NC) I2C clock for ADXL345 accelerometer
// SDO  – (NC) I2C alternate address for ADXL345 accelerometer
// Backlight + - Light, backlight connected to +3.3 V

// **********wide.hk ST7735R with ADXL335 accelerometer *******************
// Silkscreen Label (SDC side up; LCD side down) - Connection
// VCC  - +3.3 V
// GND  - Ground
// !SCL - PA2 Sclk SPI clock from microcontroller to TFT or SDC
// !SDA - PA5 MOSI SPI data from microcontroller to TFT or SDC
// DC   - PA6 TFT data/command
// RES  - PA7 TFT reset
// CS   - PA3 TFT_CS, active low to enable TFT
// *CS  - (NC) SDC_CS, active low to enable SDC
// MISO - (NC) MISO SPI data from SDC to microcontroller
// X– (NC) analog input X-axis from ADXL335 accelerometer
// Y– (NC) analog input Y-axis from ADXL335 accelerometer
// Z– (NC) analog input Z-axis from ADXL335 accelerometer
// Backlight + - Light, backlight connected to +3.3 V

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "ST7735.h"
#include "PLL.h"
#include "../inc/tm4c123gh6pm.h"
#include "ADC.h"
#include "UART.h"
#include "OS.h"
#include <string.h>		//is this bad???
#define NUMCOMMANDS 15

#define PF1             (*((volatile uint32_t *)0x40025008))
#define PF2   (*((volatile uint32_t *)0x40025010))
	
void test1(void);
void test2(void);
void dummy(void);

char* argArray[5];// = {NULL,NULL,NULL};
	int ADCbuffer[20];
void UserTask(void){
	test1();
}
void UserTask2(void){
	test2();
}

inpCommand inpCommands[NUMCOMMANDS] = {
	{"ADCread", &UserTask},
	{"ADCcollect", &UserTask2}
};

void DelayWait10ms(uint32_t n);
void parseCmd(char* str);
// test image
// [blue] [green]
// [red ] [white]
const uint16_t Test[] = {
  0x001F, 0xFFFF, 0xF800, 0x07E0
};


int main(void){  // main 2

	char string[20];  // global to assist in debugging
  PLL_Init(Bus80MHz);                  // set system clock to 80 MHz
	UART_Init();              // initialize UART
  ST7735_InitR(INITR_REDTAB);
  ST7735_FillScreen(0x0);            		// set screen to white
	ADC_Collect(9, 100, ADCbuffer, 7);  	//100 hz sampling rate. FIX SAMPLING RATE
	int cmdFound = 0;
	OS_AddPeriodicThread(&dummy, 70, 2);
		 
	while (ADC_Status() != 0){}						//wait for collect to finish before printing to screen
	
	//DisableInterrupts();	//REMOVE LATER!!!
	
	ST7735_Message (0, 0, "Reading1", ADCbuffer[0]);
	ST7735_Message (0, 1, "Reading2", ADCbuffer[1]);
	ST7735_Message (0, 2, "Reading3", ADCbuffer[2]);
	ST7735_Message (0, 3, "Reading4", ADCbuffer[3]);
	ST7735_Message (0, 4, "Reading5", ADCbuffer[4]);
	ST7735_Message (0, 5, "Reading6", ADCbuffer[5]);
	ST7735_Message (0, 6, "Reading7", ADCbuffer[6]);
	ST7735_Message (0, 7, "Reading8", ADCbuffer[7]);
	ST7735_Message (1, 0, "Reading9", ADCbuffer[8]);
	ST7735_Message (1, 1, "Reading10", ADCbuffer[9]);
	
	ADC_Open(9);	//set ADC to channel 9. ch 9 corresponds to PE4. REFER TO TABLE 2.4 FOR CHANNEL MAPPINGS
	uint16_t ADCval = ADC_In();
	ST7735_Message (1, 7, "ADC val", ADCval);
	
	while(1){
    UART_OutString("$ ");  //prompt
    UART_InString(string,19); OutCRLF();
    /*UART_OutString(" OutString="); UART_OutString(string); OutCRLF();

    UART_OutString("InUDec: ");  n=UART_InUDec();
    UART_OutString(" OutUDec="); UART_OutUDec(n); OutCRLF();

    UART_OutString("InUHex: ");  n=UART_InUHex();
    UART_OutString(" OutUHex="); UART_OutUHex(n); OutCRLF(); */
		parseCmd(string);
	  for (int i = 0; i < NUMCOMMANDS; i++){
			//UART_OutUDec(i);
			if (strcmp(argArray[0], inpCommands[i].cmdString) == 0){
				inpCommands[i].cmdPtr();  // run function specified by pointer
				cmdFound = 1;
				break;
			}
		}
		if (!cmdFound){
				UART_OutString("Invalid command");
				/*UART_OutString(string);
				UART_OutUDec(strcmp(string, inpCommands[i].cmdString));*/
				OutCRLF();
		}
		cmdFound = 0;
  }
}

// private function draws a color band on the screen
void static drawthecolors(uint8_t red, uint8_t green, uint8_t blue){
  static uint16_t y = 0;
  ST7735_DrawFastHLine(0, y, ST7735_TFTWIDTH, ST7735_Color565(red, green, blue));
  y = y + 1;
  if(y >= ST7735_TFTHEIGHT){
     y = 0;
  }
  DelayWait10ms(1);
}
int main3(void){ // main3
  uint8_t red, green, blue;
  PLL_Init(Bus80MHz);                  // set system clock to 80 MHz
  // test DrawChar() and DrawCharS()
  ST7735_InitR(INITR_REDTAB);

  // test display with a colorful demo
  red = 255;
  green = 0;
  blue = 0;
  while(1){
    // transition from red to yellow by increasing green
    for(green=0; green<255; green=green+1){
      drawthecolors(red, green, blue);
    }
    // transition from yellow to green by decreasing red
    for(red=255; red>0; red=red-1){
      drawthecolors(red, green, blue);
    }
    // transition from green to light blue by increasing blue
    for(blue=0; blue<255; blue=blue+1){
      drawthecolors(red, green, blue);
    }
    // transition from light blue to true blue by decreasing green
    for(green=255; green>0; green=green-1){
      drawthecolors(red, green, blue);
    }
    // transition from true blue to pink by increasing red
    for(red=0; red<255; red=red+1){
      drawthecolors(red, green, blue);
    }
    // transition from pink to red by decreasing blue
    for(blue=255; blue>0; blue=blue-1){
      drawthecolors(red, green, blue);
    }
  }
}


// Make PF2 an output, enable digital I/O, ensure alt. functions off
void SSR_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x20;        // 1) activate clock for Port F
  while((SYSCTL_PRGPIO_R&0x20)==0){}; // allow time for clock to start
                                    // 2) no need to unlock PF2
  GPIO_PORTF_PCTL_R &= ~0x00000F00; // 3) regular GPIO
  GPIO_PORTF_AMSEL_R &= ~0x04;      // 4) disable analog function on PF2
  GPIO_PORTF_DIR_R |= 0x04;         // 5) set direction to output
  GPIO_PORTF_AFSEL_R &= ~0x04;      // 6) regular port function
  GPIO_PORTF_DEN_R |= 0x04;         // 7) enable digital port
}


void Delay1ms(uint32_t n);
int main4(void){
  SSR_Init();
  while(1){
    Delay1ms(10);
    PF2 ^= 0x04;
  }
}
int main5(void){
  SSR_Init();
  while(1){
    DelayWait10ms(1000);
    PF2 ^= 0x04;
  }
}

// Subroutine to wait 10 msec
// Inputs: None
// Outputs: None
// Notes: ...
void DelayWait10ms(uint32_t n){uint32_t volatile time;
  while(n){
    time = 727240*2/91;  // 10msec
    while(time){
      time--;
    }
    n--;
  }
}

void test1(void){
	//OutCRLF();
	UART_OutString("ADC reading: ");
	UART_OutUDec(ADC_In());
	OutCRLF();
}

void test2(void){
	//2 arguments: sampling rate, number of samples
	UART_OutString("ADC reading: ");
	int numArgs[5];
	int numSamples; int sampleRate;
	
	while (ADC_Status() != 0){}						//wait for collect to finish before printing to screen
	for (int i = 1; i < 3; i++){
		numArgs[i] = atoi(argArray[i]); //convert char to int
	}
	sampleRate = numArgs[1];
	numSamples = numArgs[2];
	ADC_Collect(9,sampleRate, ADCbuffer, numSamples );
	for (int x = 0; x < numSamples; x++){
		UART_OutUDec(ADCbuffer[x]);  //array
		UART_OutString(", ");
	}
	OutCRLF();
}

void parseCmd(char* str){
   const char s[2] = " ";
   char *token;
   int i = 0;
	
		for (int a = 0; a < 5; a++){  //clear array 
			argArray[i] = NULL;
		}
   /* get the first token */
   token = strtok(str, s);
   
   /* walk through other tokens */
   while( token != NULL ) 
   {
      argArray[i] = token;
      token = strtok(NULL, s);
		 i++;
   }
   
 }
void dummy(void) {
	
};
