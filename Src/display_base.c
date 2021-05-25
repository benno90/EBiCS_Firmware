
#include "stm32f1xx_hal.h"
#include "display_base.h"

extern UART_HandleTypeDef huart1;

volatile uint8_t ui8_g_UART_Rx_flag = 0;

static uint32_t prevCNDTR = DISPLAY_SIZE_RX_BUFFER;
uint8_t ui8_buffer_index1 = 0; // circular buffer read position
uint8_t ui8_buffer_index2 = 0; // circular buffer write position
uint8_t ui8_bytes_received = 0;
uint8_t RxBuff[DISPLAY_SIZE_RX_BUFFER];
uint8_t TxBuff[DISPLAY_SIZE_TX_BUFFER];

void UART_IdleItCallback(void)
{
    // CNDTR : number of data to be transferred
    // after each peripheral event, this value is decremented
    if (prevCNDTR < DMA1_Channel5->CNDTR)
    {
        // example: prevCNDTR = 3, CNDTR = 60, DISPLAY_SIZE_RX_BUFFER = 64
        // -> bytes_received = 64 - 60 + 3 = 7
        ui8_bytes_received = DISPLAY_SIZE_RX_BUFFER - DMA1_Channel5->CNDTR + prevCNDTR;
    }
    else
    {
        // example: prevCNDTR = 64, CNDTR = 61 -> bytes_received = 3
        ui8_bytes_received = prevCNDTR - DMA1_Channel5->CNDTR;
    }
    prevCNDTR = DMA1_Channel5->CNDTR;
    ui8_buffer_index2 = (ui8_buffer_index2 + ui8_bytes_received) % DISPLAY_SIZE_RX_BUFFER;

    ui8_g_UART_Rx_flag = 1;
}

void Display_Init(MotorState_t* pMS)
{
    // enable idle line interrupt
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);

    // start dma reception
    if (HAL_UART_Receive_DMA(&huart1, RxBuff, DISPLAY_SIZE_RX_BUFFER) != HAL_OK)
    {
        Error_Handler();
    }
}

/*void Display_Service(ApplicationState_t* AS)
{
    if(ui8_bytes_received >= DISPLAY_SIZE_RX_BUFFER)
    {
        return;
    }

    // echo received message

    for (uint8_t i = 0; i < ui8_bytes_received; ++i)
    {
        TxBuff[i] = RxBuff[RX_BYTE(i)];
    }

    if(ui8_g_UART_TxCplt_flag)
    {
        ui8_g_UART_TxCplt_flag = 0;
        HAL_UART_Transmit_DMA(&huart1, TxBuff, ui8_bytes_received);
    }

    // set buffer read index to buffer write index
    ui8_buffer_index1 = ui8_buffer_index2;
}*/
