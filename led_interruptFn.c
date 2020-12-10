#include "UsartHardware.h"
#include "led_interruptFn.h"
@svlreg @interrupt void usartInterrupt(void){
	usartIrqHandler(MAIN_CONTROLLER);
}
