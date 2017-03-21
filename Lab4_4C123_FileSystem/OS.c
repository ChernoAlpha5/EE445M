// *************os.c**************
// EE445M/EE380L.6 Lab 2, Lab 3 solution
// high level OS functions
// 
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano 3/9/17, valvano@mail.utexas.edu


#include "inc/hw_types.h"
#include "PLL.h"
#include "inc/tm4c123gh6pm.h"
#include "UART2.h"
#include "os.h"
#include "adc.h"
#include "ST7735.h"
#include <stdint.h>

#define TIMER_CFG_16_BIT        0x00000004  // 16-bit timer configuration,
                                            // function is controlled by bits
                                            // 1:0 of GPTMTAMR and GPTMTBMR
#define TIMER_TAMR_TACDIR       0x00000010  // GPTM Timer A Count Direction
#define TIMER_TAMR_TAMR_PERIOD  0x00000002  // Periodic Timer mode
#define TIMER_CTL_TAOTE         0x00000020  // GPTM TimerA Output Trigger
                                            // Enable
#define TIMER_CTL_TAEN          0x00000001  // GPTM TimerA Enable
#define TIMER_IMR_TATOIM        0x00000001  // GPTM TimerA Time-Out Interrupt
                                            // Mask
#define TIMER_TAILR_TAILRL_M    0x0000FFFF  // GPTM TimerA Interval Load
                                            // Register Low
																						
																						
#define PF2             (*((volatile uint32_t *)0x40025010))
#define PF1             (*((volatile uint32_t *)0x40025008))																						
#define BUSCLK 50000000

#define MAXFIFOSIZE 32
																						
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
long StartCritical(void);
void EndCritical(long primask);
void StartOS(void);

uint8_t NumThreads;
#define MAXNUMTHREADS 30
#define STACKSIZE 200
struct tcb{
	int32_t *sp;
	struct tcb *next;
	struct tcb *prev;
	int16_t id;
	uint16_t sleep;
	uint8_t priority;
	uint8_t blocked;
};
typedef struct tcb tcbType;
tcbType tcbs[MAXNUMTHREADS];
tcbType *RunPt;
tcbType *NextPt;
int32_t Stacks[MAXNUMTHREADS][STACKSIZE];
int16_t CurrentID;
uint32_t OSMsCount;

uint32_t PeriodicTaskCounter;
void(*PeriodicTask)(void);
void(*SW1Task)(void);
void(*SW2Task)(void);



void OS_ClearPeriodicTime(void){
	//TIMER4_TAV_R = TIMER4_TAILR_R;
	//TIMER4_TAV_R = 0;
	PeriodicTaskCounter = 0;
}
uint32_t OS_ReadPeriodicTime(void){
	return PeriodicTaskCounter;
}

void OS_KillTask(void){
	TIMER4_CTL_R &= ~0x00000001;
}

long AndrewTriggered;
void PortF_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x20; // activate port F
  while((SYSCTL_PRGPIO_R&0x20)==0){}; // allow time for clock to start 
	AndrewTriggered = 0;
  GPIO_PORTF_DIR_R &= ~0x10;    // (c) make PF4 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x10;  //     disable alt funct on PF4
  GPIO_PORTF_DEN_R |= 0x10;     //     enable digital I/O on PF4
  GPIO_PORTF_PCTL_R &= ~0x000F0000; //  configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R &= ~0x10;  //    disable analog functionality on PF4
  GPIO_PORTF_PUR_R |= 0x10;     //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x10;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x10;    //     PF4 is not both edges
  GPIO_PORTF_IEV_R &= ~0x10;    //     PF4 falling edge event
  GPIO_PORTF_ICR_R = 0x10;      // (e) clear flag4
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00400000; // (g) priority 2
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
}

void GPIOPortF_Handler(void){
  GPIO_PORTF_ICR_R = 0x10;      // acknowledge flag4
	SW1Task();
	AndrewTriggered+=1;
}

void Timer4A_Init(void){ //Periodic Task 1
	SYSCTL_RCGCTIMER_R |= 0x10;   //  activate TIMER4
//	long Andrew = 0;
	TIMER4_CTL_R = 0x00000000;    // disable timer4A during setup
  TIMER4_CFG_R = 0x00000000;             // configure for 32-bit timer mode
  TIMER4_TAMR_R = 0x00000002;   // configure for periodic mode, default down-count settings
  TIMER4_TAPR_R = 0;            // prescale value for trigger
	TIMER4_ICR_R = 0x00000001;    // 6) clear TIMER4A timeout flag
  TIMER4_IMR_R = 0x00000001;    // enable timeout interrupts
}

void Timer4A_Handler(){
	//PF1 ^= 0x02;
	//PF1 ^= 0x02;
	TIMER4_ICR_R |= 0x01;
	PeriodicTaskCounter += 1;
	PeriodicTask();
	//PF1 ^= 0x02;
}

void Timer5A_Init(void){ //Sleep
	SYSCTL_RCGCTIMER_R |= 0x20;   //  activate TIMER5
	OSMsCount = 0;
	TIMER5_CTL_R = 0x00000000;    // disable timer5A during setup
  TIMER5_CFG_R = 0x00000000;             // configure for 32-bit timer mode
  TIMER5_TAMR_R = 0x00000002;   // configure for periodic mode, default down-count settings
  TIMER5_TAPR_R = 0;            // prescale value for trigger
	TIMER5_ICR_R = 0x00000001;    // 6) clear TIMER4A timeout flag
	TIMER5_TAILR_R = (1*(BUSCLK/1000))-1;    // start value for trigger
	NVIC_PRI23_R = (NVIC_PRI23_R&0xFFFFFF00)|0x00000060; // 8) priority 3
  NVIC_EN2_R = 0x10000000;        // 9) enable interrupt 19 in NVIC
  TIMER5_IMR_R = 0x00000001;    // enable timeout interrupts
	TIMER5_CTL_R |= 0x00000001;   // enable timer5A 32-b, periodic, no interrupts
}

void Timer5A_Handler(){
	//PF1 ^= 0x02;
	//PF1 ^= 0x02;
	TIMER5_ICR_R |= 0x01;
	OSMsCount += 1;
	if(NumThreads){
		tcbType *firstPt = RunPt;
		tcbType *currentPt = RunPt;
		for(int i=0; i<MAXNUMTHREADS; i++){
			if(currentPt->sleep){
				currentPt->sleep--;
			}
			currentPt = currentPt->next;
			if(currentPt == firstPt){
				break;
			}
		}
	}
	//PF1 ^= 0x02;
}

void WTimer5A_Init(void){
	SYSCTL_RCGCWTIMER_R |= 0x20;   //  activate WTIMER5
//	long Andrew = 0;
	WTIMER5_CTL_R = 0x00000000;    // disable Wtimer5A during setup
  WTIMER5_CFG_R = 0x00000000;             // configure for 64-bit timer mode
  WTIMER5_TAMR_R = 0x00000002;   // configure for periodic mode, default down-count settings
  WTIMER5_TAPR_R = 0;            // prescale value for trigger
	WTIMER5_ICR_R = 0x00000001;    // 6) clear WTIMER5A timeout flag
	WTIMER5_TAILR_R = 0xFFFFFFFF;    // start value for trigger
	NVIC_PRI26_R = (NVIC_PRI26_R&0xFFFFFF00)|0x00000020; // 8) priority 1
  NVIC_EN3_R = 0x00000100;        // 9) enable interrupt 19 in NVIC
  WTIMER5_IMR_R = 0x00000001;    // enable timeout interrupts
	WTIMER5_CTL_R |= 0x00000001;   // enable Wtimer5A 64-b, periodic, no interrupts
}

int WideTimer5Count = 0;
void WideTimer5A_Handler(void){
	WTIMER5_ICR_R |= 0x01;
	WideTimer5Count+=1; 
}


// ******** OS_Init ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers 
// input:  none
// output: none
void OS_Init(void){
	DisableInterrupts();
  PLL_Init(Bus50MHz);         // set processor clock to 50 MHz
	Output_Init();
	Timer4A_Init();
	Timer5A_Init();
	WTimer5A_Init();
	PortF_Init();
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0x00FFFFFF)|0xC0000000; // priority 6 SysTick
	NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0xFF00FFFF)|0x00E00000; // priority 7 PendSV
	CurrentID = 1;
	NumThreads = 0;
}

// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, long value){
	semaPt->Value = value;
}

// ******** OS_Wait ************
// decrement semaphore 
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt) {
  DisableInterrupts();
  while((semaPt->Value) <= 0){
    EnableInterrupts();
    DisableInterrupts();
  }
  (semaPt->Value) = (semaPt->Value) - 1;
  EnableInterrupts();
}


// ******** OS_Signal ************
// increment semaphore 
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt) {
  long status;
  status = StartCritical();
  (semaPt->Value) = (semaPt->Value) + 1;
  EndCritical(status);
}


// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
void OS_bWait(Sema4Type *semaPt) {
  DisableInterrupts();
  while((semaPt->Value) == 0){
    EnableInterrupts();
    DisableInterrupts();
  }
  (semaPt->Value) = 0;
  EnableInterrupts();
}

// ******** OS_bSignal ************
// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt){
	long status = StartCritical();
  (semaPt->Value) = 1;
  EndCritical(status);
}
		

void SetInitialStack(int i){
  tcbs[i].sp = &Stacks[i][STACKSIZE-16]; // thread stack pointer
  Stacks[i][STACKSIZE-1] = 0x01000000;   // thumb bit
  Stacks[i][STACKSIZE-3] = 0x14141414;   // R14
  Stacks[i][STACKSIZE-4] = 0x12121212;   // R12
  Stacks[i][STACKSIZE-5] = 0x03030303;   // R3
  Stacks[i][STACKSIZE-6] = 0x02020202;   // R2
  Stacks[i][STACKSIZE-7] = 0x01010101;   // R1
  Stacks[i][STACKSIZE-8] = 0x00000000;   // R0
  Stacks[i][STACKSIZE-9] = 0x11111111;   // R11
  Stacks[i][STACKSIZE-10] = 0x10101010;  // R10
  Stacks[i][STACKSIZE-11] = 0x09090909;  // R9
  Stacks[i][STACKSIZE-12] = 0x08080808;  // R8
  Stacks[i][STACKSIZE-13] = 0x07070707;  // R7
  Stacks[i][STACKSIZE-14] = 0x06060606;  // R6
  Stacks[i][STACKSIZE-15] = 0x05050505;  // R5
  Stacks[i][STACKSIZE-16] = 0x04040404;  // R4
}


//******** OS_AddThread *************** 
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
int OS_AddThread(void(*task)(void), unsigned long stackSize, unsigned long priority){
	long status = StartCritical();
	tcbType *unusedThread;
	int numThread;
	for(numThread = 0; numThread < MAXNUMTHREADS; numThread++){
		if(tcbs[numThread].id == 0){
			break;
		}
	}
	if(numThread == MAXNUMTHREADS){
		EndCritical(status);
		return 0;
	}
	unusedThread = &tcbs[numThread];
	
	if(NumThreads == 0){
		unusedThread->next = unusedThread;
		unusedThread->prev = unusedThread;
		RunPt = unusedThread;
	}
	else{
		unusedThread->next = RunPt->next; // Insert thread into linked list
		RunPt->next = unusedThread;
		unusedThread->prev = RunPt;
		unusedThread->next->prev = unusedThread; //set prev for thread after current to current
	}
	
	unusedThread->id = CurrentID;	//Current ID is incremented forever for different IDs
	unusedThread->sleep = 0;
	SetInitialStack(numThread);		//initialize stack
	Stacks[numThread][STACKSIZE-2] = (int32_t)(task); //  set PC for Task
	CurrentID+=1;
	NumThreads+=1;
	EndCritical(status);
  return 1;
}

//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
unsigned long OS_Id(void){
	return RunPt->id;
}

//******** OS_AddPeriodicThread *************** 
// add a background periodic task
// typically this function receives the highest priority
// Inputs: pointer to a void/void background function
//         period given in system time units (12.5ns)
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// You are free to select the time resolution for this function
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In lab 2, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddPeriodicThread(void(*task)(void),unsigned long period, unsigned long priority){
	DisableInterrupts();
	PeriodicTaskCounter = 0;
	PeriodicTask = task;          // user function
	TIMER4_TAILR_R = (period)-1;    // start value for trigger
	NVIC_PRI17_R = (NVIC_PRI17_R&0xFF00FFFF)| (priority << 21); //set priority
  NVIC_EN2_R = 1<<6;              // enable interrupt 70 in NVIC
	TIMER4_CTL_R |= 0x00000001;   // enable timer2A 32-b, periodic, no interrupts
	EnableInterrupts();
	return 0; 
}

//******** OS_AddSW1Task *************** 
// add a background task to run whenever the SW1 (PF4) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In labs 2 and 3, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW1Task(void(*task)(void), unsigned long priority){
	SW1Task = task;
	GPIO_PORTF_IM_R |= 0x10;      // (f) arm interrupt on PF4
	return 0;
}

//******** OS_AddSW2Task *************** 
// add a background task to run whenever the SW2 (PF0) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed user task will run to completion and return
// This task can not spin block loop sleep or kill
// This task can call issue OS_Signal, it can call OS_AddThread
// This task does not have a Thread ID
// In lab 2, this function can be ignored
// In lab 3, this command will be called will be called 0 or 1 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW2Task(void(*task)(void), unsigned long priority){
	SW2Task = task;
	return 0;
}

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(unsigned long sleepTime){
	RunPt->sleep = sleepTime;
	OS_Suspend();
}

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void){
	long status = StartCritical();
	RunPt->id = 0; //set id to dead
	NumThreads--;
	if(NumThreads > 0){
		RunPt->prev->next = RunPt->next;	//if no threads left there is no need to change pointers
		RunPt->next->prev = RunPt->prev;
	}
	EndCritical(status);
	OS_Suspend();		//send to graveyard	
}

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking 
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void){
	NVIC_ST_CURRENT_R = 0;
	NVIC_INT_CTRL_R = 0x04000000; //Trigger SysTick
}


int Fifo[MAXFIFOSIZE];
int PutPt;
int GetPt;
int FifoSize;
int FifoNumElements;
Sema4Type DataRoomLeft;
Sema4Type DataAvailable;
Sema4Type FifoMutex;
// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(unsigned long size){
	long sr = StartCritical();      // make atomic
	OS_InitSemaphore(&DataRoomLeft, size);
	OS_InitSemaphore(&DataAvailable, 0);
	OS_InitSemaphore(&FifoMutex, 1);
  PutPt = 0; // Empty
	GetPt = 0; // Empty
	FifoSize = size;
	FifoNumElements = 0;
  EndCritical(sr);
}

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(unsigned long data){
	/*OS_Wait(&DataRoomLeft);
	OS_bWait(&FifoMutex);*/
	if(FifoNumElements == MAXFIFOSIZE){
		return 0;
	}
	DisableInterrupts();
	Fifo[PutPt] = data;
	PutPt = (PutPt + 1) % MAXFIFOSIZE;
	FifoNumElements++;
	EnableInterrupts();
	/*OS_bSignal(&FifoMutex);
	OS_Signal(&DataAvailable);*/
	return 1;
}

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long OS_Fifo_Get(void){
	//OS_Wait(&DataAvailable);
	//OS_bWait(&FifoMutex);
	while(FifoNumElements==0);
	long sr = StartCritical();
	unsigned long data = Fifo[GetPt];
	GetPt = (GetPt + 1) % MAXFIFOSIZE;
	FifoNumElements--;
	EndCritical(sr);
	//OS_bSignal(&FifoMutex);
	//OS_Signal(&DataRoomLeft);
	return data;
}

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void){
	return DataAvailable.Value;
}

Sema4Type BoxFree;
Sema4Type DataValid;
unsigned long MailBox;

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void){
	OS_InitSemaphore(&BoxFree, 1);
	OS_InitSemaphore(&DataValid, 0);
}

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(unsigned long data){
	OS_bWait(&BoxFree);
	MailBox = data;
	OS_bSignal(&DataValid);
}

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
unsigned long OS_MailBox_Recv(void){
	OS_bWait(&DataValid);
	unsigned long data = MailBox;
	OS_bSignal(&BoxFree);
	return data;
}

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
unsigned long OS_Time(void){
	return WTIMER5_TAR_R;
}

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
unsigned long OS_TimeDifference(unsigned long start, unsigned long stop){
	long difference = start - stop;
	if(difference < 0){
		difference += 0x0FFFFFFFF;
	}
	return difference;
}

// ******** OS_ClearMsTime ************
// sets the system time to zero (from Lab 1)
// Inputs:  none
// Outputs: none
// You are free to change how this works
void OS_ClearMsTime(void){
	OSMsCount = 0;
}

// ******** OS_MsTime ************
// reads the current time in msec (from Lab 1)
// Inputs:  none
// Outputs: time in ms units
// You are free to select the time resolution for this function
// It is ok to make the resolution to match the first call to OS_AddPeriodicThread
unsigned long OS_MsTime(void){
	return OSMsCount;
}



//******** OS_Launch *************** 
// start the scheduler, enable interrupts
// Inputs: number of 12.5ns clock cycles for each time slice
//         you may select the units of this parameter
// Outputs: none (does not return)
// In Lab 2, you can ignore the theTimeSlice field
// In Lab 3, you should implement the user-defined TimeSlice field
// It is ok to limit the range of theTimeSlice to match the 24-bit SysTick
void OS_Launch(unsigned long theTimeSlice){
	NVIC_ST_RELOAD_R = theTimeSlice - 1; // reload value
  NVIC_ST_CTRL_R = 0x00000007; // enable, core clock and interrupt arm
	StartOS();                   // start on the first task
}

void OS_SelectNextThread(void){
	NextPt = RunPt->next;	//switch threads using round-robin, avoid dead/uninitialized threads and sleeping threads
	while(NextPt->id == 0 || NextPt->sleep){
		NextPt = NextPt->next;
	}
}
//fuck new lines
