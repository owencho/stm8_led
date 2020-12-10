#ifndef __GETVALUE_H
#define __GETVALUE_H
double getTemperature(void);
double getVoltage(void);
double getCurrent(void);
void setLEDIntensity(uint16_t inputValue);
void configureLEDIntensity(char * data);
#endif /* __GETVALUE_H */