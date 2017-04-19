//*****************************************************************************
//
// Robot.c - user programs, File system, stream data onto disk
//*****************************************************************************

// Jonathan W. Valvano 3/7/17, valvano@mail.utexas.edu
// EE445M/EE380L.6 
// You may use, edit, run or distribute this file 
// You are free to change the syntax/organization of this file to do Lab 4
// as long as the basic functionality is simular
// 1) runs on your Lab 2 or Lab 3
// 2) implements your own eFile.c system with no code pasted in from other sources
// 3) streams real-time data from robot onto disk
// 4) supports multiple file reads/writes
// 5) has an interpreter that demonstrates features
// 6) interactive with UART input, and switch input

// LED outputs to logic analyzer for OS profile 
// PF1 is preemptive thread switch
// PF2 is periodic task
// PF3 is SW1 task (touch PF4 button)

// Button inputs
// PF0 is SW2 task 
// PF4 is SW1 button input

// Analog inputs
// PE3 sequencer 3, channel 3, J8/PE0, sampling in DAS(), software start
// PE0 timer-triggered sampling, channel 0, J5/PE3, 50 Hz, processed by Producer
//******Sensor Board I/O*******************
// **********ST7735 TFT and SDC*******************
// ST7735
// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) connected to PB0
// Data/Command (pin 4) connected to PA6 (GPIO), high for data, low for command
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

// HC-SR04 Ultrasonic Range Finder 
// J9X  Trigger0 to PB7 output (10us pulse)
// J9X  Echo0    to PB6 T0CCP0
// J10X Trigger1 to PB5 output (10us pulse)
// J10X Echo1    to PB4 T1CCP0
// J11X Trigger2 to PB3 output (10us pulse)
// J11X Echo2    to PB2 T3CCP0
// J12X Trigger3 to PC5 output (10us pulse)
// J12X Echo3    to PF4 T2CCP0

// Ping))) Ultrasonic Range Finder 
// J9Y  Trigger/Echo0 to PB6 T0CCP0
// J10Y Trigger/Echo1 to PB4 T1CCP0
// J11Y Trigger/Echo2 to PB2 T3CCP0
// J12Y Trigger/Echo3 to PF4 T2CCP0

// IR distance sensors
// J5/A0/PE3
// J6/A1/PE2
// J7/A2/PE1
// J8/A3/PE0  

// ESP8266
// PB1 Reset
// PD6 Uart Rx <- Tx ESP8266
// PD7 Uart Tx -> Rx ESP8266

// Free pins (debugging)
// PF3, PF2, PF1 (color LED)
// PD3, PD2, PD1, PD0, PC4

#include <string.h> 
#include <stdio.h>
#include <stdint.h>
#include "OS.h"
#include "tm4c123gh6pm.h"
#include "ADC.h"
#include "can0.h"
#include "usonic.h"
#include "PWM.h"
#include "ST7735.h"
#include "Drive.h"

//*********Prototype for FFT in cr4_fft_64_stm32.s, STMicroelectronics
void cr4_fft_64_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);

#define PF0  (*((volatile unsigned long *)0x40025004))
#define PF1  (*((volatile unsigned long *)0x40025008))
#define PF2  (*((volatile unsigned long *)0x40025010))
#define PF3  (*((volatile unsigned long *)0x40025020))
#define PF4  (*((volatile unsigned long *)0x40025040))
  
#define PD0  (*((volatile unsigned long *)0x40007004))
#define PD1  (*((volatile unsigned long *)0x40007008))
#define PD2  (*((volatile unsigned long *)0x40007010))
#define PD3  (*((volatile unsigned long *)0x40007020))

uint8_t NumCreated = 0;

void PortD_Init(void){ 
  SYSCTL_RCGCGPIO_R |= 0x08;       // activate port D
  while((SYSCTL_PRGPIO_R&0x08)==0){};      
  GPIO_PORTD_DIR_R |= 0x0F;    // make PE3-0 output heartbeats
  GPIO_PORTD_AFSEL_R &= ~0x0F;   // disable alt funct on PD3-0
  GPIO_PORTD_DEN_R |= 0x0F;     // enable digital I/O on PD3-0
  GPIO_PORTD_PCTL_R = ~0x0000FFFF;
  GPIO_PORTD_AMSEL_R &= ~0x0F;;      // disable analog functionality on PD
}  
#define TIMESLICE 2*TIME_1MS  // thread switch time in system time units
  
//************SW1Push*************
// Called when SW1 Button pushed
// background threads execute once and return
void SW1Push(void){

}

//************SW2Push*************
// Called when SW2 Button pushed
// background threads execute once and return
void SW2Push(void){

}


extern Sema4Type adcDataReady;
extern uint16_t ADCValues[4];
char* Strings[8] = {"Data 0:", "Data 1:", "Data 2:", "Data 3:", "Data 4:", "Data 5:", "Data 6:", "Data 7:"};

uint32_t ADC2millimeter(uint32_t adcSample){
  if(adcSample<494) return 799; // maximum distance 80cm
  return (268130/(adcSample-159));  
}

void IRSend(void){
	uint8_t message[8];
	ADC0_SS2_4Channels_TimerTriggered_Init(BUSCLK/20);	//20 Hz data collection
	while(1){
		//GPIO_PORTF_DATA_R ^= 0x04;
		OS_Wait(&adcDataReady);
		// Message Format: MESSAGE_NUMBER, RESERVED, 6 bytes of ADC data
		message[0] = 0;
		message[1] = 42;
		message[2] = ((ADCValues[0]&0x00FF));
		message[3] = (((ADCValues[0]&0x0F00)>>8) + ((ADCValues[1]&0x0F)<<4));
		message[4] = ((ADCValues[1]&0x00FF0)>>4);
		message[5] = ((ADCValues[2]&0x00000FF));
		message[6] = ((ADCValues[2]&0x0F00)>>8) + ((ADCValues[3]&0x0F)<<4);
		message[7] = ((ADCValues[3]&0x0FF0)>>4);
		ST7735_Message(DISPLAY_NUMBER_1, 0, "ADC0: ", ADCValues[0]);
		ST7735_Message(DISPLAY_NUMBER_1, 1, "ADC1: ", ADCValues[1]);
		ST7735_Message(DISPLAY_NUMBER_1, 2, "ADC2: ", ADCValues[2]);
		ST7735_Message(DISPLAY_NUMBER_1, 3, "ADC3: ", ADCValues[3]);
		CAN0_SendData(message);
	}
}
extern Sema4Type usonicDataReady;
extern uint32_t USValues[3];
void USONICSend(void){
	uint8_t message[8];
	USONIC_Init();
	OS_AddPeriodicThread(&USONIC_StartHCSR04, BUSCLK/20, 0); //20 Hz data Collection
	while(1){
		//PF1 ^= 0x02;
		OS_Wait(&usonicDataReady);
		// Message Format: MESSAGE_NUMBER, RESERVED, 6 bytes of ADC data
		message[0] = 0;
		message[1] = 42;
		message[2] = ((USValues[0]&0x00FF));
		message[3] = (((USValues[0]&0x0F00)>>8) + ((USValues[1]&0x0F)<<4));
		message[4] = ((USValues[1]&0x00FF0)>>4);
		message[5] = ((USValues[2]&0x00000FF));
		message[6] = ((USValues[2]&0x0F00)>>8);
		message[7] = 0;
		ST7735_Message(DISPLAY_NUMBER_2, 0, "US0: ", USValues[0]);
		ST7735_Message(DISPLAY_NUMBER_2, 1, "US1: ", USValues[1]);
		ST7735_Message(DISPLAY_NUMBER_2, 2, "US2: ", USValues[2]);
	  CAN0_SendData(message);
	}
}

void MotorController(void){
	
}

//******** IdleTask  *************** 
// foreground thread, runs when no other work needed
// never blocks, never sleeps, never dies
// inputs:  none
// outputs: none
unsigned long Idlecount=0;
void IdleTask(void){ 
  while(1) { 
    Idlecount++;        // debugging 
  }
}


// NOTE: Define MOTOR_BOARD, SENSOR_BOARD_1, or SENSOR_BOARD_2 in the build options 
int main(void){        // lab 4 real main
	OS_Init();           // initialize, disable interrupts
  PortD_Init();  // user debugging profile
	
	
//********initialize communication channels
	CAN0_Open();  

//*******attach background tasks***********
  OS_AddSW1Task(&SW1Push,2);    // PF4
  OS_AddSW2Task(&SW2Push,3);   // PF0
	
	NumCreated += OS_AddThread(&IdleTask,128,7);  // runs when nothing useful to do
	
	#ifdef MOTOR_BOARD
		NumCreated += OS_AddThread(&MotorController, 128, 1);
	#else
		Output_Init();
		NumCreated += OS_AddThread(&USONICSend, 128, 1);
		NumCreated += OS_AddThread(&IRSend, 128, 1);	
	#endif

  OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
  return 0;             // this never executes
}

