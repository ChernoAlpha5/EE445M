// Drive.c
// Runs on TM4C123

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "PWM.h"
#include "Drive.h"

#define POWERMIN 400
#define POWERMAX 12400
#define SERVOMID 1875

uint16_t power, direction;

void Drive_Init(void){
	Left_Init(12500, POWERMIN, 0);          // initialize PWM0, 100 Hz
  Right_InitB(12500, POWERMIN, 0);   // initialize PWM0, 100 Hz
  Servo_Init(25000, SERVOMID);
	direction = 0; //going straight
	power = POWERMIN;
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

void Drive(int8_t speed, int8_t dir){
	Drive_Speed(speed);
	if(dir > 45 || dir < -45){
		Drive_SteepDifferentialTurn(dir);
	}
	else{
		Drive_DifferentialTurn(dir);
	}
}

