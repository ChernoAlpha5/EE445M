// Drive.c
// Runs on TM4C123

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "PWM.h"
#include "Drive.h"

#define POWERMIN 400
#define POWERMAX 12400
#define SERVOMID 1875

void Drive_Init(void){
	Left_Init(12500, POWERMIN, 0);          // initialize PWM0, 100 Hz
  Right_InitB(12500, POWERMIN, 0);   // initialize PWM0, 100 Hz
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
	duty = ((newDir + 45) * 125)/9 + 1250;
	Servo_Duty(duty);
}


//takes in a speed from -MAXSPEED to MAXSPEED
void Drive_Speed(int8_t speed){
	int8_t newSpeed;
	uint8_t direction = 0;
	uint32_t newDuty;
	if(speed < 0){
		direction = 1;
		newSpeed = speed*-1;
	}
	if(newSpeed > MAXSPEED){
		newSpeed = MAXSPEED;
	}
	if(speed == 0){
		speed = 1;
	}
	newDuty = newSpeed*POWERMAX/MAXSPEED;
	Right_DutyB(newDuty, direction);
	Left_Duty(newDuty, direction);
}

void Drive(int8_t speed, int8_t dir){
	Drive_Speed(speed);
	Drive_WheelDirection(dir);
}

