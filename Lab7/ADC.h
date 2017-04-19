// ADC.h
// Runs on LM4F120/TM4C123
// Provide a function that can initialize Timer2 or software triggered interrupts
// SS3 conversions and request an interrupt when the conversion
// is complete.
// Daniel Valvano
// May 2, 2015


void ADC0_Open(uint8_t channelNum);

void ADC0_InitSWTriggerSeq3(uint32_t channelNum);

uint32_t ADC0_InSeq3(void);

uint16_t ADC0_In(uint8_t channelNum);

// int ADC_Status(void);

void ADC_Init(uint8_t channelNum);

uint16_t ADC_In(void /*uint8_t channelNum*/);

void ADC0_SS3_4Channels_TimerTriggered_Init(uint32_t period);

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


 /*void ADC1_Init(uint8_t channelNum, uint32_t period);

 void ADC_Initiate(uint8_t channelNum);

 void ADC_Read(uint8_t channelNums[2], int* firstChannelData, int* secondChannelData);*/
