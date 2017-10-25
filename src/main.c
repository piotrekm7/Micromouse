#include "stm32f10x.h"
#include <math.h>

/**
 * Definitions
 */
#define INF 32000

#define LABIRYNT_SIZE (8 * 8)

#define LEWY_PRZOD (GPIO_SetBits(GPIOB, GPIO_Pin_6))
#define LEWY_TYL (GPIO_ResetBits(GPIOB, GPIO_Pin_6))
#define PRAWY_PRZOD (GPIO_SetBits(GPIOB, GPIO_Pin_8))
#define PRAWY_TYL (GPIO_ResetBits(GPIOB, GPIO_Pin_8))

/*
 * Variables
 */
volatile uint32_t timer_ms = 0;

volatile int swtime=0;
volatile char swflag = 0, przyciskFlag = 0;

volatile int time = 0;

char previousKierunek = 'N';

int obroty1, obroty2, temp1, temp2, previousTemp1, previousTemp2, target1,
		target2, uchyb1, uchyb2, uchybPrev1, uchybPrev2; // 1 - left motor

double K = 0.03, Td = 0.05, Ti = 15, uchybSum1, uchybSum2;

void controllerInit() {
	obroty1 = obroty2 = temp1 = temp2 = previousTemp1 = previousTemp2 =
			target1 = target2 = uchyb1 = uchyb2 = uchybPrev1 = uchybPrev2 = 0;
	uchybSum1 = uchybSum2 = 0;
}

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

void stackPop(struct Stack *stack) {
	stack->size--;
}

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

	sciany = 100 * (~GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12))
			+ 10 * (~GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4))
			+ (~GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4));

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

void go(char nextKierunek) {
	int direction; // 0-forward,1-left,2-right,3-back
	if (nextKierunek == previousKierunek) {
		direction = 0;
	} else if ((nextKierunek == 'N' && previousKierunek == 'S')
			|| (nextKierunek == 'S' && previousKierunek == 'N')
			|| (nextKierunek == 'E' && previousKierunek == 'W')
			|| (nextKierunek == 'W' && previousKierunek == 'E')) {
		direction = 3;
	} else if ((nextKierunek == 'N' && previousKierunek == 'E')
			|| (nextKierunek == 'E' && previousKierunek == 'S')
			|| (nextKierunek == 'W' && previousKierunek == 'N')
			|| (nextKierunek == 'S' && previousKierunek == 'W')) {
		direction = 1;
	} else {
		direction = 2;
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

/**
 * Hardware Initializations
 */

void pwmInit() {
	GPIO_InitTypeDef gpio;
	TIM_TimeBaseInitTypeDef tim;
	TIM_OCInitTypeDef channel;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
	RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
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

void encoderInit() {
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

	TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12,
	TIM_ICPolarity_Falling,
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

	TIM_EncoderInterfaceConfig(TIM1, TIM_EncoderMode_TI12,
	TIM_ICPolarity_Falling,
	TIM_ICPolarity_Falling);

	TIM_SetCounter(TIM1, 0);

	TIM_Cmd(TIM1, ENABLE);
}

void czujnikiInit() {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
	RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);

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
void extint_Init(){
	GPIO_InitTypeDef gpio;
	EXTI_InitTypeDef exti;
	NVIC_InitTypeDef nvic;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	gpio.GPIO_Pin = GPIO_Pin_14;
	gpio.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &gpio);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource14);
	EXTI_StructInit(&exti);
	exti.EXTI_Line = EXTI_Line14;
	exti.EXTI_Mode = EXTI_Mode_Interrupt;
	exti.EXTI_Trigger = EXTI_Trigger_Falling;
	exti.EXTI_LineCmd = ENABLE;
	EXTI_Init(&exti);

	nvic.NVIC_IRQChannel = EXTI15_10_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0x00;
	nvic.NVIC_IRQChannelSubPriority = 0x00;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
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

void adc1Init() {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
	RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
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

void timerInit() {
	// GPIO_InitTypeDef gpio;
	TIM_TimeBaseInitTypeDef tim;
	NVIC_InitTypeDef nvic;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
	RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
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

/**
 * Hardware functions
 */
void EXTI15_10_IRQHandler(){
	if (EXTI_GetITStatus(EXTI_Line14)) {
		if(!swflag) {
			swtime = 0;
			przyciskFlag=!przyciskFlag;
			swflag=1;
		}
		EXTI_ClearITPendingBit(EXTI_Line14);
	}
}

void TIM2_IRQHandler() {
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		time += 5;
		swtime +=5;
	}
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
void adc_calib() {
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

int main(void) {
	/**
	 * Init calls
	 */
	SysTick_Config(SystemCoreClock / 1000);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	pwmInit();
	encoderInit();
	adc1Init();
	adc_calib();
	przyciskInit();
	czujnikiInit();
	ledInit();
	timerInit();
	/**
	 * End
	 */

	double voltage;
	uint16_t adc;

	TIM_SetCompare2(TIM4, 0); // lewy
	TIM_SetCompare4(TIM4, 0); // prawy

	int i = 0;
	initPola();
	struct Stack stos;
	stos.size = 0;
	pola[0].s3 = -1;
	stackPush(&stos, 0);

#define PEEK stackPeek(&stos)

	controllerInit();
	int lamp = 0;
	while (pola[PEEK].odleglosc != 0 || uchyb1!=0 || uchyb2!=0) {

		/**
		 * Checking battery voltage to avoid lipo damage and flame of destruction
		 */
		/*
		 adc = adc_read(ADC_Channel_8);
		 voltage = adc * 1.0 / 4096 * 3.3;
		 if (voltage < 2) { // ZMIERZYC GRANICZNA WARTOSC PO ZLOZENIU ROBOTA
		 //GPIO_SetBits(GPIOC, GPIO_Pin_15);
		 } else
		 */
		//GPIO_ResetBits(GPIOC, GPIO_Pin_15);

		if(swtime>150) swflag=0; //delay to deal with switch bounce
		if(!przyciskFlag){
			/*
			 * Resetting all necessary variables to make the robot solve another maze
			 */
			uchyb1=uchyb2=target1=target2=obroty1=obroty2=0;
			stos.size = 0;
			pola[0].s3 = -1;
			stackPush(&stos, 0);
		}
		/**
		 * Check if switch is pushed
		 */
		/*
		if (!przyciskFlag && GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 0) {
			delay_ms(30); // check again after delay to deal with switch bounce
			if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 0) {
				przyciskFlag = 1;
				uchyb1=uchyb2=target1=target2=obroty1=obroty2=0;
				GPIO_SetBits(GPIOC, GPIO_Pin_13);
			}
		} else {
			GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		}
*/
		temp1 = (TIM_GetCounter(TIM1)); // left motor is marked as 1
		temp2 = (TIM_GetCounter(TIM3)); // right motor - 2
		// update current position of robot based on encoders signals
		if (temp1 != previousTemp1) {
			obroty1 += (temp1 < previousTemp1 || (temp1==5&&previousTemp1==0)) ? 1 : -1;
			previousTemp1 = temp1;
		}
		if (temp2 != previousTemp2) {
			obroty2 += (temp2 > previousTemp2 || (temp2==0&&previousTemp2==5)) ? 1 : -1;
			previousTemp2 = temp2;
		}

		/**timeStep is time beetween signals calculations , used for derivative and
		 * integral. It isn't done the correct way , the time beetween calculations
		 * will differ, there should be an interrupt which can invoke signals
		 * calculation, we'll introduce it in future
		 */
		int timeStep = 50; // in miliseconds, it must be divided by 1000 in calculation

		/**
		 * PID regulator
		 */
		if (time > timeStep) {
			time = 0;

			// count errors, used for PID regulator
			uchyb1 = target1 - obroty1;
			uchyb2 = target2 - obroty2;

			double signal1, signal2; // input signals to motors

			uchybSum1 += 1.0*timeStep / 1000 * uchyb1;
			uchybSum2 += 1.0*timeStep / 1000 * uchyb2;

			signal1 = 1.0 * K
					* (1 + Td * (uchyb1 - uchybPrev1) / (1.0 * timeStep / 1000)
							+ Ti * fabs(uchybSum1)) * uchyb1;

			signal2 = 1.0 * K
					* (1 + Td * (uchyb2 - uchybPrev2) / (1.0 * timeStep / 1000)
							+ Ti * fabs(uchybSum2)) * uchyb2;

			signal1 > 0 ? LEWY_PRZOD : LEWY_TYL;
			signal2 > 0 ? PRAWY_PRZOD : PRAWY_TYL;
			if (lamp == 0) {
				GPIO_SetBits(GPIOC, GPIO_Pin_15);
			} else {
				GPIO_ResetBits(GPIOC, GPIO_Pin_15);
			}
			lamp = !lamp;
			TIM_SetCompare2(TIM4, fmin(300,fabs(signal1)));
			TIM_SetCompare4(TIM4, fmin(300,fabs(signal2)));
			uchybPrev1 = uchyb1;
			uchybPrev2 = uchyb2;
		}

		/**
		 * Check if robot is at desired position
		 * if so get next target
		 */
		if (fabs(uchyb1) < 3 && fabs(uchyb2) < 3 && przyciskFlag && pola[PEEK].odleglosc != 0) {
			uchybSum1 = uchybSum2 = 0; // reset integrals
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
			uchyb1 = target1 - obroty1;
			uchyb2 = target2 - obroty2;
			delay_ms(50);
		}
	}
	while (1) {
	}
}
