#include "stm8s.h"
#define __STDINT__
#include "led_function.h"
#include "irq.h"
#include "UsartDriver.h"
#include <math.h>
#include <string.h>

uint8_t ledIntensity = 0;
uint8_t ledPower = 0;
volatile uint8_t cutOffTemp;
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
	
	ADC1_ClearFlag(ADC1_FLAG_EOC);
	return (1/denomOfTempEquation)-273.15;
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
	return (adcValue/1023.00)*80;
}
#define SENSE_RESISTOR 0.01096
#define OP_AMP_GAIN 168.98

double getCurrent(void){
	uint16_t adcValue;
	double voltageADC;
	ADC1_ConversionConfig  ( 	ADC1_CONVERSIONMODE_SINGLE ,  
														ADC1_CHANNEL_3 ,  
														ADC1_ALIGN_RIGHT); 
	ADC1_StartConversion();
	while(ADC1_GetFlagStatus(ADC1_FLAG_EOC) == FALSE);
	adcValue = ADC1_GetConversionValue();
	voltageADC = (adcValue/1023.00)*5.00;
	ADC1_ClearFlag(ADC1_FLAG_EOC);
	return voltageADC/(OP_AMP_GAIN*SENSE_RESISTOR);
}

void setLEDIntensity(uint16_t inputValue){
	double calValue;
	uint32_t value;
	calValue = ((inputValue/255.00)*7999.00);
	value= (uint32_t)calValue;
	TIM1_SetCompare3(value) ;
}

void setLEDPower(uint8_t inputValue){
	if(inputValue == 1){
		setLEDIntensity(ledIntensity);
	}
	else if (inputValue ==0){
		setLEDIntensity(0);
	}
}


LedFunctionState ledIntensityState = LED_FN_IDLE;
uint8_t ledIntensityData[5];
void configureLEDIntensity(Event * event){
	UsartEvent * usartEvent = (UsartEvent*)event;
	char * data = usartEvent->buffer;
	disableIRQ();
	switch(ledIntensityState){
        case LED_FN_IDLE :
						ledIntensity = data[5];
						if(ledPower==1){
							setLEDIntensity(ledIntensity);
						}	
						ledIntensityData[0] = 1; //command
						usartDriverTransmit(MAIN_CONTROLLER,MASTER_ADDRESS
																,1,ledIntensityData,usartEvent);
						ledIntensityState = LED_FN_REPLY_PACKET;
            break;
        case LED_FN_REPLY_PACKET:
            ledIntensityState = LED_FN_IDLE;
						setNoMoreUsartEvent();
            break;
    }	
		enableIRQ();
}

LedFunctionState ledPowerState = LED_FN_IDLE;
uint8_t ledPowerData[5];
void configureLEDPower(Event * event){
	UsartEvent * usartEvent = (UsartEvent*)event;
	char * data = usartEvent->buffer;
	disableIRQ();
	switch(ledPowerState){
        case LED_FN_IDLE :
						ledPower = data[5];
						setLEDPower(ledPower);
						ledPowerData[0] = 0; //command
						usartDriverTransmit(MAIN_CONTROLLER,MASTER_ADDRESS
																,1,ledPowerData,usartEvent);
						ledPowerState = LED_FN_REPLY_PACKET;
            break;
        case LED_FN_REPLY_PACKET:
            ledPowerState = LED_FN_IDLE;
						setNoMoreUsartEvent();
            break;
    }	
		enableIRQ();
}

LedFunctionState ledCutOffState = LED_FN_IDLE;
uint8_t cutOffData[5];
void configureLEDCutOffTemp(Event * event){
	UsartEvent * usartEvent = (UsartEvent*)event;
	char * data = usartEvent->buffer;
	disableIRQ();
	switch(ledCutOffState){
        case LED_FN_IDLE :
						cutOffTemp = data[5];
						cutOffData[0] = 4; //command
						usartDriverTransmit(MAIN_CONTROLLER,MASTER_ADDRESS
																,1,cutOffData,usartEvent);
						ledCutOffState = LED_FN_REPLY_PACKET;
            break;
        case LED_FN_REPLY_PACKET:
            ledCutOffState = LED_FN_IDLE;
						setNoMoreUsartEvent();
            break;
    }	
		enableIRQ();
}


