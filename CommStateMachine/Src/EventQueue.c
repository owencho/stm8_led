#include "Event.h"
#include "EventQueue.h"
#include "EventCompare.h"
#include "List.h"
#include "ListItem.h"
#include "Irq.h"

void eventEnqueue(EventQueue * queue,Event * event){
	  disableIRQ();
		if(queue == NULL || event == NULL){
				enableIRQ();
				return;
		}
    listAddItemToTail((List*)queue,(ListItem *)event);
    enableIRQ();
}

int eventDequeue(EventQueue * queue,Event ** event){
	  disableIRQ();
    if(queue == NULL || event == NULL || queue->count ==0){
    	  enableIRQ();
      	return 0;
    }
    resetCurrentListItem((List*)queue);
    *event = deleteHeadListItem((List*)queue);
    enableIRQ();
    return 1;
}

void * eventQueueDeleteEvent(EventQueue * eventQueue,Event * deleteEvent){
	Event * deletedEvent;
	int relativeTick;
	disableIRQ();
	if(eventQueue==NULL ||deleteEvent == NULL){
		enableIRQ();
		return NULL;
	}
	deletedEvent = (Event*)findListItem((List*)eventQueue,(void*)deleteEvent
											,(LinkedListCompare)eventCompareSameEvent);
	enableIRQ();
	return deletedEvent;
}

