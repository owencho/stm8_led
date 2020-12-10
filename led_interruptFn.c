#include "UsartHardware.h"
#include "led_interruptFn.h"
@interrupt void usartInterrupt(void){
	usartIrqHandler(MAIN_CONTROLLER);
}
