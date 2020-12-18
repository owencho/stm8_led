#include "UsartDriver.h"
#include "UsartHardware.h"
#include "EventQueue.h"
#include "Event.h"
#include "List.h"
#include "Irq.h"
#include "TimerEventQueue.h"
#include "CommEventQueue.h"
#include "CmdCompareForAVL.h"
#include "led_function.h"
#include "Crc.h"
#include <stdio.h>
#include <string.h>

GenericStateMachine abortInfo;
GenericStateMachine mallocInfo;
GenericStateMachine freeMemInfo;

GenericStateMachine setLEDInfo;
GenericStateMachine setIntensityInfo;
GenericStateMachine getTempInfo;
GenericStateMachine getVoltageInfo;
GenericStateMachine setCutOffTempInfo;

uint8_t txDataForFlags[2];
volatile UsartDriverInfo usartDriverInfo;

#define hasRequestedTxPacket(info) ((info).requestTxPacket)
#define hasRequestedRxPacket(info) ((info).requestRxPacket)
#define isLastTxByte(info) ((info.txLen) < (info.txCounter)+2)
#define isLastRxByte(info) ((info.rxLen) <= (info.rxCounter)-PAYLOAD_OFFSET)
#define getCommandByte(info) (info.rxMallocBuffer[CMD_OFFSET])
#define getSenderAddress(info) (info.rxMallocBuffer[SENDER_ADDRESS_OFFSET])

STATIC int findPacketLength(uint8_t* data){
    int size = *(&data + 1) - data;
    return (sizeof(data)/sizeof(data[0]));
}

STATIC int getPacketLength(uint8_t * txData){
    int packetLength = txData[LENGTH_OFFSET];
    return packetLength;
}

STATIC int isCorrectAddress(UsartDriverInfo info){
    char * packet = info.rxStaticBuffer;
    char usartAddress = *(packet + RECEIVER_ADDRESS_OFFSET);

    if((int)usartAddress == USART_ADDRESS)
        return 1;
    else
        return 0;
}

STATIC void usartDriverInit(void){
    //UsartDriverInfo * info =&usartDriverInfo;
    usartDriverInfo.txUsartEvent = NULL;
    usartDriverInfo.txState = TX_IDLE;
    usartDriverInfo.txBuffer = NULL;
    usartDriverInfo.requestTxPacket = 0;
    usartDriverInfo.txCounter = 0;
    usartDriverInfo.txLen = 0;
    usartDriverInfo.txFlag = 0;

    //usartDriverInfo.abortInfo = NULL;
    usartDriverInfo.rxState = RX_IDLE;
    usartDriverInfo.rxMallocBuffer = NULL;
    usartDriverInfo.rxCounter = 0;
    usartDriverInfo.rxLen = 0;
		
		abortInfo.callback = (Callback)removeAbortEventFromQueue;
    mallocInfo.callback = (Callback)allocMemForReceiver;
    freeMemInfo.callback = (Callback)freeMemForReceiver;
		
		setIntensityInfo.callback = (Callback)configureLEDIntensity;
}

void usartInit(void){
    usartDriverInit();
    usartHardwareInit();
		hardwareUsartReceive(MAIN_CONTROLLER);
}

void usartDriverTransmit(UsartPort port,uint8_t rxAddress,int length,uint8_t * txData,UsartEvent * event){
		disableIRQ();

    if(!hasRequestedTxPacket(usartDriverInfo)){
				usartDriverInfo.txCounter = 0;
        usartDriverInfo.txLen =length+1;
        usartDriverInfo.receiverAddress = rxAddress;
        usartDriverInfo.txUsartEvent = event;
        usartDriverInfo.txBuffer = txData;
        generateCRC16forTxPacket(port);
        usartDriverInfo.requestTxPacket = 1;
        hardwareUsartTransmit(port);
    }
    enableIRQ();
}

uint8_t usartTransmissionHandler(UsartPort port){
    uint8_t * txBuffer = usartDriverInfo.txBuffer;
    uint8_t * txCRC16 = usartDriverInfo.txCRC16;
    UsartEvent * event = usartDriverInfo.txUsartEvent;
    uint8_t transmitByte;
		disableIRQ();
    switch(usartDriverInfo.txState){
        case TX_IDLE :
            transmitByte = usartDriverInfo.receiverAddress;
            usartDriverInfo.txState = TX_SEND_RECEIVER_ADDRESS;
            break;
        case TX_SEND_RECEIVER_ADDRESS:
            transmitByte = USART_ADDRESS;
            usartDriverInfo.txState = TX_SEND_TRANSMITTER_ADDRESS;
            break;
        case TX_SEND_TRANSMITTER_ADDRESS:
            transmitByte = usartDriverInfo.txLen;
            usartDriverInfo.txState = TX_SEND_LENGTH;
            break;
        case TX_SEND_LENGTH:
            transmitByte = usartDriverInfo.txFlag;
            usartDriverInfo.txState = TX_SEND_FLAG;
            break;
        case TX_SEND_FLAG:
            transmitByte = txBuffer[usartDriverInfo.txCounter];
            usartDriverInfo.txCounter ++;
            usartDriverInfo.txState = TX_SEND_BYTE;
            break;
        case TX_SEND_BYTE:
            transmitByte = txBuffer[usartDriverInfo.txCounter];
            usartDriverInfo.txCounter ++;
            if(isLastTxByte(usartDriverInfo)){
                usartDriverInfo.txCounter = 0;
                usartDriverInfo.txState = TX_SEND_CRC16;
            }
            break;
        case TX_SEND_CRC16:
            transmitByte = txCRC16[usartDriverInfo.txCounter];
            usartDriverInfo.txCounter ++;
            if(usartDriverInfo.txCounter > 1){
								//setHardwareTxLastByte(port);
								usartDriverInfo.txCounter = 0;
                usartDriverInfo.requestTxPacket = 0;
                usartDriverInfo.txState = TX_IDLE;
                event->type = TX_COMPLETE;
                eventEnqueue(&evtQueue,(Event*)event);
								setHardwareTxLastByte(port);
            }
            break;
    }
		enableIRQ();
    return transmitByte;
}

void usartReceiveHandler(UsartPort port,uint16_t rxByte){
    uint8_t eventByte = rxByte >> 8;
    uint8_t dataByte = rxByte & 0xFF;
    uint8_t * staticBuffer = usartDriverInfo.rxStaticBuffer;
    uint8_t * rxCRC16 = usartDriverInfo.rxCRC16;
		disableIRQ();
    switch(usartDriverInfo.rxState){
        case RX_IDLE :
            if(eventByte == RX_PACKET_START){
							usartDriverInfo.rxState = RX_ADDRESS_LENGTH;
            }
            break;
        case RX_ADDRESS_LENGTH :
            handleRxAddressAndLength(port,rxByte);
            break;
        case RX_RECEIVE_PAYLOAD_STATIC_BUFFER :
            handleRxStaticBufferPayload(port,rxByte);
            break;
        case RX_WAIT_CRC16_STATIC_BUFFER :
            handleCRC16WithStaticBuffer(port,rxByte);
            break;
		enableIRQ();
    }
}

STATIC void handleRxAddressAndLength(UsartPort port,uint16_t rxByte){
    uint8_t eventByte = rxByte >> 8;
    uint8_t dataByte = rxByte & 0xFF;
    uint8_t * staticBuffer = usartDriverInfo.rxStaticBuffer;

    if(eventByte == RX_PACKET_START){
        resetUsartDriverReceive(port);
        return;
    }
    else if(usartDriverInfo.rxCounter < 3){
        staticBuffer[usartDriverInfo.rxCounter] = dataByte;
        usartDriverInfo.rxCounter++;
    }

    if (usartDriverInfo.rxCounter >= 3 ){
        if(isCorrectAddress(usartDriverInfo)){
            usartDriverInfo.rxLen = staticBuffer[LENGTH_OFFSET];
            usartDriverInfo.rxState = RX_RECEIVE_PAYLOAD_STATIC_BUFFER;
        }
        else{
            usartDriverInfo.rxCounter = 0;
            usartDriverInfo.rxState = RX_IDLE;
        }
    }
}

STATIC void handleRxStaticBufferPayload(UsartPort port,uint16_t rxByte){
    uint8_t eventByte = rxByte >> 8;
    uint8_t dataByte = rxByte & 0xFF;
    uint8_t * staticBuffer = usartDriverInfo.rxStaticBuffer;
    uint8_t * rxCRC16 = usartDriverInfo.rxCRC16;

    if(eventByte == RX_PACKET_START){
        resetUsartDriverReceive(port);
        return;
    }
    else if (isLastRxByte(usartDriverInfo)){
        rxCRC16[0] = dataByte;
        usartDriverInfo.rxState = RX_WAIT_CRC16_STATIC_BUFFER;
    }
    else{
        staticBuffer[usartDriverInfo.rxCounter] = dataByte;
        usartDriverInfo.rxCounter++;
    }
}
STATIC void handleCRC16WithStaticBuffer(UsartPort port,uint16_t rxByte){
    uint8_t eventByte = rxByte >> 8;
    uint8_t dataByte = rxByte & 0xFF;
    UsartDriverInfo * info =&usartDriverInfo;
    uint8_t * mallocBuffer = usartDriverInfo.rxMallocBuffer;
    uint8_t * staticBuffer = usartDriverInfo.rxStaticBuffer;
    uint8_t * rxCRC16 = usartDriverInfo.rxCRC16;

    if(eventByte == RX_PACKET_START){
        resetUsartDriverReceive(port);
        return;
    }
    else{
        rxCRC16[1] = dataByte;
        usartDriverInfo.rxState = RX_IDLE;
				generateEventForReceiveComplete(port);
    }
}

STATIC int checkRxPacketCRC(UsartPort port){
    int rxLength = usartDriverInfo.rxLen;
    uint8_t * rxCRC16ptr = usartDriverInfo.rxCRC16;
    uint8_t * rxBuffer = usartDriverInfo.rxStaticBuffer;
    uint16_t crcRxValue = *(uint16_t*)&rxCRC16ptr[1] + rxCRC16ptr[0];
    uint16_t generatedCrc16;

    generatedCrc16=generateCrc16(&rxBuffer[PAYLOAD_OFFSET+1], rxLength-1);

    if(crcRxValue == generatedCrc16){
        return 1;
    }
    return 0;
}

STATIC void generateEventForReceiveComplete(UsartPort port){
    uint8_t * rxBuffer = usartDriverInfo.rxMallocBuffer;

    if(checkRxPacketCRC(port)){
        usartDriverInfo.rxUsartEvent.type = PACKET_RX_EVENT;
    }
    else{
        usartDriverInfo.rxUsartEvent.type = RX_CRC_ERROR_EVENT;
    }
		findSMInfoAndGenerateEvent(port);
    
}


STATIC void findSMInfoAndGenerateEvent(UsartPort port){
    uint8_t * rxBuffer = usartDriverInfo.rxStaticBuffer;
		GenericStateMachine *infoSM;
		char command = rxBuffer[4];
		usartDriverInfo.isEventOccupied = 1;
		switch(command){
		
		case 0 : //setLED
						 break;
		case 1 : infoSM = &setIntensityInfo;
						 break;
		case 2 : //getVoltage
						 break;
		case 3 : //getTemp
						 break;
		case 4 : //setCutOffTemp
						 break;
		}
		usartDriverInfo.rxUsartEvent.stateMachineInfo = infoSM;
    usartDriverInfo.rxUsartEvent.buffer = rxBuffer;
    eventEnqueue(&evtQueue,(Event*)&usartDriverInfo.rxUsartEvent);
}

void generateFlagAndTransmit(UsartPort port,uint8_t rxAddress,UsartDriverFlags flags,UsartEvent * event){
	uint8_t * rxBuffer = usartDriverInfo.rxMallocBuffer;
	uint8_t flagByte = 1<<flags;
	txDataForFlags[0]=flagByte;
	txDataForFlags[1]=getCommandByte(usartDriverInfo);
	usartDriverTransmit(port,rxAddress,2,txDataForFlags,event);
}

STATIC void resetUsartDriverReceive(UsartPort port){
		if(usartDriverInfo.isEventOccupied){
			usartDriverInfo.abortUsartEvent.stateMachineInfo =&abortInfo;
			usartDriverInfo.abortUsartEvent.data = &usartDriverInfo.rxUsartEvent;
			eventEnqueue(&sysQueue,(Event*)&usartDriverInfo.abortUsartEvent);
		}
		usartDriverInfo.rxState = RX_ADDRESS_LENGTH;
    usartDriverInfo.rxCounter = 0;
    usartDriverInfo.rxLen = 0;
}

STATIC void generateCRC16forTxPacket(UsartPort port){
    uint8_t * txBuffer = usartDriverInfo.txBuffer;
    uint8_t * txCRC16 = usartDriverInfo.txCRC16;
    int length = usartDriverInfo.txLen;
    uint16_t crc16 ;
    crc16 = generateCrc16(txBuffer, length-1);
	*(uint16_t*)&txCRC16[0] = crc16;
}

STATIC void requestForFreeMemoryEvent(UsartPort port){
	usartDriverInfo.sysEvent.stateMachineInfo = &freeMemInfo;
	usartDriverInfo.sysEvent.data = (void*)&usartDriverInfo;
	eventEnqueue(&sysQueue,(Event*)&usartDriverInfo.sysEvent);
}

void allocMemForReceiver(Event * event){
		UsartDriverInfo * info =(UsartDriverInfo*)event->data;
		int receiverlength = usartDriverInfo.rxLen + 3;
    disableIRQ();
   // usartDriverInfo.rxMallocBuffer = (uint8_t*) malloc(receiverlength * sizeof(uint8_t));
    //usartDriverInfo.rxUsartEvent = (UsartEvent*) malloc(sizeof(UsartEvent));
    enableIRQ();
    usartReceiveHandler(usartDriverInfo.portName,(MALLOC_REQUEST_EVT << 8));
}

void freeMemForReceiver(Event * event){
		UsartDriverInfo * info =(UsartDriverInfo*)event->data;
    disableIRQ();
    //freeMem(usartDriverInfo.rxMallocBuffer);
    //freeMem(usartDriverInfo.rxUsartEvent);
    enableIRQ();
    usartReceiveHandler(usartDriverInfo.portName,(FREE_MALLOC_EVT << 8));
}

void removeTimerEventFromQueue(Event * event){
    disableIRQ();
	if(!eventDequeue(&evtQueue,&event)){
		timerEventDequeueSelectedEvent(&timerQueue,(TimerEvent*)event);
	}
    enableIRQ();
}

void removeAbortEventFromQueue(UsartEvent * evt){
	Event * abortEvent = (Event*)evt->data;
	disableIRQ();
	if(!eventQueueDeleteEvent(&evtQueue,abortEvent)){
		timerEventDequeueSelectedEvent(&timerQueue,(TimerEvent*)abortEvent);
	}
	enableIRQ();
}

void setNoMoreUsartEvent(){
	disableIRQ();
	usartDriverInfo.isEventOccupied = 0;
	enableIRQ();
}