// ADC.h
// Runs on LM4F120/TM4C123
// Provide a function that can initialize Timer2 or software triggered interrupts
// SS3 conversions and request an interrupt when the conversion
// is complete.
// Daniel Valvano
// May 2, 2015

void ADC0_SS2_4Channels_TimerTriggered_Init(uint32_t period);

#define AddADCFilter(NAME) \
uint16_t ADCFilter ## NAME(uint16_t data){ \
static long x ## NAME[6]; \
static long y ## NAME[6]; \
static unsigned long n ## NAME=3; \
  n ## NAME++; \
  if(n ## NAME==6) n ## NAME=3;     \
  x ## NAME[n ## NAME] = x ## NAME[n ## NAME-3] = data; \
  y ## NAME[n ## NAME] = (256*(x ## NAME[n ## NAME]+x ## NAME[n ## NAME-2])-503*x ## NAME[n ## NAME-1]+498*y ## NAME[n ## NAME-1]-251*y ## NAME[n ## NAME-2]+128)/256; \
  y ## NAME[n ## NAME-3] = y ## NAME[n ## NAME];    \
  return y ## NAME[n ## NAME]; \
} \

