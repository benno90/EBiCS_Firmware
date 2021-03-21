#include "display_aureus.h"
//#include "main.h"
#include "stm32f1xx_hal.h"


extern UART_HandleTypeDef huart1;
static uint8_t RxBuff[AUREUS_SIZE_RX];
static uint8_t TxBuff[AUREUS_SIZE_TX];

void DisplayAureus_Init(DISPLAY_AUREUS_t* DA_ctx)
{
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE); 			// enable idle line interrupt
  if (HAL_UART_Receive_DMA(&huart1, (uint8_t *) RxBuff, AUREUS_SIZE_RX) != HAL_OK)
  {
 	  Error_Handler();
  }

}

static uint8_t DisplayAureus_CheckSettingsMessage(uint8_t bytes_received)
{
  uint16_t checksum = 0x0;
  uint16_t checksum_rx;

  if(bytes_received != 15)
    return 0;

  // header and trailer

  if(RxBuff[0] != 0x3A)
    return 0;

  if(RxBuff[1] != 0x1A)
    return 0;
  
  if(RxBuff[2] != 0x53)
    return 0;

  if(RxBuff[13] != 0x0D)
    return 0;

  if(RxBuff[14] != 0x0A)
    return 0;

  // checksum
  for(uint8_t i = 1; i <= 10; ++i)
  {
    checksum += RxBuff[i];
  }
  checksum_rx = RxBuff[11] + (RxBuff[12] << 8);
  if(checksum == checksum_rx)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

static uint8_t DisplayAureus_CheckRunningMessage(uint8_t bytes_received)
{
  uint16_t checksum;
  uint16_t checksum_rx;

  if(bytes_received != 10)
    return 0;

  // header and trailer

  if(RxBuff[0] != 0x3A)
    return 0;

  if(RxBuff[1] != 0x1A)
    return 0;

  if(RxBuff[2] != 0x52)
    return 0;

  if(RxBuff[8] != 0x0D)
    return 0;

  if(RxBuff[9] != 0x0A)
    return 0;

  // checksum
  checksum = RxBuff[1] + RxBuff[2] + RxBuff[3] + RxBuff[4] + RxBuff[5];
  checksum_rx = RxBuff[6] + (RxBuff[7] << 8);
  
  if(checksum == checksum_rx)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void DisplayAureus_Service(DISPLAY_AUREUS_t* DA_ctx)
{
  uint16_t checksum;
  uint8_t bytes_received = (AUREUS_SIZE_RX - DMA1_Channel5->CNDTR);

  if(DisplayAureus_CheckRunningMessage(bytes_received))
  {
    //
    // prepare tx message
    //

    TxBuff[0] = 0x3A;
    TxBuff[1] = 0x1A;
    TxBuff[2] = 0x52;
    TxBuff[3] = 0x05;
    TxBuff[4] = 0x0;           // low voltage
    TxBuff[5] = 0x0;          // battery current
    //TxBuff[6] = 0x0D;         // wheel cycle time high byte (0D)
    //TxBuff[7] = 0xC1;         // wheel cycle time low byte (AC)
    TxBuff[6] = (DA_ctx->Tx.Wheeltime_ms & 0xFF00) >> 8;
    TxBuff[7] = (DA_ctx->Tx.Wheeltime_ms & 0x00FF);
    TxBuff[8] = 0x0;          // error code
    checksum = TxBuff[1] + TxBuff[2] + TxBuff[3] + TxBuff[4] + TxBuff[5] + TxBuff[6] + TxBuff[7] + TxBuff[8];
    TxBuff[9] = checksum & 0xFF;
    TxBuff[10] = (checksum & 0xFF00) >> 8;
    TxBuff[11] = 0x0D;
    TxBuff[12] = 0x0A;
    //
    HAL_UART_Transmit_DMA(&huart1, TxBuff, 13);

    //
    // process rx message
    //

    // light
    if(RxBuff[5] & 0x80)
    {
      DA_ctx->Rx.Headlight = 1;
    }
    else
    {
      DA_ctx->Rx.Headlight = 0;
    }

    // pas levels
    switch(RxBuff[4])
    {
      case 0x66:
        DA_ctx->Rx.AssistLevel = 1;
        break;
      case 0x8C:
        DA_ctx->Rx.AssistLevel = 2;
        break;
      case 0xB2:
        DA_ctx->Rx.AssistLevel = 3;
        break;
      case 0xD8:
        DA_ctx->Rx.AssistLevel = 4;
        break;
      case 0xFF:
        DA_ctx->Rx.AssistLevel = 5;
        break;
      default:
        DA_ctx->Rx.AssistLevel = 1;
        break;
    }
  }
  else if(DisplayAureus_CheckSettingsMessage(bytes_received))
  {
    //
    // prepare tx message
    //
    TxBuff[0] = 0x3A;
    TxBuff[1] = 0x1A;
    TxBuff[2] = 0x53;
    TxBuff[3] = 0x05;
    TxBuff[4] = 0x0;           // low voltage
    TxBuff[5] = 0x0;          // battery current
    TxBuff[6] = 0x0D;         // wheel cycle time high byte (0D)
    TxBuff[7] = 0xC1;         // wheel cycle time low byte (AC)
    TxBuff[8] = 0x0;          // error code
    uint16_t checksum = TxBuff[1] + TxBuff[2] + TxBuff[3] + TxBuff[4] + TxBuff[5] + TxBuff[6] + TxBuff[7] + TxBuff[8];
    TxBuff[9] = checksum & 0xFF;
    TxBuff[10] = (checksum & 0xFF00) >> 8;
    TxBuff[11] = 0x0D;
    TxBuff[12] = 0x0A;
    //
    HAL_UART_Transmit_DMA(&huart1, TxBuff, 13);
  }
  		
  // reset dma number of data to be transferred.
  DMA1_Channel5->CNDTR = AUREUS_SIZE_RX;
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
  HAL_UART_Receive_DMA(&huart1, RxBuff, AUREUS_SIZE_RX);

}
