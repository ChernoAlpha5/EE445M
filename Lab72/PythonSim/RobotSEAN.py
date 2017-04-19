## RobotSEAN
car_size = car_width, car_height = 10, 20
#sensors defined by position from Back center of car and angle from Back->Front vector
IRSensors = [(car_width/2, car_height, 45),     #pointing right front
            (-car_width/2, car_height, -45),    #pointing left front
            (car_width/2, 0, 135),               #pointing right back
            (-car_width/2, 0, -135),             #pointing left back
            (0, car_height, 0),       #pointing front right
            (0, 0, 180),      #pointing front left
            (car_width/2, car_height/2, 90),      #same as 1, unused
            (-car_width/2, car_height/2, -90)]      #same as 1, unused

def RobotMove(sensors):
    IR = sensors[0]
    RF = IR[0]
    LF = IR[1]
    RB = IR[2]
    LB = IR[3]
    F = IR[4]
    B = IR[5]
    RM = IR[6]
    LM = IR[7]

    rightMax = max(RF, RM)
    leftMax = max(LF, LM)
    rightMin = min(RF, RM)
    leftMin = min(LF, LM)
    frontMin = min(F, LF, RF)
    angle = 0
    if max(RF,LF,F,RM,LM) < 10:
        angle = -180
    elif RF == max(RF,LF,F,RM,LM):
        angle = 45
    elif LF == max(RF,LF,F,RM,LM):
        angle = -45
    elif F == max(RF,LF,F,RM,LM):
        angle = 0
    elif RM == max(RF,LF,F,RM,LM):
        angle = 90
    else: # LM
        angle = -90
        
    speed = 1#max(RF,LF,F,RM,LM)/5

##    if rightMax > leftMax:
##        if rightMax > F:
##            if rightMin > 10:
##                angle = 45
##            else:
##                angle = -45
##    else:
##        if leftMax > F:
##            if leftMin > 10:
##                angle = -45
##            else:
##                angle = 45
##    speed = frontMin/5
    
    return speed, angle
