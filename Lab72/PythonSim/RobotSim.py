import sys, pygame, pygame.gfxdraw, importlib
from vectors import magnitude, rotateClockwise, vectorAdd, vectorSubtract, vectorScalarMultiply, vectorAddScaled

pygame.init()

screen_size = screen_width, screen_height = 640, 480
black = 0, 0, 0
red = 255, 0, 0
green = 0, 255, 0
blue = 0, 0, 255
white = 255, 255, 255
screen = pygame.display.set_mode(screen_size)
clock = pygame.time.Clock()
track = pygame.image.load("track.bmp")
car_size = car_width, car_height = 10, 20
class car:
    def __init__(self, Front, direction, FileName, color):
        _temp = __import__(FileName, globals(), locals(), ['IRSensors', 'RobotMove'], 0)
        self.IRSensors = _temp.IRSensors
        self.RobotMove = _temp.RobotMove
        self.Front = Front
        self.Back = [self.Front[0], self.Front[1]+car_height]
        self.IR = [0, 0, 0, 0, 0, 0, 0, 0]
        self.US = [0, 0, 0, 0]
        self.sensors = self.IR, self.US 
        self.DirVec = vectorSubtract(self.Front, self.Back)
        self.MagDirVec = magnitude(self.DirVec)
        self.UDirVec = [self.DirVec[0]/self.MagDirVec , self.DirVec[1]/self.MagDirVec]
        self.color = color
        return
    
    def RobotUpdate(self, speed, wheelDir): # x,y of mid Front/Back
        #global Front, Back, DirVec, MagDirVec, UDirVec
        self.DirVec = vectorSubtract(self.Front, self.Back)
        self.MagDirVec = (self.DirVec[0]**2 + self.DirVec[1]**2)**.5
        self.UDirVec = self.DirVec[0]/self.MagDirVec , self.DirVec[1]/self.MagDirVec
        self.Back = vectorAddScaled(self.Back, self.UDirVec, speed)
    
        RUDirVec = rotateClockwise(self.UDirVec, wheelDir)
        self.Front = vectorAddScaled(self.Front, RUDirVec, speed)

        self.DirVec = vectorSubtract(self.Front, self.Back)
        self.MagDirVec = magnitude(self.DirVec)
        self.UDirVec = self.DirVec[0]/self.MagDirVec , self.DirVec[1]/self.MagDirVec
        self.Front = vectorAddScaled(self.Back, self.UDirVec, car_height)
        return

    def RobotDraw(self):
        RUDirVec = rotateClockwise(self.UDirVec, 90)
        FrontL = vectorAddScaled(self.Front, RUDirVec, -car_width/2)
        FrontR = vectorAddScaled(self.Front, RUDirVec, car_width/2)
        BackL  = vectorAddScaled(self.Back, RUDirVec, -car_width/2)
        BackR  = vectorAddScaled(self.Back, RUDirVec, car_width/2)
        #pygame.gfxdraw.aapolygon(screen, (FrontL, FrontR, BackR, BackL), red)
        pygame.gfxdraw.filled_polygon(screen, (FrontL, FrontR, BackR, BackL), self.color)
        return

    def CheckCollision(self):
        RUDirVec = rotateClockwise(self.UDirVec, 90)
        for x in range(int(-car_width/2),int(car_width/2)):
            curPixel = vectorAddScaled(self.Back, RUDirVec, x)
            curPixel = int(curPixel[0]), int(curPixel[1])
            if track.get_at(curPixel) != white:
                return 1
            curPixel = vectorAddScaled(self.Front, RUDirVec, x)
            curPixel = int(curPixel[0]), int(curPixel[1])
            if track.get_at(curPixel) != white:
                return 1
        midRight = vectorAddScaled(self.Back, RUDirVec, car_width/2)
        midRight = vectorAddScaled(midRight, self.UDirVec, car_height/2)
        midLeft = vectorAddScaled(self.Back, RUDirVec, -car_width/2)
        midLeft = vectorAddScaled(midLeft, self.UDirVec, car_height/2)
        for y in range(int(-car_height/2),int(car_height/2)):
            curPixel = vectorAddScaled(midRight, self.UDirVec, y)
            curPixel = int(curPixel[0]), int(curPixel[1])
            if track.get_at(curPixel) != white:
                return 1
            curPixel = vectorAddScaled(midLeft, self.UDirVec, y)
            curPixel = int(curPixel[0]), int(curPixel[1])
            if track.get_at(curPixel) != white:
                return 1
        return 0

    def CalculateSensor(self, sensor):
        RUDirVec = rotateClockwise(self.UDirVec, 90)
        location = self.Back
        location = vectorAddScaled(location, RUDirVec, sensor[0])
        location = vectorAddScaled(location, self.UDirVec, sensor[1])
        dirVec = rotateClockwise(self.UDirVec, sensor[2])
        x = 0
        while True:
            trackTested = vectorAddScaled(location, dirVec, x)
            trackTested = int(trackTested[0]), int(trackTested[1])
            if screen.get_at(trackTested) != white:
                return magnitude(vectorSubtract(trackTested, location))
            x = x + 1
        return 0

    def UpdateSensors(self):
        x = 0
        for sensor in self.IRSensors:
            self.IR[x] = self.CalculateSensor(sensor)
            x = x + 1
        return
carList = [car((605, 290), 12, 'Robot', red),
           car((605, 290), 12, 'Robot2', green),
           car((605, 290), 12, 'RobotSEAN', blue)]
while 1:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.display.quit()
            sys.exit()
    screen.blit(track, (0,0))
    for cars in carList:
        cars.UpdateSensors()
        speed, wheelDir = cars.RobotMove(cars.sensors)
        cars.RobotUpdate(speed, wheelDir)
        cars.RobotDraw()
        if cars.CheckCollision():
            print('crashed')
    pygame.display.flip()
    clock.tick(100)
