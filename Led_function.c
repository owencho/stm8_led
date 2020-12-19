#include "stm8s.h"
#define __STDINT__
#include "led_function.h"
#include "irq.h"
#include "UsartDriver.h"
#include <math.h>
#include <string.h>

volatile uint8_t ledIntensity = 0;
volatile uint8_t ledPower = 1;
volatile uint8_t cutOffTemp = 110;
volatile uint8_t isLEDCutOff = 0;

void disableAutoCutOffTemp(void){
	TIM2_ITConfig(TIM2_IT_UPDATE,DISABLE);
	ADC1_ITConfig(ADC1_IT_EOCIE ,DISABLE);
	ADC1_ClearFlag(ADC1_FLAG_EOC);
	ADC1_ClearITPendingBit(ADC1_IT_EOC);
	TIM2_ClearFlag(TIM2_FLAG_UPDATE);
	TIM2_ClearITPendingBit(TIM2_IT_UPDATE);
}

void enableAutoCutOffTemp(void){
	ADC1_ClearFlag(ADC1_FLAG_EOC);
	ADC1_ClearITPendingBit(ADC1_IT_EOC);
	TIM2_ClearFlag(TIM2_FLAG_UPDATE);
	TIM2_ClearITPendingBit(TIM2_IT_UPDATE);
	TIM2_ITConfig(TIM2_IT_UPDATE,ENABLE);
	ADC1_ITConfig(ADC1_IT_EOCIE ,ENABLE);
	ADC1_ConversionConfig  (ADC1_CONVERSIONMODE_SINGLE ,  
													ADC1_CHANNEL_2 ,  
													ADC1_ALIGN_RIGHT); 
	ADC1_StartConversion();

}	
float getTemperature(void){
	uint16_t adcValue;
	float denomOfTempEquation;
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

float getVoltage(void){
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

float getCurrent(void){
	uint16_t adcValue;
	float voltageADC;
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

void setLEDIntensity(uint8_t intValue){
	float calValue;
	uint32_t value;
	disableIRQ();
	calValue = ((intValue/255.00)*8000.00);
	value= (uint32_t)calValue;
	TIM1_SetCompare3(value) ;
	enableIRQ();
}

void setLEDPower(uint8_t inputValue){
	disableIRQ();
	if(inputValue == 1){
		setLEDIntensity(ledIntensity);
		isLEDCutOff = 0;
		ledPower = 1;
	}
	else if (inputValue ==0){
		setLEDIntensity(0);
		ledPower = 0;
	}
	enableIRQ();
}

LedFunctionState getEPState = LED_FN_IDLE;
uint8_t getEPData[10];
void getElectricalParameterSM(Event * event){
	UsartEvent * usartEvent = (UsartEvent*)event;
	char * data = usartEvent->buffer;
	float voltageValue;
	float currentValue;
	disableIRQ();
	switch(getEPState){
        case LED_FN_IDLE :
						disableAutoCutOffTemp();
						voltageValue = getVoltage();
						currentValue = getCurrent();
						enableAutoCutOffTemp();
						getEPData[0] = 2; //command
						*(float*)&getEPData[1] = voltageValue;
						*(float*)&getEPData[5] = currentValue;
						usartDriverTransmit(MAIN_CONTROLLER,MASTER_ADDRESS
																,9,getEPData,usartEvent);
						getEPState = LED_FN_REPLY_PACKET;
            break;
        case LED_FN_REPLY_PACKET:
            getEPState = LED_FN_IDLE;
						setNoMoreUsartEvent();
            break;
    }	
		enableIRQ();
}

LedFunctionState getTempState = LED_FN_IDLE;
uint8_t getTempData[8];
void getTemperatureSM(Event * event){
	UsartEvent * usartEvent = (UsartEvent*)event;
	char * data = usartEvent->buffer;
	float tempValue;
	uint8_t *array;
	disableIRQ();
	switch(getTempState){
        case LED_FN_IDLE :
						disableAutoCutOffTemp();
						tempValue = getTemperature();
						enableAutoCutOffTemp();
						getTempData[0] = 3; //command
						getTempData[1] = isLEDCutOff; //is LED cut off
						*(float*)&getTempData[2] = tempValue;
						usartDriverTransmit(MAIN_CONTROLLER,MASTER_ADDRESS
																,6,getTempData,usartEvent);
						getTempState = LED_FN_REPLY_PACKET;
            break;
        case LED_FN_REPLY_PACKET:
            getTempState = LED_FN_IDLE;
						setNoMoreUsartEvent();
            break;
    }	
		enableIRQ();
}

LedFunctionState ledIntensityState = LED_FN_IDLE;
volatile uint8_t ledIntensityData[5];
volatile uint8_t resentCounter= 0 ;
void configureLEDIntensity(Event * event){
	UsartEvent * usartEvent = (UsartEvent*)event;
	char * data = usartEvent->buffer;
	disableIRQ();
	switch(ledIntensityState){
        case LED_FN_IDLE :
						ledIntensity = data[DATA_OFFSET];
						if(ledPower==1 && isLEDCutOff==0){
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
						ledPower = data[DATA_OFFSET];
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
						cutOffTemp = data[DATA_OFFSET];
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


