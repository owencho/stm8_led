#include "stm8s.h"
#include "led_function.h"
#include <math.h>

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
	calValue = ((inputValue/255.00)*800.00);
	value= (uint32_t)calValue;
	TIM1_SetCompare3(value) ;
}
