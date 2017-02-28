Lab 3 prep
1) //1st method: help?
//2nd method: 

void WTimer0A_Init(void){
	SYSCTL_RCGCWTIMER_R |= 0x01;   //  activate WTIMER0
	long Andrew = 0; 
	WTIMER0_CTL_R = 0x00000000;    // disable Wtimer0A during setup
 	WTIMER0_CFG_R = 0x00000000;    // configure for 64-bit timer mode
  	WTIMER0_TAMR_R = 0x00000002;   // configure for periodic mode, default down-count settings
  	WTIMER0_TAPR_R = 0;            // prescale value for trigger
	WTIMER0_ICR_R = 0x00000001;    // 6) clear WTIMER5A timeout flag
	WTIMER0_TAILR_R = 0xFFFFFFFF;    // start value for trigger
  	WTIME0_IMR_R = 0x00000001;    // enable timeout interrupts
}

OS_AddPeriodicThread(void(*task)(void),unsigned long period, unsigned long priority){
	DisableInterrupts();
	PeriodicTaskCounter = 0;
	
	if (calls < 2){	//calls is a global variable
			if (calls % 2 == 0){
			PeriodicTask1 = task;          // user function
			TIMER4_TAILR_R = (period)-1;    // start value for trigger
			NVIC_PRI17_R = (NVIC_PRI17_R&0xFF00FFFF)| (priority << 21); //set priority
	  	NVIC_EN2_R = 1<<6;              // enable interrupt 70 in NVIC
			TIMER4_CTL_R |= 0x00000001;   // enable timer2A 32-b, periodic, no interrupts
		}
		else{	//Wide Timer0A
			PeriodicTask2 = task;          // user function
			WTIMER0_TAILR_R = (period)-1;    // start value for trigger
			NVIC_PRI23_R = (NVIC_PRI23_R&0xFF00FFFF)| (priority << 21); //set priority
	  	NVIC_EN2_R = 1<<30;              // enable interrupt 94 in NVIC
			WTIMER0_CTL_R |= 0x00000001;   // enable timer0
		}
	}
	calls++;
	EnableInterrupts();
	return 0; 
}





3)
void PortF_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x20; // activate port F
  while((SYSCTL_PRGPIO_R&0x20)==0){}; // allow time for clock to start 
  GPIO_PORTF_DIR_R &= ~0x11;    // (c) make PF4 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x11;  //     disable alt funct on PF0, PF4
  GPIO_PORTF_DEN_R |= 0x11;     //     enable digital I/O on PF4
  GPIO_PORTF_PCTL_R &= ~0x000F000F; //  configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R &= ~0x11;  //    disable analog functionality on PF4
  GPIO_PORTF_PUR_R |= 0x11;     //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x11;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x11;    //     PF4 is not both edges
  GPIO_PORTF_IEV_R &= ~0x11;    //     PF4 falling edge event
  GPIO_PORTF_ICR_R = 0x11;      // (e) clear flag4
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00400000; // (g) priority 2
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
}

//disable one button at a time?
void disarmButt(int disableButt){
	//disable button 1
	if (disableButt == 1){
		GPIO_PORTF_IM_R  &= ~0x10;	//PF4 = switch 1
	}
	//disable button 2
	if (disableButt == 2){
		GPIO_PORTF_IM_R  &= ~0x01;  //PF1 = switch 2
	}
}

4)
Blocking semaphore

//TODO: ADD blockedPtr and currBlockedPtr to sema4type struct
void OS_Wait(Sema4Type *semaPt) {
  DisableInterrupts();
  (semaPt->Value) = (semaPt->Value) - 1;

   // take tcb out of linked list
  if (semaPt->Value < 0){
  	if (semaPt->blockedPtr == 0){
  		semaPt -> blockedPtr = RunPt;
  		semaPt -> currBlockedPtr = RunPt;	//currBlockedPtr points to beginning of blocked linked list
  	} 
  	else{
  		semaPt -> currBlockedPtr -> next = RunPt; 
  	}
  	// remove from linked list
  	if (NumThreads > 1){
  		RunPt -> prev -> next = RunPt -> next;
  		RunPt -> next - > prev = RunPt -> prev; 
  	}
  }
  EnableInterrupts();
}


void OS_Signal(Sema4Type *semaPt){
	
  long status;
  status = StartCritical();
  (semaPt->Value) = (semaPt->Value) + 1; //add 1 to semaphore
  if (semaPt -> Value <= 0){
  	tcbType *wokeTCB = semaPt -> blockedPtr;
  	 //insert tcb into running tcb linked list
  	semaPt -> blockedPtr = wokeTCB -> next; //set blockedPtr to the next entry
  	wokeTCB -> next = RunPt -> next;
  	wokeTCB -> prev = RunPt;
  	RunPt -> next -> prev = wokeTCB;
  	RunPt -> next = wokeTCB;
  	
  }

  EndCritical(status);
}

TODO: add the 2nd method if you have no life (Andrew lol)

5) 
Method 1: find the max priority (smallest number)

void OS_SelectNextThread(void){
	int max = maxPriority;	//maxPriority is global variable, keep track of it when we add thread
	tcbType *maxPtr = RunPt->next;
	 // find maximum priority, avoid dead/uninitialized threads and sleeping threads
	for (int i = 0; i < NumThreads; i++){
		if (NextPt -> priority < max && (NextPt->id != 0 || NextPt->sleep > 0)){
			max = NextPt -> priority;
			maxPtr = NextPt;
		}
	}
	 // set next pointer to TCB with highest next pointer
	NextPt = maxPtr;
}

Method 2: give higher priority more time slice

void OS_SelectNextThread(void){
	NextPt = RunPt->next;	//switch threads using round-robin, avoid dead/uninitialized threads and sleeping threads
	while(NextPt->id == 0 || NextPt->sleep){
		NextPt = NextPt->next;
	}

	 //timeslice is an int (e.g. 2, 3, etc.) that specifies number of times to 
	 //SysTick interrupt before switching threads. Multiplier is to adjust time slice
	timeSlice = multiplier * (NumThreads - 	max);	
}
