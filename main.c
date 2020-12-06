/* MAIN.C file
 * 
 * Copyright (c) 2002-2005 STMicroelectronics
 */
#include "stm8s.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

void GPIO_setup(void)
{
	GPIO_DeInit(GPIOC);
	GPIO_Init(GPIOC, (GPIO_PIN_3),
						GPIO_MODE_OUT_PP_HIGH_FAST);
	GPIO_Init(GPIOC, (GPIO_PIN_4 ),
						GPIO_MODE_IN_PU_NO_IT);
						
	GPIO_DeInit(GPIOD);
	GPIO_Init(GPIOD,(GPIO_PIN_2 | GPIO_PIN_3|GPIO_PIN_4|
						GPIO_PIN_5|GPIO_PIN_6),GPIO_MODE_OUT_PP_HIGH_FAST);
}
void clock_setup(void)
{
	CLK_DeInit();
						
	CLK_HSECmd(DISABLE);
	CLK_LSICmd(DISABLE);
	CLK_HSICmd(ENABLE);
	while(CLK_GetFlagStatus(CLK_FLAG_HSIRDY) == FALSE);
						
	CLK_ClockSwitchCmd(ENABLE);
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
	CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1);
						
	CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO, CLK_SOURCE_HSI, 
	DISABLE, CLK_CURRENTCLOCKSTATE_ENABLE);
						
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_ADC, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART1, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, ENABLE);
}

#define TIM1_PRESCALER_1 ((u16) 0x00)
void TIM1_setup(void)
{               
	TIM1_DeInit();
	//Init the period 

	TIM1_TimeBaseInit(TIM1_PRESCALER_1,TIM1_COUNTERMODE_UP,799,0);
	// 1/frequency, count value 800, frequency=16M/800=20kHZ
	
	TIM1_OC3Init(TIM1_OCMODE_PWM1, TIM1_OUTPUTSTATE_ENABLE, 
								TIM1_OUTPUTNSTATE_ENABLE,0xFFFF, 
								TIM1_OCPOLARITY_LOW, TIM1_OCNPOLARITY_HIGH,
								TIM1_OCIDLESTATE_SET,
								TIM1_OCNIDLESTATE_RESET);
							
	TIM1_Cmd(ENABLE);
	TIM1_CtrlPWMOutputs(ENABLE);
}

void UART1_setup(void)
{               
	UART1_DeInit();

	
	UART1_Init(115200, UART1_WORDLENGTH_8D, 
						UART1_STOPBITS_1 ,UART1_PARITY_NO, 
						UART1_SYNCMODE_CLOCK_DISABLE , UART1_MODE_TXRX_ENABLE);

							
	UART1_Cmd(ENABLE);
}

void ADC1_setup(void)
{               
	ADC1_DeInit();
	ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS,  
					ADC1_CHANNEL_2,  
					ADC1_PRESSEL_FCPU_D2 ,  
					ADC1_EXTTRIG_GPIO,  
					DISABLE,  
					ADC1_ALIGN_RIGHT ,  
					ADC1_SCHMITTTRIG_ALL ,  
					DISABLE); 
	
	ADC1_Cmd(ENABLE);
}

void Serial_print_int (int number)  {
	char count = 0;
	char digit[5] = "";         
	while (number != 0) //split the int to char array
	{
		digit[count] = number%10;
		count++;
		number = number/10;
	}
	while (count !=0) //print char array in correct direction
	{
		UART1_SendData8(digit[count-1] + 0x30);
		while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET); 
		count--;
	}
}
double getTemperature(void){
	uint16_t adcValue;
	double denomOfTempEquation;
	ADC1_ConversionConfig  ( 	ADC1_CONVERSIONMODE_SINGLE ,  
														ADC1_CHANNEL_2 ,  
														ADC1_ALIGN_RIGHT); 
	ADC1_StartConversion();
	while(ADC1_GetFlagStatus(ADC1_FLAG_EOC) == FALSE);
	adcValue = ADC1_GetConversionValue();
	denomOfTempEquation = (1/337.3)+(1/3950.00)
												*log((1023.00/adcValue)-1);
	
	//Serial_print_int (adcValue);
	ADC1_ClearFlag(ADC1_FLAG_EOC);
	return (1/denomOfTempEquation)-273.15;
	//return adcValue;
}

double getVoltage(void){
	uint16_t adcValue;
	ADC1_ConversionConfig  ( 	ADC1_CONVERSIONMODE_SINGLE ,  
														ADC1_CHANNEL_4 ,  
														ADC1_ALIGN_RIGHT); 
	ADC1_StartConversion();
	while(ADC1_GetFlagStatus(ADC1_FLAG_EOC) == FALSE);
	adcValue = ADC1_GetConversionValue();
	ADC1_ClearFlag(ADC1_FLAG_EOC);
	return (adcValue/1023.00)*5;
}
#define SENSE_RESISTOR 0.01096
#define OP_AMP_GAIN 168.98

double getCurrent(void){
	uint16_t adcValue;
	uint16_t voltageADC;
	ADC1_ConversionConfig  ( 	ADC1_CONVERSIONMODE_SINGLE ,  
														ADC1_CHANNEL_3 ,  
														ADC1_ALIGN_RIGHT); 
	ADC1_StartConversion();
	while(ADC1_GetFlagStatus(ADC1_FLAG_EOC) == FALSE);
	adcValue = ADC1_GetConversionValue();
	voltageADC = (adcValue/1023.00)*5;
	ADC1_ClearFlag(ADC1_FLAG_EOC);
	return voltageADC/(OP_AMP_GAIN*SENSE_RESISTOR);
}

void setLEDIntensity(uint16_t inputValue){
	double calValue;
	uint32_t value;
	calValue = 800.00-((inputValue/255.00)*800.00);
	value= (uint32_t)calValue;
	TIM1_SetCompare3(value) ;
	//Serial_print_int (value);
}

double testTempValue;
double testVoltageValue;
double testCurrentValue;
int main(void)
{
	GPIO_setup();
	clock_setup();
	TIM1_setup();
	UART1_setup();
	ADC1_setup();
	setLEDIntensity(200);
	testTempValue = getTemperature();
	testVoltageValue = getVoltage();
	testCurrentValue = getCurrent();
	//TIM1_SetCompare3(value) ;
	GPIO_WriteHigh(GPIOD,GPIO_PIN_4);
	while (1){
		//Serial_print_int (value);
	}
}


