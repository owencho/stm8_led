#ifndef USARTDRIVER_H
#define USARTDRIVER_H
#include "Event.h"
#include "UsartHardware.h"
#include "CmdNode.h"
#include "TimerEvent.h"
#include "SM_Common.h"

//PACKET LOCATION
#define PACKET_HEADER_SIZE 2
#define RECEIVER_ADDRESS_OFFSET 0
#define SENDER_ADDRESS_OFFSET 1
#define LENGTH_OFFSET 2
#define PAYLOAD_OFFSET 3
#define CMD_OFFSET 4
#define DATA_OFFSET 5
//STATIC BUFFER SIZE
#define STATIC_BUFFER_SIZE 64

//MALLOC EVENT
#define MALLOC_REQUEST_EVT 0x20
#define FREE_MALLOC_EVT 0x30

typedef enum{
	UF_CMD_NOT_AVAILABLE,
	UF_CMD_OK
} UsartDriverFlags;

typedef enum{
    RX_IDLE,
    RX_ADDRESS_LENGTH,
    RX_RECEIVE_PAYLOAD_STATIC_BUFFER,
    RX_RECEIVE_PAYLOAD_MALLOC_BUFFER,
    RX_WAIT_CRC16_STATIC_BUFFER,
    RX_WAIT_CRC16_MALLOC_BUFFER,
    RX_WAIT_FOR_MALLOC_BUFFER,
    RX_WAIT_FOR_FREE_MALLOC_BUFFER
} RxHandlerState;

typedef enum{
    TX_IDLE,
    TX_SEND_RECEIVER_ADDRESS,
    TX_SEND_TRANSMITTER_ADDRESS,
    TX_SEND_LENGTH,
    TX_SEND_FLAG,
    TX_SEND_BYTE,
    TX_SEND_CRC16
} TxHandlerState;

typedef struct UsartDriverInfo UsartDriverInfo;
struct UsartDriverInfo {
    //transmit
    TxCallback txCallBack;
    UsartEvent * txUsartEvent;
    TxHandlerState txState;
    int requestTxPacket;
    int txCounter;
    int txLen;
    int txFlag;
    uint8_t  receiverAddress;
    uint8_t * txBuffer;
    uint8_t txCRC16 [2];
    //receive
    RxCallback rxCallBack;
    UsartEvent rxUsartEvent;
		UsartEvent abortUsartEvent;
    RxHandlerState rxState;
		int isEventOccupied;
    int rxCounter;
    int rxLen;
    uint8_t * rxMallocBuffer;
    uint8_t rxStaticBuffer[STATIC_BUFFER_SIZE];
    uint8_t rxCRC16 [2];
    //Common
    SystemEvent sysEvent;
    UsartPort portName;
};
STATIC int findPacketLength(uint8_t* data);
STATIC int isCorrectAddress(UsartDriverInfo info);
STATIC void usartDriverInit(void);

void usartInit(void);
/*
void usartConfig(UsartPort port,int baudRate,OversampMode overSampMode,ParityMode parityMode,
               WordLength length,StopBit sBitMode,EnableDisable halfDuplex);
*/
void usartDriverTransmit(UsartPort port,uint8_t rxAddress,int length,uint8_t * txData,UsartEvent * event);
uint8_t usartTransmissionHandler(UsartPort port);
void usartReceiveHandler(UsartPort port,uint16_t rxByte);
//internal function for usartTransmit
STATIC void generateCRC16forTxPacket(UsartPort port);
//subfunction of receiverHandler
STATIC void handleRxAddressAndLength(UsartPort port,uint16_t rxByte);
STATIC void handleRxStaticBufferPayload(UsartPort port,uint16_t rxByte);
STATIC void handleRxMallocBufferPayload(UsartPort port,uint16_t rxByte);
STATIC void handleCRC16WithStaticBuffer(UsartPort port,uint16_t rxByte);
STATIC void handleCRC16WithMallocBuffer(UsartPort port,uint16_t rxByte);
STATIC int checkRxPacketCRC(UsartPort port);
STATIC void resetUsartDriverReceive(UsartPort port);
STATIC void generateEventForReceiveComplete(UsartPort port);
//STATIC void generateAndSendNotAvailablePacket(UsartPort port);
STATIC GenericStateMachine * getStateMachineInfoFromAVL(UsartPort port);
void generateFlagAndTransmit(UsartPort port,uint8_t rxAddress,UsartDriverFlags flags,UsartEvent * event);
STATIC void requestForFreeMemoryEvent(UsartPort port);
STATIC void findSMInfoAndGenerateEvent(UsartPort port);
//malloc function
void allocMemForReceiver(Event * event);
void freeMemForReceiver(Event * event);
//removeUSartEvent
void removeTimerEventFromQueue(Event * event);
void removeAbortEventFromQueue(UsartEvent * evt);
void setNoMoreUsartEvent(void);
#endif // USARTDRIVER_H
