#include "display_base.h"
#include "main.h"
#include "config.h"
#include "stm32f1xx_hal.h"


#if DISPLAY_TYPE == DISPLAY_TYPE_AUREUS


#define AUREUS_SIZE_RX_SETTINGS 15
#define AUREUS_SIZE_RX_RUNNING 10
#define AUREUS_SIZE_TX 13


static const uint8_t shake_hands_byte[64] = {0x89, 0x9F, 0x86, 0xF9, 0x58, 0x0B, 0xFA, 0x3D, 0x21, 0x96, 0x03, 0xC1,
                                             0x76, 0x8D, 0xD1, 0x5E, 0xE2, 0x44, 0x92, 0x9E, 0x91, 0x7F, 0xD8, 0x3E, 0x74, 0xE6, 0x65, 0xD3, 0xFB, 0x36, 0xE5, 0xF7,
                                             0x14, 0xDE, 0x3B, 0x3F, 0x23, 0xFC, 0x8E, 0xEE, 0x17, 0xC5, 0x54, 0x4D, 0x93, 0xAD, 0xD2, 0x39, 0x8E, 0xDF, 0x9D, 0x61,
                                             0x24, 0xA0, 0xE5, 0xED, 0x4B, 0x50, 0x25, 0x71, 0x9A, 0x58, 0x17, 0x78};

extern UART_HandleTypeDef huart1;

extern uint8_t ui8_buffer_index1; // circular buffer read position
extern uint8_t ui8_buffer_index2; // circular buffer write position
extern uint8_t ui8_bytes_received;
extern uint8_t RxBuff[DISPLAY_SIZE_RX_BUFFER];
extern uint8_t TxBuff[DISPLAY_SIZE_TX_BUFFER];

static uint8_t DisplayAureus_CheckSettingsMessage(uint8_t bytes_received)
{
    uint16_t checksum = 0x0;
    uint16_t checksum_rx;

    if (bytes_received != AUREUS_SIZE_RX_SETTINGS)
        return 0;

    // header and trailer

    if (RxBuff[RX_BYTE(0)] != 0x3A)
        return 0;

    if (RxBuff[RX_BYTE(1)] != 0x1A)
        return 0;

    if (RxBuff[RX_BYTE(2)] != 0x53)
        return 0;

    if (RxBuff[RX_BYTE(9)] > 63) // according to the protocol specification the shake hands byte should be in the range 0-63
        return 0;

    if (RxBuff[RX_BYTE(13)] != 0x0D)
        return 0;

    if (RxBuff[RX_BYTE(14)] != 0x0A)
        return 0;

    // checksum
    for (uint8_t i = 1; i <= 10; ++i)
    {
        checksum += RxBuff[RX_BYTE(i)];
    }
    checksum_rx = RxBuff[RX_BYTE(11)] + (RxBuff[RX_BYTE(12)] << 8);
    if (checksum == checksum_rx)
        return 1;
    else
        return 0;
}

static uint8_t DisplayAureus_CheckRunningMessage(uint8_t bytes_received)
{
    uint16_t checksum;
    uint16_t checksum_rx;

    if (bytes_received != AUREUS_SIZE_RX_RUNNING)
        return 0;

    // header and trailer

    if (RxBuff[RX_BYTE(0)] != 0x3A)
        return 0;

    if (RxBuff[RX_BYTE(1)] != 0x1A)
        return 0;

    if (RxBuff[RX_BYTE(2)] != 0x52)
        return 0;

    if (RxBuff[RX_BYTE(8)] != 0x0D)
        return 0;

    if (RxBuff[RX_BYTE(9)] != 0x0A)
        return 0;

    // checksum
    checksum = RxBuff[RX_BYTE(1)] + RxBuff[RX_BYTE(2)] + RxBuff[RX_BYTE(3)] + RxBuff[RX_BYTE(4)] + RxBuff[RX_BYTE(5)];
    checksum_rx = RxBuff[RX_BYTE(6)] + (RxBuff[RX_BYTE(7)] << 8);

    if (checksum == checksum_rx)
        return 1;
    else
        return 0;
}

void Display_Service(MotorState_t *pMS)
{
    uint16_t ui16_checksum;
    uint16_t ui16_temp;

    if (DisplayAureus_CheckRunningMessage(ui8_bytes_received))
    {
        //
        // prepare tx message
        //

        TxBuff[0] = 0x3A;
        TxBuff[1] = 0x1A;
        TxBuff[2] = 0x52;
        TxBuff[3] = 0x05;
        TxBuff[4] = 0x0;                     // low voltage
        // battery current
        if(pMS->q31_battery_current_mA > 0)
            ui16_temp = pMS->q31_battery_current_mA / 333;  // the display expects the batter current in ampere x 3
        else
            ui16_temp = 0;
        TxBuff[5] = ui16_temp;
        //TxBuff[6] = 0x0D;         // wheel cycle time high byte (0D)
        //TxBuff[7] = 0xC1;         // wheel cycle time low byte (AC)
        // wheel time
		if(pMS->ui16_wheel_time_ms > 0x0DAC)
            ui16_temp = 0x0DAC;
        else
            ui16_temp = pMS->ui16_wheel_time_ms;
        TxBuff[6] = (ui16_temp & 0xFF00) >> 8;
        TxBuff[7] = (ui16_temp & 0x00FF);
        // error state
        switch(pMS->error_state)
        {
            case 0:         //MOTOR_STATE_NORMAL:
                TxBuff[8] = 0;
                break;
            case 1:         //MOTOR_STATE_BLOCKED:
                TxBuff[8] = 0x21;
                //DA.Tx.Error = 0;
                break;
            case 2:         // MOTOR_STATE_PLL_ERROR:
                TxBuff[8] = 0x22;
                break;
            default:
                TxBuff[8] = 0;
                break;
        }
        //
        ui16_checksum = TxBuff[1] + TxBuff[2] + TxBuff[3] + TxBuff[4] + TxBuff[5] + TxBuff[6] + TxBuff[7] + TxBuff[8];
        TxBuff[9] = ui16_checksum & 0xFF;
        TxBuff[10] = (ui16_checksum & 0xFF00) >> 8;
        TxBuff[11] = 0x0D;
        TxBuff[12] = 0x0A;
        //
        HAL_UART_Transmit_DMA(&huart1, TxBuff, AUREUS_SIZE_TX);

        //
        // process rx message
        //

        // light
        if (RxBuff[RX_BYTE(5)] & 0x80)
        {
            pMS->ui8_lights = 1;
        }
        else
        {
            pMS->ui8_lights = 0;
        }

        // pas levels
        pMS->ui8_assist_level = RxBuff[RX_BYTE(4)];
    }
    else if (DisplayAureus_CheckSettingsMessage(ui8_bytes_received))
    {
        //
        // prepare tx message
        //
        TxBuff[0] = 0x3A;
        TxBuff[1] = 0x1A;
        TxBuff[2] = 0x53;
        TxBuff[3] = 0x05;
        TxBuff[4] = 0x0;  // low voltage
        TxBuff[5] = 0x0;  // battery current
        TxBuff[6] = 0x0D; // wheel cycle time high byte (0D)
        TxBuff[7] = shake_hands_byte[RxBuff[RX_BYTE(9)]];
        TxBuff[8] = 0x0; // error code
        uint16_t ui16_checksum = TxBuff[1] + TxBuff[2] + TxBuff[3] + TxBuff[4] + TxBuff[5] + TxBuff[6] + TxBuff[7] + TxBuff[8];
        TxBuff[9] = ui16_checksum & 0xFF;
        TxBuff[10] = (ui16_checksum & 0xFF00) >> 8;
        TxBuff[11] = 0x0D;
        TxBuff[12] = 0x0A;
        //
        HAL_UART_Transmit_DMA(&huart1, TxBuff, AUREUS_SIZE_TX);
    }

    // set buffer read index to buffer write index
    ui8_buffer_index1 = ui8_buffer_index2;
}


#endif