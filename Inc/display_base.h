
#ifndef _DISPLAY_BASE_H
#define _DISPLAY_BASE_H

#include "main.h"

#define DISPLAY_SIZE_RX_BUFFER 64
#define DISPLAY_SIZE_TX_BUFFER 64

#define RX_BYTE(b) (ui8_buffer_index1 + b) % DISPLAY_SIZE_RX_BUFFER


extern volatile uint8_t ui8_g_UART_Rx_flag;
extern uint32_t ui32_g_DisplayBaudRate;


void Display_Init(MotorState_t* pMS);
void Display_Service(MotorState_t* pMS);

void debug_print(uint8_t* data, uint16_t size);


/* 

*********************************
How to extened the base display:
*********************************

#include "display_base.h"
#include "main.h"
#include "stm32f1xx_hal.h"

uint32_t ui32_g_DisplayBaudRate = 9600;  <- define the baud rate here

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


*/

#endif //#define _DISPLAY_BASE_H