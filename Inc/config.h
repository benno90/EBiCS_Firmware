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
//#define BLUETOOTH_SERIALIZE_DISPLAY

#define TRIGGER_OFFSET_ADC 100 // 50
#define _T 2048     // 2028
#define TRIGGER_DEFAULT 2047
#define PWM_DEAD_TIME 32

#define CAL_BAT_V 25   
#define CAL_V 25     // adcData[0] * CAL_V = voltage in mV
#define CAL_I 55      //38       
#define INDUCTANCE 6LL
#define RESISTANCE 40LL
#define FLUX_LINKAGE 1200LL
#define GAMMA 9LL
#define BATTERY_LEVEL_1 323000
#define BATTERY_LEVEL_2 329000
#define BATTERY_LEVEL_3 344000
#define BATTERY_LEVEL_4 368000
#define BATTERY_LEVEL_5 380000
#define P_FACTOR_I_Q 10 //500    // 0
#define I_FACTOR_I_Q 2 //20     // 4
#define P_FACTOR_I_D 500 //500
#define I_FACTOR_I_D 60
#define P_FACTOR_SPEED 100
#define I_FACTOR_SPEED 10
#define TS_COEF 90000
#define PAS_TIMEOUT 4000
#define RAMP_END 1600
#define THROTTLE_OFFSET 50
#define THROTTLE_MAX 4000
#define WHEEL_CIRCUMFERENCE 2234
#define GEAR_RATIO 24
#define SPEEDLIMIT 45
#define PULSES_PER_REVOLUTION 1
#define PH_CURRENT_MAX 400 //350          // in ampere x 10
#define BATTERYCURRENT_MAX 180      // im ampere x 10
#define SPEC_ANGLE -167026406L //BionX IGH3 -143165476
//#define DIRDET
#define FRAC_HIGH 30
#define FRAC_LOW 15
#define TS_MODE
#define TQ_ADC_INDEX 6  // AD1: 6, SP: 1
#define TEMP_ADC_INDEX 5 // ADC_CHANNEL_TEMPSENSOR
//#define DISPLAY_TYPE DISPLAY_TYPE_DEBUG
#define DISPLAY_TYPE DISPLAY_TYPE_AUREUS
#define REVERSE 1
#define PUSHASSIST_CURRENT 30
#define VOLTAGE_MIN 300
#define REGEN_CURRENT 0
//#define FAST_LOOP_LOG
#define FAST_LOOP_LOG_SIZE 256
//#define DISABLE_DYNAMIC_ADC
//#define INDIVIDUAL_MODES
//#define SPEEDTHROTTLE
//#define THROTTLE_OVERRIDE
#define REGEN_CURRENT_MAX 0
#define SPEED_PLL
#define P_FACTOR_PLL  9  // 8 // 8 //  11  //6
#define I_FACTOR_PLL  7 //9 //9 // 10  //11
#define AUTODETECT 0

#define EXTERNAL 1
#define INTERNAL 0
#define SPEEDSOURCE INTERNAL
#define SPEEDFILTER 1

#define SIXSTEPTHRESHOLD_UP 5586            // 5 kmh    
#define SIXSTEPTHRESHOLD_DOWN 9310         // 3 kmh
#define MOTOR_ENABLE_THRESHOLD 940         // approx 30 kmh
#define MOTOR_AUTO_ENABLE_THRESHOLD 2800         // 10 kmh
#define HALL_TIMEOUT 64000                 // 0.44 kmh

#endif /* CONFIG_H_ */
