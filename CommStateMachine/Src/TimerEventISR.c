/*
 * TimerEvent.c
 *
 *  Created on: Jun 25, 2020
 *      Author: academic
 */
#include "List.h"
#include "LinkedListCompare.h"
#include "EventCompare.h"
#include "Irq.h"
#include "TimerEventISR.h"

//extern int isEvent ;
int totalTimeDelay,relativeTick;

void timerEventISR(EventQueue * readyQueue,TimerEventQueue *timerEventQueue){
	TimerEvent * currentTimerEventItem;
	disableIRQ();
		if(timerEventQueueGetCount(timerEventQueue) !=0){
		resetCurrentListItem((List*)timerEventQueue);
		currentTimerEventItem=(TimerEvent*)getCurrentListItem((List*)timerEventQueue);
		if(currentTimerEventItem->time == timerEventQueueGetRelativeTick(timerEventQueue)){
	      	currentTimerEventItem = timerEventDequeue(timerEventQueue);
	  		listAddItemToTail((List*)readyQueue,(ListItem*)(currentTimerEventItem->data));
			resetTick(timerEventQueue);
			checkAndDequeueIfNextEventTimerIsZero(readyQueue,timerEventQueue);
		}
	}
	enableIRQ();
}

void checkAndDequeueIfNextEventTimerIsZero(EventQueue * readyQueue,TimerEventQueue *timerEventQueue){
	TimerEvent * nextTimerEventItem;
	resetCurrentTimerEventQueue(timerEventQueue);
	nextTimerEventItem=(TimerEvent*)getCurrentListItem((List*)timerEventQueue);
	while(nextTimerEventItem != NULL){
		if(nextTimerEventItem->time == 0){
				nextTimerEventItem =timerEventDequeue(timerEventQueue);
				listAddItemToTail((List*)readyQueue,(ListItem*)(nextTimerEventItem->data));
		}
		nextTimerEventItem=timerEventQueueGetCurrentEvent(timerEventQueue);
		if(nextTimerEventItem == NULL)
				break;
		else if (nextTimerEventItem-> time != 0)
				break;
	}
	resetCurrentListItem((List*)timerEventQueue);
}
