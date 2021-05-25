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
static char TxBuff[DEBUG_SIZE_TX_DMA_BUFFER];
static char Buff[DEBUG_SIZE_RX_DMA_BUFFER];

static uint8_t do_log = 0;

void DisplayDebug_Init(DISPLAY_DEBUG_t *DD_ctx)
{
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE); // enable idle line interrupt
    if (HAL_UART_Receive_DMA(&huart1, (uint8_t *)RxBuff, DEBUG_SIZE_RX_DMA_BUFFER) != HAL_OK)
    {
        Error_Handler();
    }

    //DD_ctx->go = 0;
    //DD_ctx->log = 0;
    //DD_ctx->light = 0;
    //DD_ctx->ui16_value = 0;
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

void debug_print(char* data, uint16_t size)
{
    if(size >= DEBUG_SIZE_TX_DMA_BUFFER)
    {
        return;
    }
    
    if(ui8_UART_TxCplt_flag && do_log)
    {
        HAL_UART_Transmit_DMA(&huart1, (uint8_t*) data, size);
        ui8_UART_TxCplt_flag = 0;
    }
}

static void debug_print2(char* data, uint16_t size)
{
    if(size >= DEBUG_SIZE_TX_DMA_BUFFER)
    {
        return;
    }

    if(ui8_UART_TxCplt_flag)
    {
        HAL_UART_Transmit_DMA(&huart1, (uint8_t*) data, size);
        ui8_UART_TxCplt_flag = 0;
    }
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
    Buff[bytes_received] = '\0';


    if (strncmp((char *) Buff, "TOGGLE ", 7) == 0)
    {
        // serialize bluetooth app -> select the signal to plot
        switch ((char) Buff[7])
        {
            case '1':
                DD_ctx->ui16_value2 = 1;
                break;
            case '2': 
                DD_ctx->ui16_value2 = 2;
                break;
            case '3':
                DD_ctx->ui16_value2 = 3;
                break;
            case '4': 
                DD_ctx->ui16_value2 = 4;
                break;
            case '5':
                DD_ctx->ui16_value2 = 5;
                break;
            case '6': 
                DD_ctx->ui16_value2 = 6;
                break;
            default:
                break;
        }
    }
    else if(strncmp(Buff, "go", DEBUG_SIZE_RX_DMA_BUFFER) == 0)
    {
        DD_ctx->go = !(DD_ctx->go);
        if(DD_ctx->go)
        {
            strcpy(TxBuff, "go!\n");
            debug_print2(TxBuff, 4);
        }
        else
        {
            strcpy(TxBuff, "stop\n");
            debug_print2(TxBuff, 5);
        }
    }
    else if(strncmp(Buff, "v ", 2) == 0)
    {
        // parse value
        DD_ctx->ui16_value = atoi(&Buff[2]);
        if(DD_ctx->ui16_value > 40)
        {
            DD_ctx->ui16_value = 40;
        }
        sprintf_(TxBuff, "ui16_value = %u\n", DD_ctx->ui16_value);
        debug_print2(TxBuff, strlen(TxBuff));
    }
    else if(strncmp(Buff, "x ", 2) == 0)
    {
        // parse value
        DD_ctx->ui16_value2 = atoi(&Buff[2]);
        if(DD_ctx->ui16_value2 > 20)
        {
            DD_ctx->ui16_value2 = 20;
        }
        sprintf_(TxBuff, "ui16_value2 = %u\n", DD_ctx->ui16_value2);
        debug_print2(TxBuff, strlen(TxBuff));
    }
    else if(strncmp(Buff, "log", DEBUG_SIZE_RX_DMA_BUFFER) == 0)
    {
        DD_ctx->log = !(DD_ctx->log);
        do_log = DD_ctx->log;
        ui8_UART_TxCplt_flag = 1;
        if(DD_ctx->log)
        {
            strcpy(TxBuff, "log on\n");
            debug_print2(TxBuff, 7);
        }
        else
        {
            strcpy(TxBuff, "log off\n");
            debug_print2(TxBuff, 8);
        }
    }
    else if(strncmp(Buff, "hello", DEBUG_SIZE_RX_DMA_BUFFER) == 0)
    {
        strcpy(TxBuff, "hello\n");
        ui8_UART_TxCplt_flag = 1;
        debug_print2(TxBuff, 6);
    }
    else if(strncmp(Buff, "light", DEBUG_SIZE_RX_DMA_BUFFER) == 0)
    {
        // toggle light
        DD_ctx->light = !(DD_ctx->light);
        if(DD_ctx->light)
        {
            strcpy(TxBuff, "light on\n");
            debug_print2(TxBuff, 9);
        }
        else
        {
            strcpy(TxBuff, "light off\n");
            debug_print2(TxBuff, 10);
        }
    }


    // set buffer read index to buffer write index
    buffer_index1 = buffer_index2;
}
