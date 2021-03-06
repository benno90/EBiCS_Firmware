#include "display_aureus.h"
//#include "main.h"
#include "stm32f1xx_hal.h"



extern UART_HandleTypeDef huart1;
   
   
static uint8_t RxBuff[AUREUS_SIZE_RX_SETTINGS];
static uint8_t TxBuff[AUREUS_SIZE_TX];

void DisplayAureus_Init(DISPLAY_AUREUS_t* DA_ctx)
{
  if (HAL_UART_Receive_DMA(&huart1, (uint8_t *) RxBuff, AUREUS_SIZE_RX_SETTINGS) != HAL_OK)
  {
 	  Error_Handler();
  }

}


void DisplayAureus_Service(DISPLAY_AUREUS_t* DA_ctx)
{
  static uint8_t running_state = 0;

  TxBuff[0] = 0x3A;
  TxBuff[1] = 0x1A;
  TxBuff[2] = running_state ? 0x52 : 0x53;
  TxBuff[3] = 0x05;
  TxBuff[4] = 0x0;          // low voltage
  TxBuff[5] = 0x0;          // battery current
  TxBuff[6] = 0x01;         // wheel cycle time high byte (0D)
  TxBuff[7] = 0xF4;         // wheel cycle time low byte (AC)
  TxBuff[8] = 0x0;          // error code
  uint16_t checksum = TxBuff[1] + TxBuff[2] + TxBuff[3] + TxBuff[4] + TxBuff[5] + TxBuff[6] + TxBuff[7] + TxBuff[8];
  TxBuff[9] = checksum & 0xFF;
  TxBuff[10] = (checksum & 0xFF00) >> 8;
  TxBuff[11] = 0x0D;
  TxBuff[12] = 0x0A;
        
  HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&TxBuff, AUREUS_SIZE_TX);


  running_state = 1;
}
