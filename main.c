/* MAIN.C file
 * 
 * Copyright (c) 2002-2005 STMicroelectronics
 */
#include "stm8s.h"
#define __STDINT__
#include "Event.h"
#include "CommEventQueue.h"
#include "StateMachine.h"
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
//testing
GenericStateMachine sm;
UsartEvent uEvent;
double testTempValue;
double testVoltageValue;
double testCurrentValue;
char testData[10];
//ori
Event * event;
uint8_t sizeFloat;
int main(void)
{
	disableIRQ();
	GPIO_setup();
	clock_setup();
	TIM1_setup();
	ADC1_setup();
	UART1_setup();
	TIM2_setup();
	usartInit();
	sizeFloat = sizeof(float);
	enableIRQ();
	/*
	setLEDIntensity(10);
	/*
	setLEDIntensity(10);
	testTempValue = getTemperature();
	testVoltageValue = getVoltage();
	testCurrentValue = getCurrent();
	//TIM1_SetCompare3(value) ;
	GPIO_WriteHigh(GPIOD,GPIO_PIN_4);
	*/
	//test
	/*
	sm.callback = (Callback)configureLEDIntensity;
	uEvent.stateMachineInfo = &sm;
	testData[5]= 123;
	uEvent.buffer = (uint8_t*)testData;
	configureLEDIntensity((Event*)&uEvent);
	*/
	while (1){
		//GPIO_WriteHigh(GPIOC,GPIO_PIN_7);
		//UART1_SendData8(170);
		//GPIO_WriteHigh(GPIOC,GPIO_PIN_7);
		/*
		UART1_SendData8(123);
		GPIO_WriteHigh(GPIOC,GPIO_PIN_7);
		*/
		//configureLEDIntensity((Event*)&uEvent);
		if(eventDequeue(&sysQueue,&event))
			event->stateMachine->callback(event);
		else if(eventDequeue(&evtQueue,&event))
			event->stateMachine->callback(event);
		/*
		testTempValue = getTemperature();

		testData = UART1_ReceiveData8();
		*/
}		
}

