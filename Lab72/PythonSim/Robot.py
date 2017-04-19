car_size = car_width, car_height = 10, 20
#sensors defined by position from Back center of car and angle from Back->Front vector
IRSensors = [(car_width/2, car_height, 90),     #pointing right front
            (-car_width/2, car_height, -90),    #pointing left front
            (car_width/2, 0, 90),               #pointing right back
            (-car_width/2, 0, -90),             #pointing left back
            (car_width/2, car_height, 0),       #pointing front right
            (-car_width/2, car_height, 0),      #pointing front left
            (car_width/2, car_height, 90),      #same as 1, unused
            (car_width/2, car_height, 90)]      #same as 1, unused

def RobotMove(sensors):
    IR = sensors[0]
    RF = IR[0]
    LF = IR[1]
    RB = IR[2]
    LB = IR[3]
    FR = IR[4]
    FL = IR[5]

    rightMin = min(RF, RB)
    leftMin = min(LF, LB)
    frontMin = min(FR, FL)

    if leftMin < rightMin:
        angle = 45
    else:
        angle = -45

    speed = 1   
    if frontMin < 5:
        speed = -1

    if speed < 0:
        angle = angle*-1
            
    return speed, angle
