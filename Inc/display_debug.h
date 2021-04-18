#include "config.h"

#define DEBUG_SIZE_RX_DMA_BUFFER 20
#define DEBUG_SIZE_TX_DMA_BUFFER 20

typedef struct
{
    uint8_t light;
    uint16_t current_target;

} DISPLAY_DEBUG_t;

void Debug_UART_IdleItCallback(void);
void DisplayDebug_Init(DISPLAY_DEBUG_t *DD_ctx);
void DisplayDebug_Service(DISPLAY_DEBUG_t *DD_ctx);
