#include "stm8s.h"
#define __STDINT__
#include "UsartHardware.h"
#include "EventQueue.h"
#include "Event.h"
#include "List.h"
#include "irq.h"
#include "UsartDriver.h"
#include "TimerEventQueue.h"


#define isLastByte(info) (info.lastByte)

volatile UsartInfo usartHardwareInfo;
STATIC void initUsartHardwareInfo(UsartPort port){
    usartHardwareInfo.rxCallBack = (RxCallback)usartReceiveHandler;
    usartHardwareInfo.txCallBack = (TxCallback)usartTransmissionHandler;
    usartHardwareInfo.hwTxState = HW_TX_IDLE;
    usartHardwareInfo.hwRxState = HW_RX_IDLE;
    usartHardwareInfo.txTurn =0;
    usartHardwareInfo.lastByte =0;
}

void usartHardwareInit(void){
		disableIRQ();
    initUsartHardwareInfo(MAIN_CONTROLLER);
    enableIRQ();
}

void hardwareUsartTransmit(UsartPort port){
    disableIRQ();
    usartHardwareInfo.txTurn = 1;
		GPIO_WriteHigh(GPIOC,GPIO_PIN_7);
		UART1_ClearITPendingBit(UART1_IT_RXNE);
		UART1_ClearFlag(UART1_IT_RXNE);
		UART1_ClearITPendingBit(UART1_IT_TC);
		UART1_ClearFlag(UART1_IT_TC);		
		UART1_ITConfig(UART1_IT_RXNE_OR,DISABLE);
		UART1_ITConfig(UART1_IT_TC,ENABLE);
    enableIRQ();
}

void hardwareUsartReceive(UsartPort port){
    disableIRQ();
    usartHardwareInfo.txTurn = 0;
		GPIO_WriteLow(GPIOC,GPIO_PIN_7);
		UART1_ClearITPendingBit(UART1_IT_RXNE);
		UART1_ClearFlag(UART1_IT_RXNE);
		UART1_ClearITPendingBit(UART1_IT_TC);
		UART1_ClearFlag(UART1_IT_TC);		
		UART1_ITConfig(UART1_IT_TC,DISABLE);
		UART1_ITConfig(UART1_IT_RXNE_OR,ENABLE);
    enableIRQ();
}

void usartIrqHandler(UsartPort port){
    char rxByte;
    uint8_t txByte;
		disableIRQ();
		
		if(usartHardwareInfo.lastByte){
				usartHardwareInfo.lastByte = 0;
				hardwareUsartReceive(MAIN_CONTROLLER);
				//UART1_SendData8(123);

		}
    else if(usartHardwareInfo.txTurn){
        txByte = usartTransmitHardwareHandler(port);
        //UART1_SendData8(txByte);
    }
    else{
        rxByte = UART1_ReceiveData8();
        usartReceiveHardwareHandler(port,rxByte);
    }
		UART1_ClearITPendingBit(UART1_IT_RXNE);
		UART1_ClearFlag(UART1_IT_RXNE);
		UART1_ClearITPendingBit(UART1_IT_TC);
		UART1_ClearFlag(UART1_IT_TC);
		enableIRQ();
}

uint8_t usartTransmitHardwareHandler(UsartPort port){
    uint8_t transmitByte;
		
    switch(usartHardwareInfo.hwTxState){
        case HW_TX_IDLE :
            usartHardwareInfo.hwTxState = HW_TX_SEND_DELIMITER;
            transmitByte = 0x7E;
						UART1_SendData8(0x7E);
            break;
        case HW_TX_SEND_DELIMITER :
            usartHardwareInfo.hwTxState = HW_TX_SEND_BYTE;
            transmitByte = 0x81;
						UART1_SendData8(0x81);
            break;
						
        case HW_TX_SEND_BYTE :
            transmitByte = usartHardwareInfo.txCallBack(port);
						UART1_SendData8(transmitByte);
            if(transmitByte == 0x7E){
                usartHardwareInfo.hwTxState = HW_TX_SEND_7E_BYTE;
            }
            else if(usartHardwareInfo.lastByte){
                usartHardwareInfo.hwTxState = HW_TX_IDLE;
            }
            break;
        case HW_TX_SEND_7E_BYTE :
            if(usartHardwareInfo.lastByte){
                usartHardwareInfo.hwTxState = HW_TX_IDLE;

            }
            else{
                usartHardwareInfo.hwTxState = HW_TX_SEND_BYTE;
            }
						UART1_SendData8(0xE7);
            break;
						
    }
		//UART1_SendData8(transmitByte);
    return transmitByte;
}

void usartReceiveHardwareHandler(UsartPort port,uint8_t rxByte){
    switch(usartHardwareInfo.hwRxState){
        case HW_RX_IDLE :
            if(rxByte == 0x7E){
                usartHardwareInfo.hwRxState = HW_RX_RECEIVED_DELIMITER;
            }
            break;
        case HW_RX_RECEIVED_DELIMITER :
            if(rxByte == 0x81){
                usartHardwareInfo.rxCallBack(port,RX_PACKET_START<<8);
                usartHardwareInfo.hwRxState = HW_RX_RECEIVE_BYTE;
            }
            else{
                usartHardwareInfo.hwRxState = HW_RX_IDLE;
            }
            break;
        case HW_RX_RECEIVE_BYTE :
            if(rxByte == 0x7E){
                usartHardwareInfo.hwRxState = HW_RX_RECEIVE_7E_BYTE;
            }
            else{
                usartHardwareInfo.rxCallBack(port,rxByte);
            }
            break;
        case HW_RX_RECEIVE_7E_BYTE :
            if(rxByte == 0x81){
                usartHardwareInfo.rxCallBack(port,(RX_PACKET_START<<8));
                usartHardwareInfo.hwRxState = HW_RX_RECEIVE_BYTE;
            }
            else if (rxByte == 0xE7){
                usartHardwareInfo.rxCallBack(port,0x7E);
                usartHardwareInfo.hwRxState = HW_RX_RECEIVE_BYTE;
            }
            else{
                usartHardwareInfo.hwRxState = HW_RX_IDLE;
            }
            break;
    }
}

void setHardwareTxLastByte(UsartPort port){
    usartHardwareInfo.lastByte = 1;
}

void endOfUsartTxHandler(UsartPort port){
		UART1_ITConfig(UART1_IT_TC,DISABLE);
		hardwareUsartReceive(MAIN_CONTROLLER);
		GPIO_WriteLow(GPIOC,GPIO_PIN_7);
    usartHardwareInfo.txTurn = 0;
}