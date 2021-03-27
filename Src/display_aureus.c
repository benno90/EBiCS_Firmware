#include "display_aureus.h"
#include "main.h"
#include "stm32f1xx_hal.h"


#define RX_BYTE(b) (buffer_index1 + b) % AUREUS_SIZE_DMA_BUFFER

extern volatile uint8_t ui8_UART_flag;
extern UART_HandleTypeDef huart1;


static uint32_t prevCNDTR = AUREUS_SIZE_DMA_BUFFER;
static uint8_t buffer_index1 = 0;           // circular buffer read position
static uint8_t buffer_index2 = 0;           // circular buffer write position
static uint8_t bytes_received = 0;
static uint8_t RxBuff[AUREUS_SIZE_DMA_BUFFER];
static uint8_t TxBuff[AUREUS_SIZE_TX];

void DisplayAureus_Init(DISPLAY_AUREUS_t* DA_ctx)
{
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE); 			// enable idle line interrupt
  if (HAL_UART_Receive_DMA(&huart1, (uint8_t *) RxBuff, AUREUS_SIZE_DMA_BUFFER) != HAL_OK)
  {
 	  Error_Handler();
  }

}

void My_UART_IdleItCallback(void)
{
  if(prevCNDTR < DMA1_Channel5->CNDTR)
  {
    bytes_received = AUREUS_SIZE_DMA_BUFFER - DMA1_Channel5->CNDTR + prevCNDTR; 
  }
  else
  {
    bytes_received = (prevCNDTR - DMA1_Channel5->CNDTR);
  }
  prevCNDTR = DMA1_Channel5->CNDTR;
  buffer_index2 = (buffer_index2 + bytes_received) % AUREUS_SIZE_DMA_BUFFER;
  
  ui8_UART_flag = 1; // global

  //buffer_index1 = buffer_index2;


}

static uint8_t DisplayAureus_CheckSettingsMessage(uint8_t bytes_received)
{
  uint16_t checksum = 0x0;
  uint16_t checksum_rx;

  if(bytes_received != 15)
    return 0;

  // header and trailer

  if(RxBuff[RX_BYTE(0)] != 0x3A)
    return 0;

  if(RxBuff[RX_BYTE(1)] != 0x1A)
    return 0;
  
  if(RxBuff[RX_BYTE(2)] != 0x53)
    return 0;

  if(RxBuff[RX_BYTE(13)] != 0x0D)
    return 0;

  if(RxBuff[RX_BYTE(14)] != 0x0A)
    return 0;

  // checksum
  for(uint8_t i = 1; i <= 10; ++i)
  {
    checksum += RxBuff[RX_BYTE(i)];
  }
  checksum_rx = RxBuff[RX_BYTE(11)] + (RxBuff[RX_BYTE(12)] << 8);
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

  if(RxBuff[RX_BYTE(0)] != 0x3A)
    return 0;

  if(RxBuff[RX_BYTE(1)] != 0x1A)
    return 0;

  if(RxBuff[RX_BYTE(2)] != 0x52)
    return 0;

  if(RxBuff[RX_BYTE(8)] != 0x0D)
    return 0;

  if(RxBuff[RX_BYTE(9)] != 0x0A)
    return 0;

  // checksum
  checksum = RxBuff[RX_BYTE(1)] + RxBuff[RX_BYTE(2)] + RxBuff[RX_BYTE(3)] + RxBuff[RX_BYTE(4)] + RxBuff[RX_BYTE(5)];
  checksum_rx = RxBuff[RX_BYTE(6)] + (RxBuff[RX_BYTE(7)] << 8);
  
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
  //uint8_t bytes_received = (AUREUS_SIZE_DMA_BUFFER - DMA1_Channel5->CNDTR);

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
    if(RxBuff[RX_BYTE(5)] & 0x80)
    {
      DA_ctx->Rx.Headlight = 1;
    }
    else
    {
      DA_ctx->Rx.Headlight = 0;
    }

    // pas levels
    switch(RxBuff[RX_BYTE(4)])
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
    // 27.03.21: display did not change to running mode with wheel cycle time = 0x0DC1 (3521), but when setting it to 0x0D75 (3446) it worked ..
    TxBuff[6] = 0x0D;         // wheel cycle time high byte (0D)
    TxBuff[7] = 0x76;         // wheel cycle time low byte (AC)
    //TxBuff[7] = 0xC1;         // wheel cycle time low byte (AC)
    TxBuff[8] = 0x0;          // error code
    uint16_t checksum = TxBuff[1] + TxBuff[2] + TxBuff[3] + TxBuff[4] + TxBuff[5] + TxBuff[6] + TxBuff[7] + TxBuff[8];
    TxBuff[9] = checksum & 0xFF;
    TxBuff[10] = (checksum & 0xFF00) >> 8;
    TxBuff[11] = 0x0D;
    TxBuff[12] = 0x0A;
    //
    HAL_UART_Transmit_DMA(&huart1, TxBuff, AUREUS_SIZE_TX);
  }
  		
  // reset dma number of data to be transferred.
  //DMA1_Channel5->CNDTR = AUREUS_SIZE_DMA_BUFFER;
  //__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
  //HAL_UART_Receive_DMA(&huart1, RxBuff, AUREUS_SIZE_DMA_BUFFER);


  // set buffer read index to buffer write index
  buffer_index1 = buffer_index2;

}
