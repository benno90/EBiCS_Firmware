/*
 * config.h
 *
 *  Automatically created by Lishui Parameter Configurator
 *  Author: stancecoke
 */

#ifndef CONFIG_H_
#define CONFIG_H_
#include "stdint.h"
#define DISPLAY_TYPE_AUREUS (1<<6)
#define DISPLAY_TYPE_EBiCS (1<<5)                  // King-Meter 618U protocol (KM5s, EBS-LCD2, J-LCD, SW-LCD)
#define DISPLAY_TYPE_KINGMETER_618U (1<<3)                  // King-Meter 618U protocol (KM5s, EBS-LCD2, J-LCD, SW-LCD)
#define DISPLAY_TYPE_KINGMETER_901U (1<<4)                  // King-Meter 901U protocol (KM5s)
#define DISPLAY_TYPE_KINGMETER      (DISPLAY_TYPE_KINGMETER_618U|DISPLAY_TYPE_KINGMETER_901U)
#define DISPLAY_TYPE_BAFANG (1<<2)							// For 'Blaupunkt' Display of Prophete Entdecker
#define DISPLAY_TYPE_KUNTENG (1<<1)							// For ASCII-Output in Debug mode
#define DISPLAY_TYPE_DEBUG (1<<0)							// For ASCII-Output in Debug mode);

#define TRIGGER_OFFSET_ADC 50
#define TRIGGER_DEFAULT 2020
#define _T 2028
#define CAL_BAT_V 25          //256
#define CAL_V 25
#define CAL_I 38LL<<8
#define INDUCTANCE 6LL
#define RESISTANCE 40LL
#define FLUX_LINKAGE 1200LL
#define GAMMA 9LL
#define BATTERY_LEVEL_1 323000
#define BATTERY_LEVEL_2 329000
#define BATTERY_LEVEL_3 344000
#define BATTERY_LEVEL_4 368000
#define BATTERY_LEVEL_5 380000
#define P_FACTOR_I_Q 0 //500
#define I_FACTOR_I_Q 20
#define P_FACTOR_I_D 0 //500
#define I_FACTOR_I_D 20
#define P_FACTOR_SPEED 100
#define I_FACTOR_SPEED 10
#define TS_COEF 90000
#define PAS_TIMEOUT 4000
#define RAMP_END 1600
#define THROTTLE_OFFSET 50
#define THROTTLE_MAX 4000
#define WHEEL_CIRCUMFERENCE 2200
#define GEAR_RATIO 18 //dummy for testing
#define SPEEDLIMIT 50
#define PULSES_PER_REVOLUTION 1
#define PH_CURRENT_MAX 900
#define BATTERYCURRENT_MAX 15000
#define SPEC_ANGLE -167026406L //BionX IGH3 -143165476
//#define DIRDET
#define FRAC_HIGH 30
#define FRAC_LOW 15
#define TS_MODE
#define TQ_ADC_INDEX 6  // AD1: 6, SP: 1
#define DISPLAY_TYPE DISPLAY_TYPE_DEBUG
//#define DISPLAY_TYPE DISPLAY_TYPE_AUREUS
#define REVERSE 1
#define PUSHASSIST_CURRENT 30
#define VOLTAGE_MIN 300
#define REGEN_CURRENT 0
//#define FAST_LOOP_LOG
//#define DISABLE_DYNAMIC_ADC
//#define INDIVIDUAL_MODES
//#define SPEEDTHROTTLE
//#define THROTTLE_OVERRIDE
#define REGEN_CURRENT_MAX 0
#define SPEED_PLL
#define P_FACTOR_PLL  8 // 8 //  11  //6
#define I_FACTOR_PLL  9 //9 // 10  //11
#define AUTODETECT 0

#define EXTERNAL 1
#define INTERNAL 0
#define SPEEDSOURCE INTERNAL
#define SPEEDFILTER 1

#define SIXSTEPTHRESHOLD_UP 7300            // 5 kmh    
#define SIXSTEPTHRESHOLD_DOWN 12222         // 3 kmh

#endif /* CONFIG_H_ */
