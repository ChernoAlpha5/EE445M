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
#include "can0.h"
#include "ff.h"
#include "wavfile.h"
#include "DAC.h"
#include "diskio.h"


uint8_t NumCreated = 0;
 
#define TIMESLICE 2*TIME_1MS  // thread switch time in system time units

#define NUMSOUNDS 3
const char *soundName[NUMSOUNDS] = {"lavender.wav", "vroom.wav"};
tWavFile Sound[NUMSOUNDS];
tWavHeader Header[NUMSOUNDS];
uint16_t CurSound = NUMSOUNDS;
  
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




FRESULT MountFresult;
static FATFS g_sFatFs;

#define BUFSIZE 8192
char SongBuffer[2][BUFSIZE];
uint16_t CurNote = 0;
uint16_t CurBuffer = 0;
uint32_t NextBufSize = 0;
uint32_t CurBufSize = 0;
Sema4Type BufferDone;


void OutputDac(void);

void StopSound(void){
	OS_RemovePeriodicThread(&OutputDac);
}


void OutputDac(void){
	DAC_Out((SongBuffer[CurBuffer][CurNote+1]+128)<<2);
	CurNote+=4;
	if(CurNote  > CurBufSize){
		StopSound();
	}
	if(CurNote==BUFSIZE){
		CurBuffer ^=0x01;
		CurBufSize = NextBufSize;
		CurNote=0;
		OS_Signal(&BufferDone);
	}
}



void PlaySound(uint8_t soundNum){
	if(CurSound != NUMSOUNDS){
		StopSound();
		WavClose(&Sound[CurSound]);
	}
	CurSound = soundNum;
	WavOpen(soundName[soundNum],&Sound[soundNum]);
	WavGetFormat(&Sound[soundNum],&Header[soundNum]);
	CurBufSize = WavRead(&Sound[soundNum],SongBuffer[0],BUFSIZE);	
	NextBufSize = WavRead(&Sound[soundNum],SongBuffer[1],BUFSIZE);	
	OS_AddPeriodicThread(&OutputDac, BUSCLK/Header[soundNum].ui32SampleRate, 0);
}

void LoadBuffer(void){
	OS_InitSemaphore(&BufferDone, 0);
	while(1){
		OS_Wait(&BufferDone);
		NextBufSize = WavRead(&Sound[CurSound],SongBuffer[CurBuffer^0x01],BUFSIZE);
	}
}

uint8_t SensorData[NUMMSGS*NUM_SENSORBOARDS][MSGLENGTH];
void PlayMusic(void){
	DAC_Init();
	MountFresult = f_mount(&g_sFatFs, "", 0);
	while(1){
		PlaySound(0);
		CAN0_GetMail(SensorData);
		
	}
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
	
	
//********initialize communication channels
	CAN0_Open();  

//*******attach background tasks***********
  OS_AddSW1Task(&SW1Push,2);    // PF4
  OS_AddSW2Task(&SW2Push,3);   // PF0
	
	NumCreated += OS_AddThread(&IdleTask,128,7);  // runs when nothing useful to do
	NumCreated += OS_AddThread(&LoadBuffer,128,1);  
	NumCreated += OS_AddThread(&PlayMusic,128,2);  

  OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
  return 0;             // this never executes
}

