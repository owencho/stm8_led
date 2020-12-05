/* MAIN.C file
 * 
 * Copyright (c) 2002-2005 STMicroelectronics
 */
#include "stm8s.h"
/*
void GPIO_setup(void)
{
	GPIO_DeInit(GPIOC);
	GPIO_Init(GPIOC, (GPIO_PIN_4 | GPIO_PIN_3),
						GPIO_MODE_OUT_PP_HIGH_FAST);
						
	GPIO_DeInit(GPIOD);
	GPIO_Init(GPIOD,(GPIO_PIN_2 | GPIO_PIN_3|GPIO_PIN_4|
						GPIO_PIN_5),GPIO_MODE_OUT_PP_HIGH_FAST);
}
void clock_setup(void)
{
	CLK_DeInit();
						
	CLK_HSECmd(DISABLE);
	CLK_LSICmd(DISABLE);
	CLK_HSICmd(ENABLE);
	while(CLK_GetFlagStatus(CLK_FLAG_HSIRDY) == FALSE);
						
	CLK_ClockSwitchCmd(ENABLE);
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV8);
	CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1);
						
	CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO, CLK_SOURCE_HSI, 
	DISABLE, CLK_CURRENTCLOCKSTATE_ENABLE);
						
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_ADC, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART1, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, ENABLE);
}

void TIM1_setup(void)
{               
	TIM1_DeInit();
	//Init the period 
	TIM1_TimeBaseInit(0,TIM1_COUNTERMODE_UP , 7999,0); 
	/*1/frequency, count value 8000, frequency=16M/8000=2kHZ*/
	/*
	TIM1_OC3Init(TIM1_OCMODE_PWM1, TIM1_OUTPUTSTATE_ENABLE, 
								TIM1_OUTPUTNSTATE_ENABLE,2000, 
								TIM1_OCPOLARITY_LOW, TIM1_OCNPOLARITY_HIGH,
								TIM1_OCIDLESTATE_SET,
								TIM1_OCNIDLESTATE_RESET);
	/*4000/2000 * 100% = 50% duty cycle*/
	/*							
	TIM1_Cmd(ENABLE);
	TIM1_CtrlPWMOutputs(ENABLE);
}
/*
int main(void)
{
	GPIO_setup();
	clock_setup();
	TIM1_setup();
	GPIO_WriteHigh(GPIOD,GPIO_PIN_4);
	while (1);
}

*/

//#include "stm8s.h"

main() {
    
	GPIO_Init(GPIOD, GPIO_PIN_ALL ,GPIO_MODE_OUT_PP_HIGH_SLOW);
	GPIO_WriteHigh(GPIOD,GPIO_PIN_4);
	while (1);
}
