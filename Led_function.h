#ifndef __GETVALUE_H
#define __GETVALUE_H
#include "Event.h"
void disableAutoCutOffTemp(void);
void enableAutoCutOffTemp(void);
double getTemperature(void);
double getVoltage(void);
double getCurrent(void);
void setLEDPower(uint8_t inputValue);
void setLEDIntensity(uint8_t intValue);

void getTemperatureSM(Event * event);
void configureLEDIntensity(Event * event);
void configureLEDPower(Event * event);
void configureLEDCutOffTemp(Event * event);
void getElectricalParameterSM(Event * event);
typedef enum{
	LED_FN_IDLE,
	LED_FN_RESENT_PACKET,
	LED_FN_REPLY_PACKET
} LedFunctionState;


#endif /* __GETVALUE_H */