// OS.h
// Runs on TM4C123
// Andrew Wong and Clint Simpson
// LPOS-1 (Low-Profile Operating System, v1)

int OS_AddPeriodicThread(void(*task)(void), uint32_t period, uint32_t priority);
void OS_ClearPeriodicTime(void);
uint32_t OS_ReadPeriodicTime(void);
