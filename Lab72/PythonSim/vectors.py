import math

def magnitude(v):
    return (v[0]**2 + v[1]**2)**.5
def rotateClockwise(v, angle):
    radAngle = math.radians(angle)
    sinDir = math.sin(radAngle)
    cosDir = math.cos(radAngle)
    return v[0]*cosDir-v[1]*sinDir, v[0]*sinDir+v[1]*cosDir
def vectorAdd(v1, v2):
    return v1[0] + v2[0], v1[1] + v2[1]
def vectorSubtract(v1, v2):
    return v1[0] - v2[0], v1[1] - v2[1]
def vectorScalarMultiply(v1, s2):
    return v1[0]*s2, v1[1]*s2
def vectorAddScaled(v1, v2, s2):
    return vectorAdd(v1, vectorScalarMultiply(v2, s2))
