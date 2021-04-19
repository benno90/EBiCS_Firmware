#include "config.h"

#define DEBUG_SIZE_RX_DMA_BUFFER 40
#define DEBUG_SIZE_TX_DMA_BUFFER 40

typedef struct
{
    uint8_t go;
    uint8_t log;
    uint8_t light;
    uint16_t ui16_value;

} DISPLAY_DEBUG_t;

void Debug_UART_IdleItCallback(void);
void DisplayDebug_Init(DISPLAY_DEBUG_t *DD_ctx);
void DisplayDebug_Service(DISPLAY_DEBUG_t *DD_ctx);

void debug_print(uint8_t* data, uint16_t size);