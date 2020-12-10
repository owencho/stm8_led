/* MAIN.C file
 * 
 * Copyright (c) 2002-2005 STMicroelectronics
 */
#include "stm8s.h"
#define __STDINT__
#include "Event.h"
#include "CommEventQueue.h"
#include "UsartHardware.h"
#include "UsartDriver.h"
#include "EventQueue.h"
#include "led_function.h"
#include "led_setup.h"
//#include <stdio.h>
//#include <stdarg.h>
//#include <string.h>
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
char testData[10];
Event * event;
int main(void)
{
	enableInterrupts();
	GPIO_setup();
	clock_setup();
	TIM1_setup();
	UART1_setup();
	ADC1_setup();
	usartInit();
	/*
	setLEDIntensity(10);
	testTempValue = getTemperature();
	testVoltageValue = getVoltage();
	testCurrentValue = getCurrent();
	//TIM1_SetCompare3(value) ;
	GPIO_WriteHigh(GPIOD,GPIO_PIN_4);
	*/
	testData[5]= 123;
	configureLEDIntensity(testData);
	while (1){
		if(eventDequeue(&sysQueue,&event))
			event->stateMachine->callback(event);
		else if(eventDequeue(&evtQueue,&event))
			event->stateMachine->callback(event);
	}
}


