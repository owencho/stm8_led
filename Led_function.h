#ifndef __GETVALUE_H
#define __GETVALUE_H
#include "Event.h"
void disableAutoCutOffTemp(void);
void enableAutoCutOffTemp(void);
double getTemperature(void);
double getVoltage(void);
double getCurrent(void);
void configureLEDIntensity(void);
void setLEDIntensity(uint8_t intValue);

void getTemperatureSM(Event * event);
void ledIntensitySM(Event * event);
void ledPowerSM(Event * event);
void ledCutOffTempSM(Event * event);
void getElectricalParameterSM(Event * event);
typedef enum{
	LED_FN_IDLE,
	LED_FN_RESENT_PACKET,
	LED_FN_REPLY_PACKET
} LedFunctionState;


#endif /* __GETVALUE_H */