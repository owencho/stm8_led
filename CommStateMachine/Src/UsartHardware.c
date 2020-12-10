#include "stm8s.h"
#define __STDINT__
#include "UsartHardware.h"
#include "EventQueue.h"
#include "Event.h"
#include "List.h"
#include "irq.h"
#include "UsartDriver.h"
#include "TimerEventQueue.h"


#define isLastByte(info) ((info)->lastByte)

volatile UsartInfo usartInfo;

STATIC void initUsartHardwareInfo(UsartPort port){
		#define hwInfo (usartInfo)
    hwInfo.rxCallBack = (RxCallback)usartReceiveHandler;
    hwInfo.txCallBack = (TxCallback)usartTransmissionHandler;
    hwInfo.hwTxState = HW_TX_IDLE;
    hwInfo.hwRxState = HW_RX_IDLE;
    hwInfo.txTurn =0;
    hwInfo.lastByte =0;
}

void usartHardwareInit(void){
		disableIRQ();
    //memset(&usartInfo[1],0,sizeof(UsartInfo));
    initUsartHardwareInfo(MAIN_CONTROLLER);
    enableIRQ();
}

void hardwareUsartTransmit(UsartPort port){
		UsartInfo * info = &usartInfo;
    disableIRQ();
    info->txTurn = 1;
		UART1_ITConfig(UART1_IT_TC,ENABLE); 

    //usartEnableTransmission(info->usart);
    //usartEnableInterrupt(info->usart,TRANS_COMPLETE);
    //usartDisableReceiver(info->usart);
    enableIRQ();
}

void hardwareUsartReceive(UsartPort port){
		UsartInfo * info =&usartInfo;
    disableIRQ();
    info->txTurn = 0;
		UART1_ITConfig(UART1_IT_RXNE_OR,ENABLE); 
    //usartEnableReceiver(info->usart);
    //usartEnableInterrupt(info->usart,RXNE_INTERRUPT);
    enableIRQ();
}

void usartIrqHandler(UsartPort port){
    UsartInfo * info = &usartInfo;
    char rxByte;
    char txByte;

    if(info->txTurn){
        txByte = usartTransmitHardwareHandler(port);
				UART1_ClearITPendingBit(UART1_IT_TC);
				UART1_ClearFlag(UART1_IT_TC);
        UART1_SendData8(txByte);
    }
    else{
        rxByte = UART1_ReceiveData8();
				UART1_ClearITPendingBit(UART1_IT_RXNE);
        usartReceiveHardwareHandler(port,rxByte);
    }
}

uint8_t usartTransmitHardwareHandler(UsartPort port){
    UsartInfo * info = &usartInfo;
    uint8_t transmitByte;
    switch(info->hwTxState){
        case HW_TX_IDLE :
            info->hwTxState = HW_TX_SEND_DELIMITER;
            transmitByte = 0x7E;
            break;
        case HW_TX_SEND_DELIMITER :
            info->hwTxState = HW_TX_SEND_BYTE;
            transmitByte = 0x81;
            break;
        case HW_TX_SEND_BYTE :
            transmitByte = info->txCallBack(port);
            //instead of passing port better info->usartDriverInfo
            if(transmitByte == 0x7E){
                info->hwTxState = HW_TX_SEND_7E_BYTE;
            }
            else if(isLastByte(info)){
                endOfUsartTxHandler(port);
                info->hwTxState = HW_TX_IDLE;
                info->lastByte = 0;
            }
            break;
        case HW_TX_SEND_7E_BYTE :
            if(isLastByte(info)){
                endOfUsartTxHandler(port);
                info->hwTxState = HW_TX_IDLE;
                info->lastByte = 0;
            }
            else{
                info->hwTxState = HW_TX_SEND_BYTE;
            }
            transmitByte = 0xE7;
            break;
    }
    return transmitByte;
}

void usartReceiveHardwareHandler(UsartPort port,uint8_t rxByte){
    UsartInfo * info = &usartInfo;
    switch(info->hwRxState){
        case HW_RX_IDLE :
            if(rxByte == 0x7E){
                info->hwRxState = HW_RX_RECEIVED_DELIMITER;
            }
            break;
        case HW_RX_RECEIVED_DELIMITER :
            if(rxByte == 0x81){
                info->rxCallBack(port,RX_PACKET_START<<8);
                info->hwRxState = HW_RX_RECEIVE_BYTE;
            }
            else{
                info->hwRxState = HW_RX_IDLE;
            }
            break;
        case HW_RX_RECEIVE_BYTE :
            if(rxByte == 0x7E){
                info->hwRxState = HW_RX_RECEIVE_7E_BYTE;
            }
            else{
                info->rxCallBack(port,rxByte);
            }
            break;
        case HW_RX_RECEIVE_7E_BYTE :
            if(rxByte == 0x81){
                info->rxCallBack(port,(RX_PACKET_START<<8));
                info->hwRxState = HW_RX_RECEIVE_BYTE;
            }
            else if (rxByte == 0xE7){
                info->rxCallBack(port,0x7E);
                info->hwRxState = HW_RX_RECEIVE_BYTE;
            }
            else{
                info->hwRxState = HW_RX_IDLE;
            }
            break;
    }
}

void setHardwareTxLastByte(UsartPort port){
    UsartInfo * info = &usartInfo;
    info->lastByte = 1;
}

void endOfUsartTxHandler(UsartPort port){
    UsartInfo * info = &usartInfo;
		UART1_ITConfig(UART1_IT_TC,DISABLE);
    //usartDisableInterrupt(usart,TRANS_COMPLETE);
    //usartDisableTransmission(usart);
    //usartEnableReceiver(usart);
    info->txTurn = 0;
}