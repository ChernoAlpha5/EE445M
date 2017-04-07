;/*****************************************************************************/
;/* OSasm.s: low-level OS commands, written in assembly                       */
;/*****************************************************************************/
;Jonathan Valvano/Andreas Gerstlauer, OS Lab 5 solution, 2/28/16


        AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB
        REQUIRE8
        PRESERVE8

		EXPORT	OS_Id
        EXPORT  OS_Sleep
		EXPORT	OS_Kill
		EXPORT	OS_Time
		EXPORT	OS_AddThread

			
;PF2ADDR  DCW 0x5010
;		 DCW 0x4002
		 
OS_Id
     ;toggle PF2 before each SVC call
	PUSH {r0, r1}
	LDR r0, =0x40025010  ; @0x00000850
	LDR r1,[r0]
	EOR r1,r1,#0x04
	STR r1,[r0]
	POP {r0, r1}
	SVC		#0
	BX		LR
		 
OS_Kill
	PUSH {r0, r1}
	LDR r0, =0x40025010  ; @0x00000850
	LDR r1,[r0]
	EOR r1,r1,#0x04
	STR r1,[r0]
	POP {r0, r1}
	SVC		#1
	BX		LR

OS_Sleep
	PUSH {r0, r1}
	LDR r0, =0x40025010  ; @0x00000850
	LDR r1,[r0]
	EOR r1,r1,#0x04
	STR r1,[r0]
	POP {r0, r1}
	SVC		#2
	BX		LR

OS_Time
	PUSH {r0, r1}
	LDR r0, =0x40025010  ; @0x00000850
	LDR r1,[r0]
	EOR r1,r1,#0x04
	STR r1,[r0]
	POP {r0, r1}
	SVC		#3
	BX		LR

OS_AddThread
	PUSH {r0, r1}
	LDR r0, =0x40025010  ; @0x00000850
	LDR r1,[r0]
	EOR r1,r1,#0x04
	STR r1,[r0]
	POP {r0, r1}
	SVC		#4
	BX		LR

    ALIGN
    END
