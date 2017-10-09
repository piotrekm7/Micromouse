#include "stm32f10x.h"
#include <math.h>

volatile uint32_t timer_ms = 0;

volatile double time = 0;
char obrotyFlag1 = 1, obrotyFlag2 = 1;
double obroty1 = 0, obroty2 = 0; // 1-lewy
int temp1 = 0, temp2 = 0;
int previousTemp1 = -1, previousTemp2 = -1;
double target1 = 0, target2 = 0;
int uchyb1 = 1, uchyb2 = 1, uchybPrev1 = 0, uchybPrev2 = 0;
double K = 2, Td = 1; ; Ti = 1;

#define INF 32000

#define LABIRYNT_SIZE (8 * 8)

char previousKierunek = 'N';

struct Stack { // zdefiniowanie stosu
  short stack[LABIRYNT_SIZE];
  short size;
};

struct Pole {
  short numer;
  short s1, s2, s3, s4; /// sasiedzi
  short odleglosc;      /// odleglosc od srodka planszy
};

struct Pole pola[LABIRYNT_SIZE];

struct WhereToGo {
  char kierunek;
  short numer;
};

struct WhereToGo findPath(short actual); // zwraca N,W,E,S kierunek gdzie jechac

short stackPeek(struct Stack *stack) { // wskazuje co jest na wierzchu
  if (stack->size == 0) {
    return -1;
  }
  return stack->stack[stack->size - 1];
}

void stackPush(struct Stack *stack, short i) { // dodanie do stosu
  stack->stack[stack->size] = i;
  stack->size++;
}

void stackPop(struct Stack *stack) { stack->size--; }

void initPola() {
  short temp;
  int i;
  for (i = 0; i < LABIRYNT_SIZE; i++) {
    pola[i].numer = i;
    temp = fmin(abs(i / 8 - 3), abs(i / 8 - 4));
    temp += fmin(abs(i % 8 - 3), abs(i % 8 - 4));
    pola[i].odleglosc = temp;
    pola[i].s1 = i - 8 >= 0 ? i - 8 : -1;
    pola[i].s2 = i % 8 != 7 ? i + 1 : -1;
    pola[i].s3 = i + 8 <= LABIRYNT_SIZE - 1 ? i + 8 : -1;
    pola[i].s4 = i % 8 != 0 ? i - 1 : -1;
  }
}

void sprawdzSciany(short pole) {
  int sciany;

  sciany = 100 * (~GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12)) +
           10 * (~GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4)) +
           (~GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4));

  char L, M, R;
  L = M = R = 0;

  if (sciany >= 100) {
    L = 1;
  }
  sciany %= 100;
  if (sciany >= 10) {
    M = 1;
  }
  sciany %= 10;
  if (sciany >= 1) {
    R = 1;
  }

  switch (previousKierunek) {
  case 'N': {
    if (L == 1) {
      pola[pole].s1 = -1;
    }
    if (M == 1) {
      pola[pole].s2 = -1;
    }
    if (R == 1) {
      pola[pole].s3 = -1;
    }
    break;
  }
  case 'W': {
    if (L == 1) {
      pola[pole].s4 = -1;
    }
    if (M == 1) {
      pola[pole].s1 = -1;
    }
    if (R == 1) {
      pola[pole].s2 = -1;
    }
    break;
  }
  case 'E': {
    if (L == 1) {
      pola[pole].s2 = -1;
    }
    if (M == 1) {
      pola[pole].s3 = -1;
    }
    if (R == 1) {
      pola[pole].s4 = -1;
    }
    break;
  }
  case 'S': {
    if (L == 1) {
      pola[pole].s3 = -1;
    }
    if (M == 1) {
      pola[pole].s4 = -1;
    }
    if (R == 1) {
      pola[pole].s1 = -1;
    }
    break;
  }
  }
}

char checkPole(short pole) {
  int temp = INF;
  if (pola[pole].s1 != -1)
    temp = fmin(temp, pola[pola[pole].s1].odleglosc);
  if (pola[pole].s2 != -1)
    temp = fmin(temp, pola[pola[pole].s2].odleglosc);
  if (pola[pole].s3 != -1)
    temp = fmin(temp, pola[pola[pole].s3].odleglosc);
  if (pola[pole].s4 != -1)
    temp = fmin(temp, pola[pola[pole].s4].odleglosc);

  if (pola[pole].odleglosc == 0 || pola[pole].odleglosc == temp + 1) {
    return 0; /// jest ok
  } else {
    pola[pole].odleglosc = temp + 1;
    return 1; /// trzeba poprawic odleglosci
  }
}

struct WhereToGo findPath(short actual) { // zwraca N,W,E,S
  short temp = INF;
  struct WhereToGo whereToGo;

  switch (previousKierunek) {
  case 'N': {
    if (pola[actual].s2 != -1 && pola[pola[actual].s2].odleglosc < temp) {
      temp = pola[pola[actual].s2].odleglosc;
      whereToGo.kierunek = 'N';
      whereToGo.numer = pola[actual].s2;
    }
    if (pola[actual].s1 != -1 && pola[pola[actual].s1].odleglosc < temp) {
      temp = pola[pola[actual].s1].odleglosc;
      whereToGo.kierunek = 'W';
      whereToGo.numer = pola[actual].s1;
    }
    if (pola[actual].s3 != -1 && pola[pola[actual].s3].odleglosc < temp) {
      temp = pola[pola[actual].s3].odleglosc;
      whereToGo.kierunek = 'E';
      whereToGo.numer = pola[actual].s3;
    }
    if (pola[actual].s4 != -1 && pola[pola[actual].s4].odleglosc < temp) {
      temp = pola[pola[actual].s4].odleglosc;
      whereToGo.kierunek = 'S';
      whereToGo.numer = pola[actual].s4;
    }
    break;
  }
  case 'W': {
    if (pola[actual].s1 != -1 && pola[pola[actual].s1].odleglosc < temp) {
      temp = pola[pola[actual].s1].odleglosc;
      whereToGo.kierunek = 'W';
      whereToGo.numer = pola[actual].s1;
    }
    if (pola[actual].s2 != -1 && pola[pola[actual].s2].odleglosc < temp) {
      temp = pola[pola[actual].s2].odleglosc;
      whereToGo.kierunek = 'N';
      whereToGo.numer = pola[actual].s2;
    }
    if (pola[actual].s4 != -1 && pola[pola[actual].s4].odleglosc < temp) {
      temp = pola[pola[actual].s4].odleglosc;
      whereToGo.kierunek = 'S';
      whereToGo.numer = pola[actual].s4;
    }
    if (pola[actual].s3 != -1 && pola[pola[actual].s3].odleglosc < temp) {
      temp = pola[pola[actual].s3].odleglosc;
      whereToGo.kierunek = 'E';
      whereToGo.numer = pola[actual].s3;
    }
    break;
  }
  case 'E': {
    if (pola[actual].s3 != -1 && pola[pola[actual].s3].odleglosc < temp) {
      temp = pola[pola[actual].s3].odleglosc;
      whereToGo.kierunek = 'E';
      whereToGo.numer = pola[actual].s3;
    }
    if (pola[actual].s2 != -1 && pola[pola[actual].s2].odleglosc < temp) {
      temp = pola[pola[actual].s2].odleglosc;
      whereToGo.kierunek = 'N';
      whereToGo.numer = pola[actual].s2;
    }
    if (pola[actual].s4 != -1 && pola[pola[actual].s4].odleglosc < temp) {
      temp = pola[pola[actual].s4].odleglosc;
      whereToGo.kierunek = 'S';
      whereToGo.numer = pola[actual].s4;
    }
    if (pola[actual].s1 != -1 && pola[pola[actual].s1].odleglosc < temp) {
      temp = pola[pola[actual].s1].odleglosc;
      whereToGo.kierunek = 'W';
      whereToGo.numer = pola[actual].s1;
    }
    break;
  }
  case 'S': {
    if (pola[actual].s4 != -1 && pola[pola[actual].s4].odleglosc < temp) {
      temp = pola[pola[actual].s4].odleglosc;
      whereToGo.kierunek = 'S';
      whereToGo.numer = pola[actual].s4;
    }
    if (pola[actual].s1 != -1 && pola[pola[actual].s1].odleglosc < temp) {
      temp = pola[pola[actual].s1].odleglosc;
      whereToGo.kierunek = 'W';
      whereToGo.numer = pola[actual].s1;
    }
    if (pola[actual].s3 != -1 && pola[pola[actual].s3].odleglosc < temp) {
      temp = pola[pola[actual].s3].odleglosc;
      whereToGo.kierunek = 'E';
      whereToGo.numer = pola[actual].s3;
    }
    if (pola[actual].s2 != -1 && pola[pola[actual].s2].odleglosc < temp) {
      temp = pola[pola[actual].s2].odleglosc;
      whereToGo.kierunek = 'N';
      whereToGo.numer = pola[actual].s2;
    }
    break;
  }
  }

  return whereToGo;
}

void SysTick_Handler() {
  if (timer_ms) {
    timer_ms--;
  }
}

void delay_ms(int time) {
  timer_ms = time;
  while (timer_ms > 0) {
  };
}

#define LEWY_PRZOD (GPIO_SetBits(GPIOB, GPIO_Pin_6))
#define LEWY_TYL (GPIO_ResetBits(GPIOB, GPIO_Pin_6))
#define PRAWY_PRZOD (GPIO_SetBits(GPIOB, GPIO_Pin_8))
#define PRAWY_TYL (GPIO_ResetBits(GPIOB, GPIO_Pin_8))

void PWM_Init() {
  GPIO_InitTypeDef gpio;
  TIM_TimeBaseInitTypeDef tim;
  TIM_OCInitTypeDef channel;
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                             RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD,
                         ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

  GPIO_StructInit(&gpio); // AENABLE
  gpio.GPIO_Pin = GPIO_Pin_7;
  gpio.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOB, &gpio);

  GPIO_StructInit(&gpio); // BENABLE
  gpio.GPIO_Pin = GPIO_Pin_9;
  gpio.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOB, &gpio);

  TIM_TimeBaseStructInit(&tim);
  tim.TIM_CounterMode = TIM_CounterMode_Up;
  tim.TIM_Prescaler = 1;
  tim.TIM_Period = 1000 - 1;
  TIM_TimeBaseInit(TIM4, &tim);

  TIM_OCStructInit(&channel);
  channel.TIM_OCMode = TIM_OCMode_PWM1;
  channel.TIM_OutputState = TIM_OutputState_Enable;
  channel.TIM_Pulse = 100; // PIERWSZY KANAL
  TIM_OC2Init(TIM4, &channel);

  channel.TIM_Pulse = 100; // DRUGI KANAL
  TIM_OC4Init(TIM4, &channel);

  TIM_Cmd(TIM4, ENABLE);

  gpio.GPIO_Pin = GPIO_Pin_5; // MODE
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &gpio);

  GPIO_SetBits(GPIOB, GPIO_Pin_5);

  gpio.GPIO_Pin = GPIO_Pin_6; // USTAWIANIE KIERUNKU OBROTOW
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &gpio);

  LEWY_PRZOD;

  gpio.GPIO_Pin = GPIO_Pin_8;
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &gpio);

  PRAWY_PRZOD;
}

void ENCODER_Init() {
  GPIO_InitTypeDef GPIO_StructInit;
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_ICInitTypeDef TIM_ICInitStructure;
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

  GPIO_StructInit.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_StructInit.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_StructInit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_StructInit);

  TIM_TimeBaseStructure.TIM_Prescaler = 1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Period = 5;
  TIM_TimeBaseStructure.TIM_ClockDivision = 1;
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

  TIM_ICStructInit(&TIM_ICInitStructure);
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInit(TIM3, &TIM_ICInitStructure);

  TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInit(TIM3, &TIM_ICInitStructure);

  TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Falling,
                             TIM_ICPolarity_Falling);

  TIM_SetCounter(TIM3, 0);

  TIM_Cmd(TIM3, ENABLE);

  GPIO_StructInit.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
  GPIO_StructInit.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_StructInit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_StructInit);

  TIM_TimeBaseStructure.TIM_Prescaler = 1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Period = 5; /////////////zZZZZZZZ
  TIM_TimeBaseStructure.TIM_ClockDivision = 1;
  TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

  TIM_ICStructInit(&TIM_ICInitStructure);
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInit(TIM1, &TIM_ICInitStructure);

  TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInit(TIM1, &TIM_ICInitStructure);

  TIM_EncoderInterfaceConfig(TIM1, TIM_EncoderMode_TI12, TIM_ICPolarity_Falling,
                             TIM_ICPolarity_Falling);

  TIM_SetCounter(TIM1, 0);

  TIM_Cmd(TIM1, ENABLE);
}

void CZUJNIKI_Init() {
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                             RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD,
                         ENABLE);

  GPIO_InitTypeDef GPIO_StructInit; // ENABLE2
  GPIO_StructInit.GPIO_Pin = GPIO_Pin_3;
  GPIO_StructInit.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOA, &GPIO_StructInit);

  GPIO_StructInit.GPIO_Pin = GPIO_Pin_4; // OUT2
  GPIO_StructInit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_StructInit);

  GPIO_StructInit.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_3; // ENABLE1 , ENABLE3
  GPIO_StructInit.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_StructInit);

  GPIO_StructInit.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_4; // OUT1, OUT3
  GPIO_StructInit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOB, &GPIO_StructInit);

  GPIO_SetBits(GPIOA, GPIO_Pin_3);
  GPIO_SetBits(GPIOB, GPIO_Pin_3);
  GPIO_SetBits(GPIOB, GPIO_Pin_13);
}

void przyciskInit(void) {
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

  GPIO_InitTypeDef gpio;
  GPIO_StructInit(&gpio);

  gpio.GPIO_Pin = GPIO_Pin_14;
  gpio.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOB, &gpio);
}
void ledInit(void) {
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  GPIO_InitTypeDef gpio;
  GPIO_StructInit(&gpio);

  gpio.GPIO_Pin = GPIO_Pin_13;
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &gpio);

  gpio.GPIO_Pin = GPIO_Pin_15;
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &gpio);
}

void ADC_Calib() {
  ADC_ResetCalibration(ADC1);
  while (ADC_GetResetCalibrationStatus(ADC1))
    ;

  ADC_StartCalibration(ADC1);
  while (ADC_GetCalibrationStatus(ADC1))
    ;
}

int adc_read(int channel) {
  ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_71Cycles5);
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);

  while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC))
    ;

  return ADC_GetConversionValue(ADC1);
}

void ADC1_Init() {
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
  RCC_ADCCLKConfig(RCC_PCLK2_Div6);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                             RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD,
                         ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

  ADC_InitTypeDef adc;
  GPIO_InitTypeDef gpio;

  GPIO_StructInit(&gpio);
  gpio.GPIO_Pin = GPIO_Pin_0;
  gpio.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOB, &gpio);

  ADC_StructInit(&adc);
  adc.ADC_ContinuousConvMode = DISABLE;
  adc.ADC_NbrOfChannel = 1;
  adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_Init(ADC1, &adc);

  ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_71Cycles5);

  ADC_Cmd(ADC1, ENABLE);
}

void go(char nextKierunek) {
  char direction; // 0-forward,1-left,2-right,3-back
  if (nextKierunek == previousKierunek) {
    direction = '0';
  } else if ((nextKierunek == 'N' && previousKierunek == 'S') ||
             (nextKierunek == 'S' && previousKierunek == 'N') ||
             (nextKierunek == 'E' && previousKierunek == 'W') ||
             (nextKierunek == 'W' && previousKierunek == 'E')) {
    direction = '3';
  } else if ((nextKierunek == 'N' && previousKierunek == 'E') ||
             (nextKierunek == 'E' && previousKierunek == 'S') ||
             (nextKierunek == 'W' && previousKierunek == 'N') ||
             (nextKierunek == 'S' && previousKierunek == 'W')) {
    direction = '1';
  } else {
    direction = '2';
  }

  switch (direction) { // target1-prawy, target2-lewy

  case 1:
    target1 += 27 + 70;
    target2 -= 27 + 70;
    break;
  case 2:
    target1 -= 27 + 70;
    target2 += 27 + 70;
    break;
  case 3:
    target1 -= 54 + 70;
    target2 += 54 + 70;
    break;
  default:
    target1 += 70;
    target2 += 70;
  }
}

void TIMER_Init() {
  GPIO_InitTypeDef gpio;
  TIM_TimeBaseInitTypeDef tim;
  NVIC_InitTypeDef nvic;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                             RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD,
                         ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

  TIM_TimeBaseStructInit(&tim);
  tim.TIM_CounterMode = TIM_CounterMode_Up;
  tim.TIM_Prescaler = 3600;
  tim.TIM_Period = 100 - 1;
  TIM_TimeBaseInit(TIM2, &tim);

  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
  TIM_Cmd(TIM2, ENABLE);

  nvic.NVIC_IRQChannel = TIM2_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 0;
  nvic.NVIC_IRQChannelSubPriority = 0;
  nvic.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvic);
}

int main(void) {
  char przyciskFlag = 0;
  SysTick_Config(SystemCoreClock / 1000);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

  PWM_Init();
  ENCODER_Init();
  CZUJNIKI_Init();
  przyciskInit();
  ledInit();
  ADC1_Init();
  ADC_Calib();

  double voltage;
  uint16_t adc;

  TIM_SetCompare2(TIM4, 0); // lewy
  TIM_SetCompare4(TIM4, 0); // prawy

  int i;
  initPola();
  struct Stack stos;
  stos.size = 0;
  pola[0].s3 = -1;
  stackPush(&stos, 0);
  
#define PEEK stackPeek(&stos)

  while (pola[PEEK].odleglosc != 0) {
    adc = adc_read(ADC_Channel_8);
    voltage = adc * 1.0 / 4096 * 3.3;
    if (voltage < 2) { // ZMIERZYC GRANICZNA WARTOSC PO ZLOZENIU ROBOTA
      GPIO_SetBits(GPIOC, GPIO_Pin_15);
    } else
      GPIO_ResetBits(GPIOC, GPIO_Pin_15);

    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 0) {
      delay_ms(50);
      if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 0) {
        if (1 /*przyciskFlag*/) {
          // target1+=80;
          // target2+=80;
          przyciskFlag = 1;
          GPIO_SetBits(GPIOC, GPIO_Pin_13);
        }
      }
    } else {
      // przyciskFlag = 1;
      GPIO_ResetBits(GPIOC, GPIO_Pin_13);
    }

    // regulator
    temp1 = (TIM_GetCounter(TIM3)); // prawy
    temp2 = (TIM_GetCounter(TIM1)); // lewy
    if (temp1 != previousTemp1) {   // obroty1 to prawy SILNIK >????
      obroty1 += temp1 > previousTemp1 ? 1 : -1; //
      if (temp1 > previousTemp1 || ((previousTemp1 == 5) && (temp1 == 0))) {
        // GPIO_SetBits(GPIOC, GPIO_Pin_15);
      } else {
        // GPIO_ResetBits(GPIOC, GPIO_Pin_15);
      }
      previousTemp1 = temp1;
    }
    if (temp2 != previousTemp2) {
      obroty2 += temp2 < previousTemp2 ? 1 : -1;
      if (temp2 < previousTemp2 || ((previousTemp2 == 0) && (temp2 == 5))) {
        // GPIO_SetBits(GPIOC, GPIO_Pin_13);
      } else {
        // GPIO_ResetBits(GPIOC, GPIO_Pin_13);
      }
      previousTemp2 = temp2;
    }

    uchyb1 = target1 - obroty1;
    uchyb2 = target2 - obroty2;

    if (uchyb2 > 0) {
      LEWY_PRZOD;
    } else {
      LEWY_TYL;
    }

    if (uchyb1 > 0) {
      PRAWY_PRZOD;
    } else {
      PRAWY_TYL;
    }

    TIM_SetCompare2(
        TIM4, uchyb2 > 0
                  ? K * (1 + Td * ((uchyb2 - uchybPrev2) / (0.001))) * uchyb2
                  : -K * (1 + Td * ((uchyb2 - uchybPrev2) / (0.001))) * uchyb2);
    TIM_SetCompare4(
        TIM4, uchyb1 > 0
                  ? K * (1 + Td * ((uchyb1 - uchybPrev1) / (0.001))) * uchyb1
                  : -K * (1 + Td * ((uchyb1 - uchybPrev1) / (0.001))) * uchyb1);
    uchybPrev1 = uchyb1;
    uchybPrev2 = uchyb2;

    /// koniec regulatora

    if (abs(uchyb1) < 25 && abs(uchyb2) < 25 && przyciskFlag) {
      sprawdzSciany(PEEK);
      if (checkPole(PEEK) == 1) {
        for (i = stos.size - 2; i >= 0; i--) {
          if (checkPole(stos.stack[i]) == 0) {
            break;
          }
        }
        char check = 1;
        while (check) {
          check = 0;
          for (i = 0; i < LABIRYNT_SIZE; i++) {
            check |= checkPole(i); //?
          }
        }
      }

      struct WhereToGo next = findPath(PEEK);
      char powrot;
      switch (next.kierunek) {
      case 'W': {
        powrot = previousKierunek == 'E' ? 1 : 0;
        break;
      }
      case 'N': {
        powrot = previousKierunek == 'S' ? 1 : 0;
        break;
      }
      case 'E': {
        powrot = previousKierunek == 'W' ? 1 : 0;
        break;
      }
      case 'S': {
        powrot = previousKierunek == 'N' ? 1 : 0;
        break;
      }
      }
      if (powrot) {
        stackPop(&stos);
      } else {
        stackPush(&stos, next.numer);
      }

      go(next.kierunek);
      previousKierunek = next.kierunek;
    }

    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 0 ||
        GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4) == 0 ||
        GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4) == 0) {
      // GPIO_SetBits(GPIOC, GPIO_Pin_15);
    } else {
      // GPIO_ResetBits(GPIOC, GPIO_Pin_15);
    }
  }

  void TIM2_IRQHandler() {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET) {
      TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
      time += 5;
    }
  }
}
