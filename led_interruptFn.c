#include "stm8s.h"
#define __STDINT__
#include "UsartHardware.h"
#include "led_interruptFn.h"
#include "led_function.h"
#include "irq.h"
#include <math.h>

extern volatile uint8_t ledPower;
extern volatile uint8_t cutOffTemp;
extern volatile uint8_t isLEDCutOff;
@svlreg @interrupt void usartInterrupt(void){
	disableIRQ();
	usartIrqHandler(MAIN_CONTROLLER);
	enableIRQ();
}

@svlreg @interrupt void timer2Interrupt(void){
	disableIRQ();
	//setup adc conversion for temperature
	ADC1_ConversionConfig  (ADC1_CONVERSIONMODE_SINGLE ,  
													ADC1_CHANNEL_2 ,  
													ADC1_ALIGN_RIGHT); 
	ADC1_ITConfig(ADC1_IT_EOCIE ,ENABLE); 
	ADC1_StartConversion();
	TIM2_ClearFlag(TIM2_FLAG_UPDATE);
	TIM2_ClearITPendingBit(TIM2_IT_UPDATE);
	enableIRQ();
}

@svlreg @interrupt void tempADCInterrupt(void){

	//setup adc conversion for temperature
	uint16_t adcValue;
	double denomOfTempEquation;
	float tempValue;
	disableIRQ();
	adcValue = ADC1_GetConversionValue();
	denomOfTempEquation = (1/337.3)+(1/3950.00)*log((1023.00/adcValue)-1);
	
	ADC1_ClearFlag(ADC1_FLAG_EOC);
	tempValue = (1/denomOfTempEquation)-273.15;
	if(tempValue > cutOffTemp){
		isLEDCutOff = 1;
	}	
	configureLEDIntensity();
	ADC1_ITConfig(ADC1_IT_EOCIE ,DISABLE);
	ADC1_ClearFlag(ADC1_FLAG_EOC);
	ADC1_ClearITPendingBit(ADC1_IT_EOC);
	enableIRQ();
}

