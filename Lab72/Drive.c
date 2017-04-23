// Drive.c
// Runs on TM4C123

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "PWM.h"
#include "Drive.h"

#define POWERMIN 100
#define POWERMAX 12400
#define SERVOMID 1875

uint16_t power, direction;

void Drive_Init(void){
	direction = 0; //going straight
	power = POWERMIN;
	Left_Init(12500, power, direction);          // initialize PWM0, 100 Hz
  Right_InitB(12500, power, direction);   // initialize PWM0, 100 Hz
  Servo_Init(25000, SERVOMID);
}



//takes in an angle from -45 to 45 degrees, changes servo PWM Duty cycle
void Drive_WheelDirection(int8_t dir){
	int8_t newDir;
	uint16_t duty;
	if(dir < -45){
		newDir = -45;
	}
	else if(dir > 45){
		newDir = 45;
	}
	else{
		newDir = dir;
	}
	duty = ((45  - newDir) * 125)/9 + 1250;
	Servo_Duty(duty);
}

void Drive_DifferentialTurn(int8_t dir){	
	Drive_WheelDirection(dir);
	if(dir < 0){
		Left_Duty(power/4, direction);
	}
	else{
		Right_DutyB(power/4,direction);
	}
	
}

void Drive_SteepDifferentialTurn(int8_t dir){
	Drive_WheelDirection(dir);
	if(dir < 0){
		Left_Duty(power/2, 1 - direction);
	}
	else{
		Right_DutyB(power/1, 1 - direction);
	}
}

//takes in a speed from -MAXSPEED to MAXSPEED
void Drive_Speed(int8_t speed){
	int8_t newSpeed;
	uint32_t newDuty;
	if(speed < 0){
		direction = 1;
		newSpeed = speed*-1;
	}
	else{
		direction = 0;
		newSpeed = speed;
	}
	if(newSpeed > MAXSPEED){
		newSpeed = MAXSPEED;
	}
	newDuty = newSpeed*POWERMAX/MAXSPEED;
	if(newDuty < POWERMIN){
		newDuty = POWERMIN;
	}
	power = newDuty;
	Left_Duty(newDuty, direction);
	Right_DutyB(newDuty, direction);
	
}


#define IMAX  4000
#define IMIN	-2000

#define UMAX POWERMAX
#define UMIN POWERMIN

#define EKIdT  (E*100)/646
#define EKP    (E*100)/20

void Drive_PIControlDirection(int32_t E){
	static int32_t IL = 0;
	static int32_t IR = 0;
	int32_t P = EKP;
	IL -= EKIdT;
	IR += EKIdT;
	if(IL<IMIN) IL = IMIN;
	if(IL>IMAX) IL = IMAX;
	if(IR<IMIN) IR = IMIN;
	if(IR>IMAX) IR = IMAX;
	
	int32_t UL = IL - P;
	int32_t UR = IR + P;
	
	uint8_t dirL = UL < 0;
	uint8_t dirR = UR < 0;
	
	if(dirL) UL*=-1;
	if(dirR) UR*=-1;
	
	if(UL<UMIN) UL = UMIN;
	if(UR<UMIN) UR = UMIN;
	if(UL>UMAX) UL = UMAX;
	if(UR>UMAX) UR = UMAX;
	
	Left_Duty(UL, dirL);
	Right_DutyB(UR, dirR);
}

void Drive(int8_t speed, int8_t dir){
	Drive_Speed(speed);
	if(dir > 45 || dir < -45){
		Drive_SteepDifferentialTurn(dir);
	}
	else{
		Drive_DifferentialTurn(dir);
	}
}

