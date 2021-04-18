#include "display_debug.h"
#include "main.h"
#include "stm32f1xx_hal.h"

#define RX_BYTE(b) (buffer_index1 + b) % DEBUG_SIZE_RX_DMA_BUFFER

extern volatile uint8_t ui8_UART_flag;
extern UART_HandleTypeDef huart1;

static uint32_t prevCNDTR = DEBUG_SIZE_RX_DMA_BUFFER;
static uint8_t buffer_index1 = 0; // circular buffer read position
static uint8_t buffer_index2 = 0; // circular buffer write position
static uint8_t bytes_received = 0;
static uint8_t RxBuff[DEBUG_SIZE_RX_DMA_BUFFER];
static uint8_t TxBuff[DEBUG_SIZE_TX_DMA_BUFFER];
static uint8_t Buff[DEBUG_SIZE_RX_DMA_BUFFER];

void DisplayDebug_Init(DISPLAY_DEBUG_t *DD_ctx)
{
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE); // enable idle line interrupt
    if (HAL_UART_Receive_DMA(&huart1, (uint8_t *)RxBuff, DEBUG_SIZE_RX_DMA_BUFFER) != HAL_OK)
    {
        Error_Handler();
    }

    DD_ctx->light = 0;
    DD_ctx->current_target = 0;
}

void Debug_UART_IdleItCallback(void)
{
    if (prevCNDTR < DMA1_Channel5->CNDTR)
    {
        bytes_received = DEBUG_SIZE_RX_DMA_BUFFER - DMA1_Channel5->CNDTR + prevCNDTR;
    }
    else
    {
        bytes_received = (prevCNDTR - DMA1_Channel5->CNDTR);
    }
    prevCNDTR = DMA1_Channel5->CNDTR;
    buffer_index2 = (buffer_index2 + bytes_received) % DEBUG_SIZE_RX_DMA_BUFFER;

    ui8_UART_flag = 1; // global

    //buffer_index1 = buffer_index2;
}

void DisplayDebug_Service(DISPLAY_DEBUG_t *DD_ctx)
{
    if(bytes_received >= DEBUG_SIZE_RX_DMA_BUFFER)
    {
        return;
    }

    // echo received message

    //for (uint8_t i = 0; i < bytes_received; ++i)
    //{
    //    TxBuff[i] = RxBuff[RX_BYTE(i)];
    //}
    //HAL_UART_Transmit_DMA(&huart1, TxBuff, bytes_received);

    
    for (uint8_t i = 0; i < bytes_received; ++i)
    {
        Buff[i] = RxBuff[RX_BYTE(i)];
    }

    if(strncmp(Buff, "hello", DEBUG_SIZE_RX_DMA_BUFFER) == 0)
    {
        strcpy(TxBuff, "hello\n");
        HAL_UART_Transmit_DMA(&huart1, TxBuff, 6);
    }
    
    if(strncmp(Buff, "light", DEBUG_SIZE_RX_DMA_BUFFER) == 0)
    {
        // toggle light
        DD_ctx->light = !(DD_ctx->light);
        if(DD_ctx->light)
        {
            strcpy(TxBuff, "light on\n");
            HAL_UART_Transmit_DMA(&huart1, TxBuff, 9);
        }
        else
        {
            strcpy(TxBuff, "light off\n");
            HAL_UART_Transmit_DMA(&huart1, TxBuff, 10);
        }
    }


    // set buffer read index to buffer write index
    buffer_index1 = buffer_index2;
}