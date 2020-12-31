#ifndef USARTHARDWARE_H
#define USARTHARDWARE_H
#include "StateMachine.h"
#include "irq.h"
#include <stdint.h>
#include "SM_Common.h"
//This is user define configuration
#define USART_ADDRESS 0x1
#define MASTER_ADDRESS 0x0
#define RX_PACKET_START 0x11

typedef enum{
    MAIN_CONTROLLER
} UsartPort;

typedef enum{
    HW_TX_IDLE,
    HW_TX_SEND_DELIMITER,
    HW_TX_SEND_BYTE,
    HW_TX_SEND_7E_BYTE
} UsartHardwareTxState;

typedef enum{
    HW_RX_IDLE,
    HW_RX_RECEIVED_DELIMITER,
    HW_RX_RECEIVE_BYTE,
    HW_RX_RECEIVE_7E_BYTE
} UsartHardwareRxState;

typedef void (*UsartCallback)(UsartPort port);
typedef uint8_t (*TxCallback)(UsartPort port);
typedef void (*RxCallback)(UsartPort port,uint16_t rxByte);

typedef struct UsartInfo UsartInfo;
struct UsartInfo {
    TxCallback txCallBack;
    RxCallback rxCallBack;
    UsartCallback errorCallBack;
    UsartHardwareTxState hwTxState;
    UsartHardwareRxState hwRxState;
    int txTurn;
    int lastByte;
    //link usartDriverInfo
};

//#define getUsartNumber() (sizeof(usartInfo)/sizeof(UsartInfo))

STATIC void initUsartHardwareInfo(UsartPort port);
void usartHardwareInit(void);
void hardwareUsartTransmit(UsartPort port);
void hardwareUsartReceive(UsartPort port);
void usartIrqHandler(UsartPort port);
uint8_t usartTransmitHardwareHandler(UsartPort port);
void usartReceiveHardwareHandler(UsartPort port,uint8_t rxByte);
void setHardwareTxLastByte(UsartPort port);
void endOfUsartTxHandler(UsartPort port);


#endif // USARTHARDWARE_H
