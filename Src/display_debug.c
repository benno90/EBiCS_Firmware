#include "display_base.h"
#include "main.h"
#include "config.h"
#include "stm32f1xx_hal.h"
#include "print.h"
#include <stdlib.h> // atoi


#if DISPLAY_TYPE == DISPLAY_TYPE_DEBUG



extern UART_HandleTypeDef huart1;

extern uint8_t ui8_buffer_index1; // circular buffer read position
extern uint8_t ui8_buffer_index2; // circular buffer write position
extern uint8_t ui8_bytes_received;
extern uint8_t RxBuff[DISPLAY_SIZE_RX_BUFFER];
extern uint8_t TxBuff[DISPLAY_SIZE_TX_BUFFER];

static uint8_t do_log = 1;

void debug_print(uint8_t* data, uint16_t size)
{
    if(size >= DISPLAY_SIZE_TX_BUFFER)
    {
        return;
    }
    
    if(ui8_g_UART_TxCplt_flag && do_log)
    {
        HAL_UART_Transmit_DMA(&huart1, (uint8_t*) data, size);
        ui8_g_UART_TxCplt_flag = 0;
    }
}

void debug_print2(uint8_t* data, uint16_t size)
{
    if(size >= DISPLAY_SIZE_TX_BUFFER)
    {
        return;
    }

    if(ui8_g_UART_TxCplt_flag)
    {
        HAL_UART_Transmit_DMA(&huart1, (uint8_t*) data, size);
        ui8_g_UART_TxCplt_flag = 0;
    }
}

void Display_Service(MotorState_t *pMS)
{
    if(ui8_bytes_received >= DISPLAY_SIZE_TX_BUFFER - 1)
    {
        return;
    }

    // echo received message

    //for (uint8_t i = 0; i < bytes_received; ++i)
    //{
    //    TxBuff[i] = RxBuff[RX_BYTE(i)];
    //}
    //HAL_UART_Transmit_DMA(&huart1, TxBuff, bytes_received);

    
    for (uint8_t i = 0; i < ui8_bytes_received; ++i)
    {
        TxBuff[i] = RxBuff[RX_BYTE(i)];
    }
    TxBuff[ui8_bytes_received] = '\0';


    if (strncmp((char *) TxBuff, "TOGGLE ", 7) == 0)
    {
        // serialize bluetooth app -> select the signal to plot
        do_log = 1;
        pMS->ui8_log = 1;
        switch ((char) TxBuff[7])
        {
            case '1':
                pMS->ui8_dbg_log_value = 1;
                break;
            case '2': 
                pMS->ui8_dbg_log_value = 2;
                break;
            case '3':
                pMS->ui8_dbg_log_value = 3;
                break;
            case '4': 
                pMS->ui8_dbg_log_value = 4;
                break;
            case '5':
                pMS->ui8_dbg_log_value = 5;
                break;
            case '6': 
                pMS->ui8_dbg_log_value = 6;
                break;
            default:
                break;
        }
    }
    else if(strncmp((char *) TxBuff, "go", 2) == 0)
    {
        pMS->ui8_go = !(pMS->ui8_go);
        if(pMS->ui8_go)
        {
            strcpy((char *) TxBuff, "go!\n");
            debug_print2(TxBuff, 4);
        }
        else
        {
            strcpy((char *) TxBuff, "stop\n");
            debug_print2(TxBuff, 5);
        }
    }
    else if(strncmp((char *) TxBuff, "v ", 2) == 0)
    {
        // parse value
        pMS->ui16_dbg_value = atoi((char *) &(TxBuff[2]));
        if(pMS->ui16_dbg_value > 2000)
        {
            pMS->ui16_dbg_value = 200;
        }
        sprintf_((char *) TxBuff, "ui16_value = %u\n", pMS->ui16_dbg_value);
        debug_print2(TxBuff, strlen((char *) TxBuff));
    }
    else if(strncmp((char *) TxBuff, "x ", 2) == 0)
    {
        // parse value
        pMS->ui16_dbg_value2 = atoi((char *) &(TxBuff[2]));
        if(pMS->ui16_dbg_value2 > 20)
        {
            pMS->ui16_dbg_value2 = 20;
        }
        sprintf_((char *) TxBuff, "ui16_value2 = %u\n", pMS->ui16_dbg_value2);
        debug_print2(TxBuff, strlen((char *) TxBuff));
    }
    else if(strncmp((char *) TxBuff, "l ", 2) == 0)
    {
        // parse value
        pMS->ui8_dbg_log_value = atoi((char *) &(TxBuff[2]));
        if(pMS->ui8_dbg_log_value > 10)
        {
            pMS->ui8_dbg_log_value = 0;
        }
        sprintf_((char *) TxBuff, "dbg log = %u\n", pMS->ui8_dbg_log_value);
        debug_print2(TxBuff, strlen((char *) TxBuff));
    }
    else if(strncmp((char *) TxBuff, "log", 3) == 0)
    {
        pMS->ui8_log = !(pMS->ui8_log);
        do_log = pMS->ui8_log;
        ui8_g_UART_TxCplt_flag = 1;
        if(pMS->ui8_log)
        {
            strcpy((char *) TxBuff, "log on\n");
            debug_print2(TxBuff, 7);
        }
        else
        {
            strcpy((char *) TxBuff, "log off\n");
            debug_print2(TxBuff, 8);
        }
    }
    else if(strncmp((char *) TxBuff, "hello", 5) == 0)
    {
        strcpy((char *) TxBuff, "hello\n");
        ui8_g_UART_TxCplt_flag = 1;
        debug_print2(TxBuff, 6);
    }
    else if(strncmp((char *) TxBuff, "light", 5) == 0)
    {
        // toggle light
        pMS->ui8_lights = !(pMS->ui8_lights);
        if(pMS->ui8_lights)
        {
            strcpy((char *) TxBuff, "light on\n");
            debug_print2(TxBuff, 9);
        }
        else
        {
            strcpy((char *) TxBuff, "light off\n");
            debug_print2(TxBuff, 10);
        }
    }
    else if(strncmp((char *) TxBuff, "angleopt", 8) == 0)
    {
        if(!pMS->ui8_rotor_angle_opt_flag)
        {
            pMS->ui8_rotor_angle_opt_flag = 1;
        }
    }


    // set buffer read index to buffer write index
    ui8_buffer_index1 = ui8_buffer_index2;
}


#endif
