#ifndef EVENTTYPE_H_
#define EVENTTYPE_H_
typedef enum{
    NO_EVENT ,RX_COMPLETE ,TX_COMPLETE , ERR_EVENT,
    PACKET_RX_EVENT,RX_CRC_ERROR_EVENT
} EventType;
#endif // EVENTTYPE_H