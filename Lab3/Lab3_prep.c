Lab 3 prep
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


void void OS_Signal(Sema4Type *semaPt){
	
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
