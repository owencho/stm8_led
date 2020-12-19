#ifndef __GETVALUE_H
#define __GETVALUE_H
#include "Event.h"
double getTemperature(void);
double getVoltage(void);
double getCurrent(void);
void setLEDPower(uint8_t inputValue);
void setLEDIntensity(uint16_t inputValue);
void configureLEDIntensity(Event * event);
void configureLEDPower(Event * event);
void configureLEDCutOffTemp(Event * event);
typedef enum{
	LED_FN_IDLE,
	LED_FN_REPLY_PACKET
} LedFunctionState;


#endif /* __GETVALUE_H */