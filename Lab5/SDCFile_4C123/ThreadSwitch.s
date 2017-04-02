		AREA    |.text|, CODE, READONLY, ALIGN=2
		THUMB		   
		PRESERVE8 {TRUE}
		EXPORT PendSV_Handler 
;		EXPORT SysTick_Handler
		EXPORT StartOS
		EXPORT SVC_Handler
			
		EXTERN RunPt
		EXTERN NextPt
		EXTERN RecordDongs
		EXTERN OS_Id
		EXTERN OS_Kill
		EXTERN OS_Sleep
		EXTERN OS_Time
		EXTERN OS_AddThread

			
;Round-Robin Switching			
;SysTick_Handler 
;	CPSID I
;	PUSH {R4 - R11}
;	LDR R0, =RunPt
;	LDR R1, [R0]
;	STR SP, [R1]
;	LDR R1, [R1, #4]
;	STR R1, [R0]
;	LDR SP, [R1]
;	POP {R4 - R11}
;	CPSIE I
;	BX LR

SVCJump
	B OS_Id
	B OS_Kill
	B OS_Sleep
	B OS_Time
	B OS_AddThread

SVC_Handler
	LDR R12, [SP, #24]
	LDRH R12, [R12, #-2]
	BIC R12, #0xFF00
	CMP R12, #5
	BHI EndSVC
	LDM SP, {R0 -  R3}
	PUSH {LR}
	LDR LR, =Done
	CMP R12, #0
	BEQ OS_Id
	CMP R12, #1
	BEQ OS_Kill
	CMP R12, #2
	BEQ OS_Sleep
	CMP R12, #3
	BEQ OS_Time
	CMP R12, #4
	BEQ OS_AddThread
Done POP {LR}
	STR R0, [SP]
EndSVC BX LR



;switch using chosen NextPt
PendSV_Handler
	CPSID I
	PUSH {LR}
	BL OS_Id
	BL RecordDongs
	POP {LR}
	PUSH {R4 - R11}
	LDR R0, =RunPt
	LDR R1, [R0]
	STR SP, [R1]
	LDR R1, =NextPt
	LDR R2, [R1]
	STR R2, [R0]
	LDR SP, [R2]
	POP {R4 - R11}
	CPSIE I
	BX LR
	
StartOS
    LDR     R0, =RunPt         ; currently running thread
    LDR     R2, [R0]           ; R2 = value of RunPt
    LDR     SP, [R2]           ; new thread SP; SP = RunPt->stackPointer;
    POP     {R4-R11}           ; restore regs r4-11
    POP     {R0-R3}            ; restore regs r0-3
    POP     {R12}
    POP     {LR}               ; discard LR from initial stack
    POP     {LR}               ; start location
    POP     {R1}               ; discard PSR
    CPSIE   I                  ; Enable interrupts at processor level
    BX      LR                 ; start first thread

    ALIGN
    END
		