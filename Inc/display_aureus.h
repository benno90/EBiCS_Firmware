
#ifndef _DISPLAY_AUREUS_H
#define _DISPLAY_AUREUS_H


#include "config.h"

//#define AUREUS_SIZE_RX_SETTINGS 15
//#define AUREUS_SIZE_RX_RUNNING 10
#define AUREUS_SIZE_DMA_BUFFER 30
#define AUREUS_SIZE_TX 13

typedef struct
{
  // Parameters received from display in setting mode:
  uint8_t  dummy;
} RX_SETTINGS_t;


typedef struct
{
  // Parameters received from display in operation mode:
  uint8_t  AssistLevel;               // 0..255 Power Assist Level
  uint8_t  Headlight;                 // KM_HEADLIGHT_OFF / KM_HEADLIGHT_ON / KM_HEADLIGHT_LOW / KM_HEADLIGHT_HIGH
} RX_PARAM_t;



typedef struct
{
  // Parameters to be send to display in operation mode:
  uint8_t  Battery;                   // KM_BATTERY_NORMAL / KM_BATTERY_LOW
  uint16_t Wheeltime_ms;              // Unit:1ms
  uint8_t  Error;                     // KM_ERROR_NONE, ..
  uint16_t Current_A_x3;               // Unit: Ampere x3
} TX_PARAM_t;



typedef struct
{
    RX_SETTINGS_t   Settings;
    RX_PARAM_t      Rx;
    TX_PARAM_t      Tx;

} DISPLAY_AUREUS_t;


void Aureus_UART_IdleItCallback(void);
void DisplayAureus_Init(DISPLAY_AUREUS_t* DA_ctx);
void DisplayAureus_Service(DISPLAY_AUREUS_t* DA_ctx);


#endif  //#define _DISPLAY_AUREUS_H