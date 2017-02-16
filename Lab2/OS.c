#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "UART.H"
#include "OS.h"
#include "SysTick.h"
#include "PLL.h"

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
																						
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
long StartCritical(void);
void EndCritical(long primask);
void StartOS(void);

#define MAXNUMTHREADS 10
#define STACKSIZE 100
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

uint32_t Counter;
void(*PeriodicTask)(void);

void Timer4A_Handler(){
	//PF1 ^= 0x02;
	//PF1 ^= 0x02;
	TIMER4_ICR_R |= 0x01;
	Counter += 1;
	PeriodicTask();
	//PF1 ^= 0x02;
}
uint32_t osCounter = 0;
void Timer5A_Handler(void){
	//PF1 = 0x2;							// used for measuring ISR time
  TIMER5_ICR_R = TIMER_ICR_TATOCINT;// acknowledge timer0A timeout
	osCounter++; 												//increment global counter
  (*PeriodicTask)();                // execute user task
	//PF1 = 0x0;
}
void OS_ClearPeriodicTime(void){
	//TIMER4_TAV_R = TIMER4_TAILR_R;
	//TIMER4_TAV_R = 0;
	Counter = 0;
}
uint32_t OS_ReadPeriodicTime(void){
	return Counter;
}

void OS_KillTask(void){
	TIMER4_CTL_R &= ~0x00000001;
}

// ******** OS_Init ValvanoWare ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: systick, 50 MHz PLL
// input:  none
// output: none
/*void OS_Init(void){
  OS_DisableInterrupts();
  PLL_Init(Bus50MHz);         // set processor clock to 50 MHz
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0x00FFFFFF)|0xE0000000; // priority 7
}*/

void Andrew(void){
	while(1);
}

// ******** OS_Init ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers 
// input:  none
// output: none
void OS_Init(void){
	DisableInterrupts();
  PLL_Init(Bus50MHz);         // set processor clock to 50 MHz
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0x00FFFFFF)|0xC0000000; // priority 6 SysTick
	NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0xFF00FFFF)|0x00E00000; // priority 7 PendSV
	CurrentID = 0;
	RunPt = &tcbs[0];
	RunPt->next = RunPt;
	OS_AddThread(Andrew, 128, 0);
	RunPt->id = -1; 
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
	long status;
  status = StartCritical();
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
		return 0;
	}
	unusedThread = &tcbs[numThread];
	
	unusedThread->next = RunPt->next; // Insert thread into linked list
	RunPt->next = unusedThread;
	unusedThread->prev = RunPt;
	unusedThread->next->prev = unusedThread; //set prev for thread after current to current
	
	unusedThread->id = CurrentID;	//Current ID is incremented forever for different IDs
	SetInitialStack(numThread);		//initialize stack
	Stacks[numThread][STACKSIZE-2] = (int32_t)(task); //  set PC for Task
	CurrentID+=1;
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
int OS_AddPeriodicThread(void(*task)(void),unsigned long periodMs, unsigned long priority){
	DisableInterrupts();
	Counter = 0;
	SYSCTL_RCGCTIMER_R |= 0x10;   //  activate TIMER4
	PeriodicTask = task;          // user function
	TIMER4_CTL_R = 0x00000000;    // disable timer2A during setup
  TIMER4_CFG_R = 0x00000000;             // configure for 32-bit timer mode
  TIMER4_TAMR_R = 0x00000002;   // configure for periodic mode, default down-count settings
  TIMER4_TAPR_R = 0;            // prescale value for trigger
  TIMER4_TAILR_R = (periodMs*(BUSCLK/1000))-1;    // start value for trigger
	TIMER4_ICR_R = 0x00000001;    // 6) clear TIMER4A timeout flag
  TIMER4_IMR_R = 0x00000001;    // enable timeout interrupts
	NVIC_PRI17_R = (NVIC_PRI17_R&0xFF00FFFF)| (priority << 21); //set pririty
  NVIC_EN2_R = 1<<6;              // enable interrupt 70 in NVIC
	TIMER4_CTL_R |= 0x00000001;   // enable timer2A 32-b, periodic, no interrupts
	EnableInterrupts();
	return 0; 
}

// ***************** Timer5_Init ****************
// Activate Timer4 interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in units (1/clockfreq)
// Outputs: none
int OS_AddPeriodicThread2(void(*task)(void), unsigned long period,unsigned long priority){
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
	RunPt->id = 0; //set id to dead
	RunPt->prev->next = RunPt->next;
	RunPt->next->prev = RunPt->prev;
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
	return 0;
}

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long OS_Fifo_Get(void){
	return 0;
}

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void){
	return 0;
}

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void){
	
}

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(unsigned long data){
	
}

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
unsigned long OS_MailBox_Recv(void){
	return 0;
}

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
unsigned long OS_Time(void){
	return 0;
}

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
unsigned long OS_TimeDifference(unsigned long start, unsigned long stop){
	return 0;
}

// ******** OS_ClearMsTime ************
// sets the system time to zero (from Lab 1)
// Inputs:  none
// Outputs: none
// You are free to change how this works
void OS_ClearMsTime(void){
	
}

// ******** OS_MsTime ************
// reads the current time in msec (from Lab 1)
// Inputs:  none
// Outputs: time in ms units
// You are free to select the time resolution for this function
// It is ok to make the resolution to match the first call to OS_AddPeriodicThread
unsigned long OS_MsTime(void){
	return 0;
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
	while(NextPt->id <= 0 || NextPt->sleep){
		NextPt = NextPt->next;
	}
}
