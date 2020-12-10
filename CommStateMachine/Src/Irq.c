/*
 * Irq.c
 *
 *  Created on: Jun 27, 2020
 *      Author: owen
 */

#include "stm8s.h"
#include "Irq.h"

int disableIrqCount = 0 ;
void disableIRQ(void){
    if(disableIrqCount == 0)
        disableInterrupts();
    disableIrqCount++;
}
void enableIRQ(void){
  disableIrqCount--;
  if(disableIrqCount == 0)
      enableInterrupts();
}
