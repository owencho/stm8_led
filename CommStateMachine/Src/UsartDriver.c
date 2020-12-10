#include "UsartDriver.h"
#include "UsartHardware.h"
#include "EventQueue.h"
#include "Event.h"
#include "List.h"
#include "Irq.h"
#include "TimerEventQueue.h"
#include "CommEventQueue.h"
#include "CmdCompareForAVL.h"
#include "Crc.h"
#include <stdio.h>
#include <string.h>

GenericStateMachine mallocInfo;
GenericStateMachine freeMemInfo;
uint8_t txDataForFlags[2];
volatile UsartDriverInfo usartDriverInfo;

#define hasRequestedTxPacket(info) ((info)->requestTxPacket)
#define hasRequestedRxPacket(info) ((info)->requestRxPacket)
#define isLastTxByte(info) ((info->txLen) < (info->txCounter)+1)
#define isLastRxByte(info) ((info->rxLen) <= (info->rxCounter)-PAYLOAD_OFFSET)
#define getCommandByte(info) (info->rxMallocBuffer[CMD_OFFSET])
#define getSenderAddress(info) (info->rxMallocBuffer[SENDER_ADDRESS_OFFSET])

STATIC int findPacketLength(uint8_t* data){
    int size = *(&data + 1) - data;
    return (sizeof(data)/sizeof(data[0]));
}

STATIC int getPacketLength(uint8_t * txData){
    int packetLength = txData[LENGTH_OFFSET];
    return packetLength;
}

STATIC int isCorrectAddress(UsartDriverInfo *info){
    char * packet = info->rxStaticBuffer;
    char usartAddress = *(packet + RECEIVER_ADDRESS_OFFSET);

    if((int)usartAddress == USART_ADDRESS)
        return 1;
    else
        return 0;
}

STATIC void usartDriverInit(void){
		#define info (usartDriverInfo)
    //UsartDriverInfo * info =&usartDriverInfo;
    info.txUsartEvent = NULL;
    info.txState = TX_IDLE;
    info.txBuffer = NULL;
    info.requestTxPacket = 0;
    info.txCounter = 0;
    info.txLen = 0;
    info.txFlag = 0;

    info.rxUsartEvent = NULL;
    info.rxState = RX_IDLE;
    info.rxMallocBuffer = NULL;
    info.requestRxPacket = 0;
    info.rxCounter = 0;
    info.rxLen = 0;

    mallocInfo.callback = (Callback)allocMemForReceiver;
    freeMemInfo.callback = (Callback)freeMemForReceiver;
}

void usartInit(void){
    usartDriverInit();
    usartHardwareInit();
}

void usartDriverTransmit(UsartPort port,uint8_t rxAddress,int length,uint8_t * txData,UsartEvent * event){
		UsartDriverInfo * info =&usartDriverInfo;
		disableIRQ();

    if(!hasRequestedTxPacket(info)){
        info->txLen =length;
        info->receiverAddress = rxAddress;
        info->txUsartEvent = event;
        info->txBuffer = txData;
        generateCRC16forTxPacket(port);
        info->requestTxPacket = 1;
        hardwareUsartTransmit(port);
    }
    enableIRQ();
}

uint8_t usartTransmissionHandler(UsartPort port){
    UsartDriverInfo * info =&usartDriverInfo;
    uint8_t * txBuffer = info->txBuffer;
    uint8_t * txCRC16 = info->txCRC16;
    UsartEvent * event = info->txUsartEvent;
    uint8_t transmitByte;

    switch(info->txState){
        case TX_IDLE :
            transmitByte = info->receiverAddress;
            info->txState = TX_SEND_RECEIVER_ADDRESS;
            break;
        case TX_SEND_RECEIVER_ADDRESS:
            transmitByte = USART_ADDRESS;
            info->txState = TX_SEND_TRANSMITTER_ADDRESS;
            break;
        case TX_SEND_TRANSMITTER_ADDRESS:
            transmitByte = info->txLen;
            info->txState = TX_SEND_LENGTH;
            break;
        case TX_SEND_LENGTH:
            transmitByte = info->txFlag;
            info->txState = TX_SEND_FLAG;
            break;
        case TX_SEND_FLAG:
            transmitByte = txBuffer[info->txCounter];
            info->txCounter ++;
            info->txState = TX_SEND_BYTE;
            break;
        case TX_SEND_BYTE:
            transmitByte = txBuffer[info->txCounter];
            info->txCounter ++;
            if(isLastTxByte(info)){
                info->txCounter = 0;
                info->txState = TX_SEND_CRC16;
            }
            break;
        case TX_SEND_CRC16:
            transmitByte = txCRC16[info->txCounter];
            info->txCounter ++;
            if(info->txCounter > 1){
                event->type = TX_COMPLETE;
                setHardwareTxLastByte(port);
                //eventEnqueue(&evtQueue,(Event*)event);
                hardwareUsartReceive(port);
								info->txCounter = 0;
                info->requestTxPacket = 0;
                info->txState = TX_IDLE;
            }
            break;
    }
    return transmitByte;
}

void usartReceiveHandler(UsartPort port,uint16_t rxByte){
    uint8_t eventByte = rxByte >> 8;
    uint8_t dataByte = rxByte & 0xFF;
    UsartDriverInfo * info =&usartDriverInfo;
    UsartEvent * evt = info->rxUsartEvent;
    uint8_t * staticBuffer = info->rxStaticBuffer;
    uint8_t * mallocBuffer = info->rxMallocBuffer;
    uint8_t * rxCRC16 = info->rxCRC16;

    switch(info->rxState){
        case RX_IDLE :
            if(eventByte == RX_PACKET_START){
                info->rxCounter = 0;
                info->rxState = RX_ADDRESS_LENGTH;
            }
            break;
        case RX_ADDRESS_LENGTH :
            handleRxAddressAndLength(port,rxByte);
            break;
        case RX_RECEIVE_PAYLOAD_STATIC_BUFFER :
            handleRxStaticBufferPayload(port,rxByte);
            break;
        case RX_RECEIVE_PAYLOAD_MALLOC_BUFFER :
            handleRxMallocBufferPayload(port,rxByte);
            break;
        case RX_WAIT_CRC16_STATIC_BUFFER :
            handleCRC16WithStaticBuffer(port,rxByte);
            break;
        case RX_WAIT_CRC16_MALLOC_BUFFER :
						handleCRC16WithMallocBuffer(port,rxByte);
            break;
        case RX_WAIT_FOR_MALLOC_BUFFER :
            if(eventByte == MALLOC_REQUEST_EVT){
                strcpy(mallocBuffer, staticBuffer);
								generateEventForReceiveComplete(port);
                info->rxState = RX_IDLE;
            }
            break;
        case RX_WAIT_FOR_FREE_MALLOC_BUFFER:
            if(eventByte == FREE_MALLOC_EVT){
								resetUsartDriverReceive(port);
                info->rxState = RX_IDLE;
            }
    }
}

STATIC void handleRxAddressAndLength(UsartPort port,uint16_t rxByte){
    uint8_t eventByte = rxByte >> 8;
    uint8_t dataByte = rxByte & 0xFF;
    UsartDriverInfo * info =&usartDriverInfo;
    UsartEvent * evt = info->rxUsartEvent;
    uint8_t * staticBuffer = info->rxStaticBuffer;

    if(eventByte == RX_PACKET_START){
        resetUsartDriverReceive(port);
        info->rxState = RX_ADDRESS_LENGTH;
        return;
    }
    else if(info->rxCounter < 3){
        staticBuffer[info->rxCounter] = dataByte;
        info->rxCounter++;
    }

    if (info->rxCounter >= 3 ){
        if(isCorrectAddress(info)){
            info->rxLen = staticBuffer[LENGTH_OFFSET];
            info->sysEvent.stateMachineInfo = &mallocInfo;
            info->sysEvent.data = (void*)info;
            eventEnqueue(&sysQueue,(Event*)&info->sysEvent);
            info->rxState = RX_RECEIVE_PAYLOAD_STATIC_BUFFER;
        }
        else{
            info->rxCounter = 0;
            info->rxState = RX_IDLE;
        }
    }
}

STATIC void handleRxStaticBufferPayload(UsartPort port,uint16_t rxByte){
    uint8_t eventByte = rxByte >> 8;
    uint8_t dataByte = rxByte & 0xFF;
    UsartDriverInfo * info =&usartDriverInfo;
    uint8_t * staticBuffer = info->rxStaticBuffer;
    uint8_t * mallocBuffer = info->rxMallocBuffer;
    uint8_t * rxCRC16 = info->rxCRC16;

    if(eventByte == RX_PACKET_START){
        resetUsartDriverReceive(port);
        info->rxState = RX_ADDRESS_LENGTH;
    }
    else if(eventByte == MALLOC_REQUEST_EVT){
        strcpy(mallocBuffer, staticBuffer);
        info->rxState = RX_RECEIVE_PAYLOAD_MALLOC_BUFFER;
    }
    else if (isLastRxByte(info)){
        rxCRC16[0] = dataByte;
        info->rxState = RX_WAIT_CRC16_STATIC_BUFFER;
    }
    else{
        staticBuffer[info->rxCounter] = dataByte;
        info->rxCounter++;
    }
}

STATIC void handleRxMallocBufferPayload(UsartPort port,uint16_t rxByte){
    uint8_t eventByte = rxByte >> 8;
    uint8_t dataByte = rxByte & 0xFF;
    UsartDriverInfo * info =&usartDriverInfo;
    uint8_t * mallocBuffer = info->rxMallocBuffer;
    uint8_t * rxCRC16 = info->rxCRC16;

    if(eventByte == RX_PACKET_START){
		requestForFreeMemoryEvent(port);
        resetUsartDriverReceive(port);
        info->rxState = RX_WAIT_FOR_FREE_MALLOC_BUFFER;
    }
    else if (isLastRxByte(info)){
        rxCRC16[0] = dataByte;
        info->rxState = RX_WAIT_CRC16_MALLOC_BUFFER;
    }
    else{
        mallocBuffer[info->rxCounter] = dataByte;
        info->rxCounter++;
    }
}

STATIC void handleCRC16WithStaticBuffer(UsartPort port,uint16_t rxByte){
    uint8_t eventByte = rxByte >> 8;
    uint8_t dataByte = rxByte & 0xFF;
    UsartDriverInfo * info =&usartDriverInfo;
    uint8_t * mallocBuffer = info->rxMallocBuffer;
    uint8_t * staticBuffer = info->rxStaticBuffer;
    uint8_t * rxCRC16 = info->rxCRC16;

    if(eventByte == RX_PACKET_START){
        resetUsartDriverReceive(port);
        info->rxState = RX_ADDRESS_LENGTH;
    }
    else if(eventByte == MALLOC_REQUEST_EVT){
        strcpy(mallocBuffer, staticBuffer);
        info->rxState = RX_WAIT_CRC16_MALLOC_BUFFER;
    }
    else{
        rxCRC16[1] = dataByte;
        info->rxState = RX_WAIT_FOR_MALLOC_BUFFER;
    }
}

STATIC void handleCRC16WithMallocBuffer(UsartPort port,uint16_t rxByte){
    uint8_t eventByte = rxByte >> 8;
    uint8_t dataByte = rxByte & 0xFF;
    UsartDriverInfo * info =&usartDriverInfo;
    uint8_t * rxCRC16 = info->rxCRC16;

	if(eventByte == RX_PACKET_START){
		requestForFreeMemoryEvent(port);
		resetUsartDriverReceive(port);
		info->rxState = RX_WAIT_FOR_FREE_MALLOC_BUFFER;
	}
	else{
		rxCRC16[1] = dataByte;
		generateEventForReceiveComplete(port);
		info->rxState = RX_IDLE;
	}
}

STATIC int checkRxPacketCRC(UsartPort port){
    UsartDriverInfo * info =&usartDriverInfo;
    int rxLength = info->rxLen;
    uint8_t * rxCRC16ptr = info->rxCRC16;
    uint8_t * rxBuffer = info->rxMallocBuffer;
    uint16_t crcRxValue = *(uint16_t*)&rxCRC16ptr[0];
    uint16_t generatedCrc16;

    generatedCrc16=generateCrc16(&rxBuffer[PAYLOAD_OFFSET], rxLength);

    if(crcRxValue == generatedCrc16){
        return 1;
    }
    return 0;
}

STATIC void generateEventForReceiveComplete(UsartPort port){
    UsartDriverInfo * info =&usartDriverInfo;
    UsartEvent * rxUsartEvent = info->rxUsartEvent;
    uint8_t * rxBuffer = info->rxMallocBuffer;

    if(checkRxPacketCRC(port)){
        rxUsartEvent->type = PACKET_RX_EVENT;
    }
    else{
        rxUsartEvent->type = RX_CRC_ERROR_EVENT;
    }
    findSMInfoAndGenerateEvent(port);
}

STATIC void findSMInfoAndGenerateEvent(UsartPort port){
    UsartDriverInfo * info =&usartDriverInfo;
    UsartEvent * rxUsartEvent = info->rxUsartEvent;
    uint8_t * rxBuffer = info->rxMallocBuffer;
		GenericStateMachine * infoSM;
		char command = rxBuffer[4];
		
		switch(command){
		
		case 0 : //setLED
						 break;
		case 1 : //setIntensity
						 break;
		case 2 : //getVoltage
						 break;
		case 3 : //getTemp
						 break;
		case 4 : //setCutOffTemp
						 break;
		}
		/*
    #ifdef MASTER
        infoSM = (GenericStateMachine *)MASTER_SMINFO;
    #else
        infoSM = getStateMachineInfoFromAVL(port);
    #endif

    if(!infoSM){
        generateAndSendNotAvailablePacket(port);
        return;
    }
		rxUsartEvent->stateMachineInfo= infoSM;
    rxUsartEvent->buffer = rxBuffer;
    eventEnqueue(&evtQueue,(Event*)rxUsartEvent);
		*/
}
/*
STATIC GenericStateMachine * getStateMachineInfoFromAVL(UsartPort port){
    UsartDriverInfo * info =&usartDriverInfo[port];
    UsartEvent * rxUsartEvent = info->rxUsartEvent;
    uint8_t * rxBuffer = info->rxMallocBuffer;
    int cmd = getCommandByte(info);
    CmdNode * cmdNode = (CmdNode * )avlFindNode((Node*)rootCmdNode,(void*)&cmd,
                                                (Compare)cmdCompareForAVL);
	if(!cmdNode){
        return NULL;
    }
	return (GenericStateMachine *)cmdNode->info;
}
*/

STATIC void generateAndSendNotAvailablePacket(UsartPort port){
	UsartDriverInfo * info =&usartDriverInfo;
	uint8_t senderAddress = getSenderAddress(info);
	UsartEvent * rxUsartEvent = info->rxUsartEvent;
	uint8_t * rxBuffer = info->rxMallocBuffer;
	rxUsartEvent->stateMachineInfo = &freeMemInfo;
	rxUsartEvent->data = (void*)info;
 	generateFlagAndTransmit(port,senderAddress,UF_CMD_NOT_AVAILABLE,rxUsartEvent);

}

void generateFlagAndTransmit(UsartPort port,uint8_t rxAddress,UsartDriverFlags flags,UsartEvent * event){
	UsartDriverInfo * info =&usartDriverInfo;
	uint8_t * rxBuffer = info->rxMallocBuffer;
	uint8_t flagByte = 1<<flags;
    txDataForFlags[0]=flagByte;
    txDataForFlags[1]=getCommandByte(info);
	usartDriverTransmit(port,rxAddress,2,txDataForFlags,event);
}

STATIC void resetUsartDriverReceive(UsartPort port){
    UsartDriverInfo * info =&usartDriverInfo;
    info->rxState = RX_IDLE;
    info->rxCounter = 0;
    info->rxLen = 0;
}

STATIC void generateCRC16forTxPacket(UsartPort port){
    UsartDriverInfo * info =&usartDriverInfo;
    uint8_t * txBuffer = info->txBuffer;
    uint8_t * txCRC16 = info->txCRC16;
    int length = info->txLen;
    uint16_t crc16 ;
    crc16 = generateCrc16(txBuffer, length);
	*(uint16_t*)&txCRC16[0] = crc16;
}

STATIC void requestForFreeMemoryEvent(UsartPort port){
	UsartDriverInfo * info =&usartDriverInfo;
	info->sysEvent.stateMachineInfo = &freeMemInfo;
	info->sysEvent.data = (void*)info;
	eventEnqueue(&sysQueue,(Event*)&info->sysEvent);
}

void allocMemForReceiver(Event * event){
		UsartDriverInfo * info =(UsartDriverInfo*)event->data;
		int receiverlength = info->rxLen + 3;
    disableIRQ();
    info->rxMallocBuffer = (uint8_t*) malloc(receiverlength * sizeof(uint8_t));
    info->rxUsartEvent = (UsartEvent*) malloc(sizeof(UsartEvent));
    enableIRQ();
    usartReceiveHandler(info->portName,(MALLOC_REQUEST_EVT << 8));
}

void freeMemForReceiver(Event * event){
		UsartDriverInfo * info =(UsartDriverInfo*)event->data;
    disableIRQ();
    freeMem(info->rxMallocBuffer);
    freeMem(info->rxUsartEvent);
    enableIRQ();
    usartReceiveHandler(info->portName,(FREE_MALLOC_EVT << 8));
}

void removeTimerEventFromQueue(Event * event){
    disableIRQ();
	if(!eventDequeue(&evtQueue,&event)){
		timerEventDequeueSelectedEvent(&timerQueue,(TimerEvent*)event);
	}
    enableIRQ();
}
