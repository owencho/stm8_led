/* MAIN.C file
 * 
 * Copyright (c) 2002-2005 STMicroelectronics
 */
#include "stm8s.h"
#include "led_function.h"
#include "led_setup.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

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


