
#ifndef _DISPLAY_BASE_H
#define _DISPLAY_BASE_H

#include "main.h"

#define DISPLAY_SIZE_RX_BUFFER 64
#define DISPLAY_SIZE_TX_BUFFER 64

#define RX_BYTE(b) (ui8_buffer_index1 + b) % DISPLAY_SIZE_RX_BUFFER


#if DISPLAY_TYPE == DISPLAY_TYPE_DEBUG
    #define DISPLAY_UART_BAUDRATE 57600
#else
    #define DISPLAY_UART_BAUDRATE 9600
#endif

extern volatile uint8_t ui8_g_UART_Rx_flag;
extern volatile uint8_t ui8_g_UART_TxCplt_flag;


void Display_Init(MotorState_t* pMS);
void Display_Service(MotorState_t* pMS);

void debug_print(uint8_t* data, uint16_t size);
void debug_print2(uint8_t* data, uint16_t size);


/* 

*********************************
How the display code works
*********************************

Incoming messages are continuously written into the ring-buffer 'RxBuff' via DMA.
The UART Idle Line Interrupt is triggered as soon as the rx data line is idle -> UART_IdleItCallback.
The callback determines the size of the received message, sets the read and write pointers an sets ui8_g_UART_Rx_flag to one.
The flag causes the Display_Service code to be executed in the main while loop.



*********************************
How to extened the base display:
*********************************

(1) specify DISPLAY_UART_BAUD_RATE

(2) implement display c file, see template code below


// ---------------------------------------------- display template code

#include "display_base.h"
#include "main.h"
#include "config.h"
#include "stm32f1xx_hal.h"

extern UART_HandleTypeDef huart1;

extern uint8_t ui8_buffer_index1; // circular buffer read position
extern uint8_t ui8_buffer_index2; // circular buffer write position
extern uint8_t ui8_bytes_received;
extern uint8_t RxBuff[DISPLAY_SIZE_RX_BUFFER];
extern uint8_t TxBuff[DISPLAY_SIZE_TX_BUFFER];

void Display_Service(MotorState_t *pMS)
{

    implement service here
    
    
    // set buffer read index to buffer write index
    ui8_buffer_index1 = ui8_buffer_index2;
}

// ---------------------------------------------- display template code


*/

#endif //#define _DISPLAY_BASE_H