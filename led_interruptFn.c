#include "UsartHardware.h"
#include "led_interruptFn.h"
#include "irq.h"
@svlreg @interrupt void usartInterrupt(void){
	disableIRQ();
	usartIrqHandler(MAIN_CONTROLLER);
	enableIRQ();
}
