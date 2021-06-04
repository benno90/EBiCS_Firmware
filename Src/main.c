
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_hal.h"



/* USER CODE BEGIN Includes */

// Lishui BLCD FOC Open Source Firmware
// Board uses IRS2003 half bridge drivers, this need inverted pulses for low-side Mosfets, deadtime is generated in driver
// This firmware bases on the ST user manual UM1052
// It uses the OpenSTM32 workbench (SW4STM32 toolchain)
// Basic peripheral setup was generated with CubeMX

#include "print.h"
#include "FOC.h"
#include "config.h"
#include "eeprom.h"
#include "display_base.h"

#include <arm_math.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;


UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart1_rx;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/


uint32_t ui32_tim1_counter=0;
uint32_t ui32_tim3_counter=0;

uint8_t ui8_hall_state=0;
uint8_t ui8_hall_state_old=0;
uint8_t ui8_hall_case =0;
uint16_t ui16_tim2_recent=0;
uint16_t ui16_timertics = HALL_TIMEOUT; 					//timertics between two hall events for 60° interpolation
uint8_t ui8_hall_timeout_flag = 1;
uint16_t ui16_reg_adc_value; // torque
uint16_t ui16_ph1_offset=0;
uint16_t ui16_ph2_offset=0;
uint16_t ui16_ph3_offset=0;
uint16_t ui16_torque_offset = 0;
int16_t i16_ph1_current=0;

int16_t i16_ph2_current=0;
int16_t i16_ph2_current_filter=0;
int16_t i16_ph3_current=0;
uint16_t i=0;
uint16_t j=0;
uint16_t k=0;
static uint8_t ui8_slowloop_counter=0;
volatile uint8_t ui8_adc_inj_flag=0;
volatile uint8_t ui8_adc_regular_flag=0;
uint8_t ui8_speedcase=0;
uint8_t ui8_speedfactor=0;
int8_t i8_direction= REVERSE; //for permanent reverse direction
int8_t i8_reverse_flag = 1; //for temporaribly reverse direction

volatile uint8_t ui8_adc_offset_done_flag=0;
volatile uint8_t ui8_Push_Assist_flag=0;
volatile uint8_t ui8_PAS_flag=0;
volatile uint8_t ui8_external_SPEED_control_flag=0;
volatile uint8_t ui8_internal_SPEED_control_flag=0;
volatile uint8_t ui8_BC_limit_flag=0;  //flag for Battery current limitation
volatile uint8_t ui8_6step_flag=0;

q31_t q31_rotorposition_PLL = 0;
q31_t q31_pll_angle_per_tic = 0;

uint8_t ui8_UART_Counter=0;
int8_t i8_recent_rotor_direction=1;
int16_t i16_hall_order=1;

uint16_t uint16_mapped_throttle=0;
uint16_t uint16_mapped_PAS=0;
uint16_t uint16_half_rotation_counter=0;
uint16_t uint16_full_rotation_counter=0;


q31_t q31_rotorposition_absolute;
q31_t q31_rotorposition_hall;
q31_t q31_rotorposition_motor_specific = SPEC_ANGLE;
q31_t q31_u_d_temp=0;
q31_t q31_u_q_temp=0;
int16_t i16_sinus=0;
int16_t i16_cosinus=0;
char buffer[100];
char char_dyn_adc_state_old=1;
const uint8_t assist_factor[10]={0, 51, 102, 153, 204, 255, 255, 255, 255, 255};
const uint8_t assist_profile[2][6]= {	{0,10,20,30,45,48},
										{64,64,128,200,255,0}};

uint16_t switchtime[3];
volatile uint16_t adcData[8]; //Buffer for ADC1 Input
q31_t tic_array[6];


//const uint32_t ui32_wheel_speed_tics_lower_limit  = WHEEL_CIRCUMFERENCE * 5 * 3600 / (6 * GEAR_RATIO * SPEEDLIMIT * 10); //tics=wheelcirc*timerfrequency/(no. of hallevents per rev*gear-ratio*speedlimit)*3600/1000000
//const uint32_t ui32_wheel_speed_tics_higher_limit = WHEEL_CIRCUMFERENCE * 5 * 3600 / (6 * GEAR_RATIO * (SPEEDLIMIT + 3) * 10);

uint32_t uint32_tics_filtered = HALL_TIMEOUT << 3;

uint16_t VirtAddVarTab[NB_OF_VAR] = {0x01, 0x02, 0x03};


MotorState_t MS;
MotorParams_t MP;
CurrentData_t CurrentData;
BatteryVoltageData_t BatteryVoltageData;
TemperatureData_t TemperatureData;
PedalData_t PedalData;
WheelSpeedData_t WheelSpeedData;

//structs for PI_control
PI_control_t PI_iq;   // todo: rename to PI_amplitude
PI_control_t PI_id;   // todo: rename to PI_angle
PI_control_t PI_speed;
//
// PI_control static variables
static q31_t q31_i_d_cumulated = 0;             // angle control



typedef enum {HALL_STATE_SIXSTEP = 0, HALL_STATE_EXTRAPOLATION = 1, HALL_STATE_PLL = 2} hall_angle_state_t;
static hall_angle_state_t enum_hall_angle_state = HALL_STATE_SIXSTEP;
static uint8_t ui8_six_step_hall_count = 3;
static uint8_t ui8_extrapolation_hall_count = 20;

typedef enum {MOTOR_STATE_NORMAL = 0, MOTOR_STATE_BLOCKED = 1, MOTOR_STATE_PLL_ERROR = 2, MOTOR_STATE_HALL_ERROR = 3, MOTOR_STATE_DBG_ERROR = 10} motor_error_state_t;
static motor_error_state_t enum_motor_error_state = MOTOR_STATE_NORMAL;
static uint8_t ui8_motor_error_state_hall_count = 0;
static uint16_t ui16_motor_init_state_timeout = 0;

static q31_t q31_speed_pll_i = 0;
static q31_t q31_pll_abs_delta = 715827882; // +60 degree



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);


void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
                                

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

static void process_pedal_data(void);
static void process_wheel_speed_data(void);
static void process_battery_voltage(void);
static void process_chip_temperature(void);
static void fast_loop_log(void);
static void debug_comm(void);

static void dyn_adc_state(q31_t angle);
static void set_inj_channel(char state);
void get_standstill_position();
q31_t speed_PLL (q31_t ist, q31_t soll);
int32_t map (int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);
int32_t speed_to_tics (uint8_t speed);
int8_t tics_to_speed (uint32_t tics);
int16_t internal_tics_to_speedx100 (uint32_t tics);
int16_t external_tics_to_speedx100 (uint32_t tics);
int16_t q31_degree_to_degree(q31_t q31_degree);


static uint8_t ui8_pwm_enabled_flag = 0;

static void enable_pwm()
{
	uint16_half_rotation_counter=0;
	uint16_full_rotation_counter=0;
    //
    uint32_t ui32_KV = 260000;
    uint32_t u_q = ui32_KV * _T / (BatteryVoltageData.q31_battery_voltage_V_x10 * uint32_tics_filtered >> 3);
    //u_q = u_q + 50;
    //u_q = 0;

    if(u_q > 1500)
    {
        u_q = 1500;
    }

    if(u_q < 200)
    {
        u_q = 0;
    }

    MS.u_q = u_q;
    PI_iq.integral_part = (u_q << PI_iq.shift) / PI_iq.gain_i;
    MS.u_d = 0;
    PI_id.integral_part = 0;
    //
#if DISPLAY_TYPE == DISPLAY_TYPE_DEBUG
    sprintf_(buffer, "enable pwm, u_q = %lu\n", u_q);
    debug_print((uint8_t* ) buffer, strlen(buffer));
#endif

    q31_rotorposition_absolute = q31_rotorposition_hall + (DEG_plus60 >> 1);
    compute_switchtime(0, u_q, q31_rotorposition_absolute);
    //
	TIM1->CCR1 = switchtime[0]; //1023;
	TIM1->CCR2 = switchtime[1]; //1023;
	TIM1->CCR3 = switchtime[2]; //1023;
    //
    enum_hall_angle_state = HALL_STATE_SIXSTEP;
    ui8_six_step_hall_count = 3;
    ui8_pwm_enabled_flag = 1;
    //
	SET_BIT(TIM1->BDTR, TIM_BDTR_MOE);
}

static void disable_pwm()
{
   	CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE);
    //uint32_tics_filtered=1000000; // set velocity to zero
    enum_hall_angle_state = HALL_STATE_SIXSTEP;
    ui8_six_step_hall_count = 3;
    ui8_pwm_enabled_flag = 0;
}

static void trigger_motor_error(motor_error_state_t err)
{
    disable_pwm();
    enum_motor_error_state = err;
    ui8_motor_error_state_hall_count = 100;
#if DISPLAY_TYPE == DISPLAY_TYPE_DEBUG
    switch(err)
    {
        case MOTOR_STATE_BLOCKED:
            sprintf_(buffer, "\nERR: MOTOR_STATE_BLOCKED\n\n");
            break;
        case MOTOR_STATE_PLL_ERROR:
            sprintf_(buffer, "\nERR: MOTOR_STATE_PLL_ERROR\n\n");
            break;
        case MOTOR_STATE_HALL_ERROR:
            sprintf_(buffer, "\nERR: MOTOR_STATE_HALL_ERROR\n\n");
            break;
        case MOTOR_STATE_DBG_ERROR:
            sprintf_(buffer, "\nERR: MOTOR_STATE_DBG_ERROR\n\n");
            break;
        default:
            sprintf_(buffer, "\nERR: UNKNOWN\n");
            break;        
    }
    debug_print2((uint8_t *) buffer, strlen(buffer));
#endif
}


/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration----------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART1_UART_Init();

    //initialize MS struct.
    MS.hall_angle_detect_flag = 1;
    MS.ui8_assist_level = 1;
    MS.regen_level = 7;
    MP.pulses_per_revolution = PULSES_PER_REVOLUTION;
    MP.wheel_cirumference = WHEEL_CIRCUMFERENCE;
    MP.speedLimit = SPEEDLIMIT;

    MS.ui8_dbg_log_value = 0;
    MS.ui16_dbg_value2 = 0;
    MS.ui16_dbg_value = 0;
    MS.ui8_go = 0;
    MS.ui8_log = 1;

    //
    BatteryVoltageData.ui8_shift = 5;
    TemperatureData.ui8_shift = 5;
    CurrentData.ui8_battery_current_shift = 3;
    CurrentData.q31_battery_current_mA = 0;
    CurrentData.q31_battery_current_mA_cumulated = 0;

    //
    PedalData.uint8_pas_shift = 2;
    PedalData.uint32_PAS_counter = PAS_TIMEOUT;
    PedalData.uint32_PAS = PAS_TIMEOUT;
    PedalData.uint32_PAS_raw = PAS_TIMEOUT;
    PedalData.uint32_PAS_cumulated = PAS_TIMEOUT << PedalData.uint8_pas_shift;
    PedalData.uint32_PAS_HIGH_counter = 0;
    PedalData.uint32_PAS_HIGH_accumulated = 32000;
    PedalData.uint32_PAS_fraction = 100;
    //
    PedalData.uint8_torque_shift = 4;
    PedalData.uint32_torque_adc_cumulated = 0;
    PedalData.uint32_torque_Nm_x10 = 0;

    //
    WheelSpeedData.uint32_external_SPEED_counter = 32000;
    WheelSpeedData.uint32_SPEEDx100_kmh_cumulated = 0;
    WheelSpeedData.ui8_speed_shift = 1;


//init PI structs
#define SHIFT_ID 0
    PI_id.gain_i = I_FACTOR_I_D;
    PI_id.gain_p = P_FACTOR_I_D;
    PI_id.setpoint = 0;
    PI_id.limit_output_max_shifted = Q31_DEGREE * 20 << SHIFT_ID;
    PI_id.limit_output_min_shifted = -Q31_DEGREE * 20 << SHIFT_ID;
    PI_id.max_step_shifted = Q31_DEGREE << SHIFT_ID; // shifted value
    PI_id.shift = SHIFT_ID;

#define SHIFT_IQ 10
    PI_iq.gain_i = I_FACTOR_I_Q;
    PI_iq.gain_p = P_FACTOR_I_Q;
    PI_iq.setpoint = 0;
    PI_iq.limit_output_max_shifted = _U_MAX << SHIFT_IQ;
    PI_iq.limit_output_min_shifted = 0 << SHIFT_IQ; // currently no regeneration
    PI_iq.max_step_shifted = 4 << SHIFT_IQ;         // shifted value
    PI_iq.shift = SHIFT_IQ;

#ifdef SPEEDTHROTTLE

    PI_speed.gain_i = I_FACTOR_SPEED;
    PI_speed.gain_p = P_FACTOR_SPEED;
    PI_speed.setpoint = 0;
    PI_speed.limit_output = PH_CURRENT_MAX;
    PI_speed.max_step = 5000;
    PI_speed.shift = 12;
    PI_speed.limit_i = PH_CURRENT_MAX;

#endif

    //Virtual EEPROM init
    HAL_FLASH_Unlock();
    EE_Init();
    HAL_FLASH_Lock();

    MX_ADC1_Init();
    /* Run the ADC calibration */
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        /* Calibration Error */
        Error_Handler();
    }
    MX_ADC2_Init();
    /* Run the ADC calibration */
    if (HAL_ADCEx_Calibration_Start(&hadc2) != HAL_OK)
    {
        /* Calibration Error */
        Error_Handler();
    }

    /* USER CODE BEGIN 2 */
    SET_BIT(ADC1->CR2, ADC_CR2_JEXTTRIG); //external trigger enable
    __HAL_ADC_ENABLE_IT(&hadc1, ADC_IT_JEOC);
    SET_BIT(ADC2->CR2, ADC_CR2_JEXTTRIG); //external trigger enable
    __HAL_ADC_ENABLE_IT(&hadc2, ADC_IT_JEOC);

    //HAL_ADC_Start_IT(&hadc1);
    HAL_ADCEx_MultiModeStart_DMA(&hadc1, (uint32_t *)adcData, 7);
    HAL_ADC_Start_IT(&hadc2);
    MX_TIM1_Init(); //Hier die Reihenfolge getauscht!
    MX_TIM2_Init();
    MX_TIM3_Init();

    // Start Timer 1
    if (HAL_TIM_Base_Start_IT(&htim1) != HAL_OK)
    {
        /* Counter Enable Error */
        Error_Handler();
    }

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1); // turn on complementary channel
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);

    HAL_TIM_PWM_Start_IT(&htim1, TIM_CHANNEL_4);

    TIM1->CCR4 = TRIGGER_DEFAULT; //ADC sampling just before timer overflow (just before middle of PWM-Cycle)
                                  //PWM Mode 1: Interrupt at counting down.

    // TIM1->BDTR |= 1L<<15;
    // TIM1->BDTR &= ~(1L<<15); //reset MOE (Main Output Enable) bit to disable PWM output
    // Start Timer 2
    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
    {
        /* Counter Enable Error */
        Error_Handler();
    }

    // Start Timer 3

    if (HAL_TIM_Base_Start_IT(&htim3) != HAL_OK)
    {
        /* Counter Enable Error */
        Error_Handler();
    }

    Display_Init(&MS);

    TIM1->CCR1 = 1023; //set initial PWM values
    TIM1->CCR2 = 1023;
    TIM1->CCR3 = 1023;

    CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE); //Disable PWM

    HAL_Delay(200); //wait for stable conditions

    BatteryVoltageData.q31_battery_voltage_adc_cumulated = 0;
    TemperatureData.q31_temperature_adc_cumulated = 0;

    for (i = 0; i < 32; i++)
    {
        while (!ui8_adc_regular_flag){}
        ui16_ph1_offset += adcData[2];
        ui16_ph2_offset += adcData[3];
        ui16_ph3_offset += adcData[4];
        ui16_torque_offset += adcData[TQ_ADC_INDEX];
        BatteryVoltageData.q31_battery_voltage_adc_cumulated += adcData[0];
        TemperatureData.q31_temperature_adc_cumulated += adcData[TEMP_ADC_INDEX];
        ui8_adc_regular_flag = 0;
    }
    ui16_ph1_offset = ui16_ph1_offset >> 5;
    ui16_ph2_offset = ui16_ph2_offset >> 5;
    ui16_ph3_offset = ui16_ph3_offset >> 5;
    ui16_torque_offset = ui16_torque_offset >> 5;
    ui16_torque_offset += 30; // hardcoded offset -> move to config.h

    BatteryVoltageData.q31_battery_voltage_adc_cumulated =
        (BatteryVoltageData.q31_battery_voltage_adc_cumulated >> 5) << BatteryVoltageData.ui8_shift;

    BatteryVoltageData.q31_battery_voltage_V_x10 =
        (BatteryVoltageData.q31_battery_voltage_adc_cumulated * CAL_BAT_V / 100) >> BatteryVoltageData.ui8_shift;

    TemperatureData.q31_temperature_adc_cumulated =
        (TemperatureData.q31_temperature_adc_cumulated >> 5) << TemperatureData.ui8_shift;
    TemperatureData.q31_temperature_degrees = 0; // setting it in the slow loop

// comment hochsitzcola 20.05.21
#ifdef DISABLE_DYNAMIC_ADC               // set  injected channel with offsets
    ADC1->JSQR = 0b00100000000000000000; //ADC1 injected reads phase A JL = 0b00, JSQ4 = 0b00100 (decimal 4 = channel 4)
    ADC1->JOFR1 = ui16_ph1_offset;
    ADC2->JSQR = 0b00101000000000000000; //ADC2 injected reads phase B, JSQ4 = 0b00101, decimal 5
    ADC2->JOFR1 = ui16_ph2_offset;
#endif

    ui8_adc_offset_done_flag = 1;

#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)
    printf_("phase current offsets:  %d, %d, %d \n ", ui16_ph1_offset, ui16_ph2_offset, ui16_ph3_offset);
    printf_("torque offset:  %d\n", ui16_torque_offset);
#if (AUTODETECT == 1)
    autodetect();
    while (1) { };
#endif

#endif
#if (DISPLAY_TYPE != DISPLAY_TYPE_DEBUG || !AUTODETECT)
    EE_ReadVariable(EEPROM_POS_SPEC_ANGLE, &MP.spec_angle);

    // set motor specific angle to value from emulated EEPROM only if valid
    if (MP.spec_angle != 0xFFFF)
    {
        q31_rotorposition_motor_specific = MP.spec_angle << 16;
        EE_ReadVariable(EEPROM_POS_HALL_ORDER, &i16_hall_order);
    }
#endif

    //q31_rotorposition_motor_specific = -167026406;
    q31_rotorposition_motor_specific = -1789569706; // -150 degrees
    //q31_rotorposition_motor_specific = -1801499903;  // -152 degrees
    //q31_rotorposition_motor_specific = 0;
    i16_hall_order = 1;
    // set absolute position to corresponding hall pattern.

#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)
    printf_("Lishui FOC v0.9 \n ");
    printf_("Motor specific angle:  %d, direction %d \n ", q31_rotorposition_motor_specific, i16_hall_order);
#endif

    CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE); //Disable PWM

    get_standstill_position();

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {

#if DISPLAY_TYPE == DISPLAY_TYPE_DEBUG
        if (MS.ui16_dbg_value2)
        {
            trigger_motor_error(MOTOR_STATE_DBG_ERROR);
            MS.ui16_dbg_value2 = 0;
        }
#endif

        // --------------------------------------------------
        // PI control
        //

        if (PI_flag || !ui8_pwm_enabled_flag)
        {
            // only pi control can enable the pwm
            runPIcontrol();
        }

        // --------------------------------------------------
        // UART Comm
        //

        //display message processing
        if (ui8_g_UART_Rx_flag)
        {
            MS.q31_battery_current_mA = CurrentData.q31_battery_current_mA;
            MS.error_state = enum_motor_error_state; // todo -> move enum_motor_error_state into MS

            Display_Service(&MS);

            if (MS.ui8_lights)
                HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_SET); // LED_Pin?
            else
                HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_RESET);

            ui8_g_UART_Rx_flag = 0;
        }

        // --------------------------------------------------
        process_pedal_data();

        // --------------------------------------------------
        process_wheel_speed_data();

        // --------------------------------------------------
        fast_loop_log();

        //----------------------------------------------------------------------------------------------------------------------------------------------------------
        //slow loop procedere @16Hz, for LEV standard every 4th loop run, send page,
        if (ui32_tim3_counter > 500)
        {

            // --------------------------------------------------
            debug_comm();

            if (ui16_motor_init_state_timeout > 0)
            {
                --ui16_motor_init_state_timeout;
            }

            if (ui8_slowloop_counter == 0)
            {
                // 1Hz process frequency for battery voltage and chip temperature

                // --------------------------------------------------
                process_battery_voltage();
                process_chip_temperature();

                ui8_slowloop_counter = 16;
            }
            else
            {
                --ui8_slowloop_counter;
            }

            ui32_tim3_counter = 0;
        }

        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */

} // end main while loop

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}


/* ADC1 init function */
static void MX_ADC1_Init(void)
{

  ADC_MultiModeTypeDef multimode;
  ADC_InjectionConfTypeDef sConfigInjected;
  ADC_ChannelConfTypeDef sConfig;

  // enable temperature sensor -> seems not to be necessary
  //ADC1->CR2 |= ADC_CR2_TSVREFE;

    /**Common config 
    */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE; //Scan muß für getriggerte Wandlung gesetzt sein
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;// Trigger regular ADC with timer 3 ADC_EXTERNALTRIGCONV_T1_CC1;// // ADC_SOFTWARE_START; //
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 7;
  hadc1.Init.NbrOfDiscConversion = 0;


  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the ADC multi-mode 
    */
  multimode.Mode = ADC_DUALMODE_REGSIMULT_INJECSIMULT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure Injected Channel 
    */
  sConfigInjected.InjectedChannel = ADC_CHANNEL_4;
  sConfigInjected.InjectedRank = ADC_INJECTED_RANK_1;
  sConfigInjected.InjectedNbrOfConversion = 1;
  sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfigInjected.ExternalTrigInjecConv = ADC_EXTERNALTRIGINJECCONV_T1_CC4; // Hier bin ich nicht sicher ob Trigger out oder direkt CC4
  sConfigInjected.AutoInjectedConv = DISABLE; //muß aus sein
  sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
  sConfigInjected.InjectedOffset = ui16_ph1_offset;//1900;
  HAL_ADC_Stop(&hadc1); //ADC muß gestoppt sein, damit Triggerquelle gesetzt werden kann.
  if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  /**Configure Regular Channel
  */
sConfig.Channel = ADC_CHANNEL_7; //battery voltage
sConfig.Rank = ADC_REGULAR_RANK_1;
sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5 ;//ADC_SAMPLETIME_1CYCLE_5
if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
{
  _Error_Handler(__FILE__, __LINE__);
}


/**Configure Regular Channel
*/
sConfig.Channel = ADC_CHANNEL_3; //Connector SP: throttle input
sConfig.Rank = ADC_REGULAR_RANK_2;
sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;//ADC_SAMPLETIME_239CYCLES_5;
if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
{
_Error_Handler(__FILE__, __LINE__);
}
/**Configure Regular Channel
*/
sConfig.Channel = ADC_CHANNEL_4; //Phase current 1
sConfig.Rank = ADC_REGULAR_RANK_3;
sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;//ADC_SAMPLETIME_239CYCLES_5;
if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
{
_Error_Handler(__FILE__, __LINE__);
}
/**Configure Regular Channel
*/
sConfig.Channel = ADC_CHANNEL_5; //Phase current 2
sConfig.Rank = ADC_REGULAR_RANK_4;
sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;//ADC_SAMPLETIME_239CYCLES_5;
if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
{
_Error_Handler(__FILE__, __LINE__);
}
/**Configure Regular Channel
*/
sConfig.Channel = ADC_CHANNEL_6; //Phase current 3
sConfig.Rank = ADC_REGULAR_RANK_5;
sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;//ADC_SAMPLETIME_239CYCLES_5;
if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
{
_Error_Handler(__FILE__, __LINE__);
}

/**Configure Regular Channel
*/
sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;     //ADC_CHANNEL_8; // connector AD2
sConfig.Rank = ADC_REGULAR_RANK_6;
sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
{
_Error_Handler(__FILE__, __LINE__);
}

/**Configure Regular Channel
*/
sConfig.Channel = ADC_CHANNEL_9; // connector AD1, temperature or torque input for Controller from PhoebeLiu @ aliexpress
sConfig.Rank = ADC_REGULAR_RANK_7;
sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;//ADC_SAMPLETIME_239CYCLES_5;
if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
{
_Error_Handler(__FILE__, __LINE__);
}

}

/* ADC2 init function */
static void MX_ADC2_Init(void)
{

  ADC_InjectionConfTypeDef sConfigInjected;

    /**Common config 
    */
  hadc2.Instance = ADC2;
  hadc2.Init.ScanConvMode = ADC_SCAN_ENABLE; //hier auch Scan enable?!
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure Injected Channel 
    */
  sConfigInjected.InjectedChannel = ADC_CHANNEL_5;
  sConfigInjected.InjectedRank = ADC_INJECTED_RANK_1;
  sConfigInjected.InjectedNbrOfConversion = 1;
  sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfigInjected.ExternalTrigInjecConv = ADC_INJECTED_SOFTWARE_START;
  sConfigInjected.AutoInjectedConv = DISABLE;
  sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
  sConfigInjected.InjectedOffset = ui16_ph2_offset;//	1860;
  if (HAL_ADCEx_InjectedConfigChannel(&hadc2, &sConfigInjected) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}
/* TIM1 init function */
static void MX_TIM1_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;
  TIM_OC_InitTypeDef sConfigOC;
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig;

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED1;
  htim1.Init.Period = _T;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  if (HAL_TIM_OC_Init(&htim1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC4REF;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_LOW;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_SET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  //sConfigOC.OCMode = TIM_OCMODE_ACTIVE; // war hier ein Bock?!
  sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
  if (HAL_TIM_OC_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = PWM_DEAD_TIME;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  HAL_TIM_MspPostInit(&htim1);

}

/* TIM2 init function */
static void MX_TIM2_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 128;                       // counter frequency = 64MHz / 128 = 500 kHz
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = HALL_TIMEOUT;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM3 init function 8kHz interrupt frequency for regular adc triggering */
static void MX_TIM3_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 7813;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC1;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  HAL_TIM_MspPostInit(&htim3);

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = DISPLAY_UART_BAUDRATE;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();



  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  // benno 27.05.21 - disabled rx cplt interrupt
  // HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  // HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : Hall_1_Pin Hall_2_Pin Hall_3_Pin */
  GPIO_InitStruct.Pin = Hall_1_Pin|Hall_2_Pin|Hall_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LIGHT_Pin */
  GPIO_InitStruct.Pin = LIGHT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(LIGHT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Brake_Pin */
  GPIO_InitStruct.Pin = Brake_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Brake_GPIO_Port, &GPIO_InitStruct);


  /*Configure GPIO pins : Speed_EXTI5_Pin PAS_EXTI8_Pin */
  GPIO_InitStruct.Pin = Speed_EXTI5_Pin|PAS_EXTI8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);


  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

}

/* USER CODE BEGIN 4 */

static void process_pedal_data(void)
{
    if (ui8_PAS_flag)
    {
        if (PedalData.uint32_PAS_counter > 100)  // debounce
        {
            PedalData.uint32_PAS_cumulated -= PedalData.uint32_PAS_cumulated >> PedalData.uint8_pas_shift;
            PedalData.uint32_PAS_cumulated += PedalData.uint32_PAS_counter;
            PedalData.uint32_PAS = PedalData.uint32_PAS_cumulated >> PedalData.uint8_pas_shift;
            PedalData.uint32_PAS_raw = PedalData.uint32_PAS_counter;

            PedalData.uint32_PAS_HIGH_accumulated -= PedalData.uint32_PAS_HIGH_accumulated >> PedalData.uint8_pas_shift;
            PedalData.uint32_PAS_HIGH_accumulated += PedalData.uint32_PAS_HIGH_counter;

            PedalData.uint32_PAS_fraction = (PedalData.uint32_PAS_HIGH_accumulated >> PedalData.uint8_pas_shift) * 100 / PedalData.uint32_PAS;
            PedalData.uint32_PAS_HIGH_counter = 0;
            PedalData.uint32_PAS_counter = 0;
            ui8_PAS_flag = 0;


            uint32_t ui32_reg_adc_value_shifted = ui16_reg_adc_value << PedalData.uint8_torque_shift;

            if (ui32_reg_adc_value_shifted > PedalData.uint32_torque_adc_cumulated)
            {
                // accept rising values unfiltered
                PedalData.uint32_torque_adc_cumulated = ui32_reg_adc_value_shifted;
            }
            else
            {
                // filter falling values
                PedalData.uint32_torque_adc_cumulated -= PedalData.uint32_torque_adc_cumulated >> PedalData.uint8_torque_shift;
                PedalData.uint32_torque_adc_cumulated += ui16_reg_adc_value;
            }

            PedalData.uint32_torque_Nm_x10 = 10 * (PedalData.uint32_torque_adc_cumulated >> PedalData.uint8_torque_shift) / CAL_TORQUE;
        }
    }

    if (PedalData.uint32_PAS_counter >= PAS_TIMEOUT)
    {
        PedalData.uint32_PAS = PAS_TIMEOUT;
        PedalData.uint32_PAS_raw = PAS_TIMEOUT;
        PedalData.uint32_PAS_cumulated = PAS_TIMEOUT << PedalData.uint8_pas_shift;
        PedalData.uint32_torque_adc_cumulated = 0;
        PedalData.uint32_torque_Nm_x10 = 0;
    }

    if (PedalData.uint32_PAS_raw >= PAS_TIMEOUT)
    {
        PedalData.uint8_pedaling = 0;
    }
    else
    {
        PedalData.uint8_pedaling = 1;
    }
}

static void process_wheel_speed_data(void)
{
#if (SPEEDSOURCE == EXTERNAL)
    if (ui8_external_SPEED_control_flag)
    {

        if (WheelSpeedData.uint32_external_SPEED_counter > 200)
        { //debounce
            WheelSpeedData.uint32_SPEEDx100_kmh_cumulated -= WheelSpeedData.uint32_SPEEDx100_kmh_cumulated >> WheelSpeedData.ui8_speed_shift;
            WheelSpeedData.uint32_SPEEDx100_kmh_cumulated += internal_tics_to_speedx100(WheelSpeedData.uint32_external_SPEED_COUNTER);

            MS.ui16_wheel_time_ms = WheelSpeedData.uint32_external_SPEED_counter * PULSES_PER_REVOLUTION / 8
            
            WheelSpeedData.uint32_external_SPEED_counter = 0;
            ui8_external_SPEED_control_flag = 0;
        }
    }
            
    if (WheelSpeedData.uint32_external_SPEED_counter > 127999)
    {
        WheelSpeedData.uint32_SPEEDx100_kmh_cumulated = 0;
    }

#elif (SPEEDSOURCE == INTERNAL)

    if (ui8_internal_SPEED_control_flag)
    {
        WheelSpeedData.uint32_SPEEDx100_kmh_cumulated -= WheelSpeedData.uint32_SPEEDx100_kmh_cumulated >> WheelSpeedData.ui8_speed_shift;
        WheelSpeedData.uint32_SPEEDx100_kmh_cumulated += internal_tics_to_speedx100(uint32_tics_filtered >> 3);
        ui8_internal_SPEED_control_flag = 0;
		  
        // period [s] = tics x 6 x GEAR_RATIO / frequency    (frequency = 500kHz)
        MS.ui16_wheel_time_ms = (uint32_tics_filtered>>3) * 6 * GEAR_RATIO / (500);
    }

#endif
}

static void process_battery_voltage(void)
{
    BatteryVoltageData.q31_battery_voltage_adc_cumulated -= (BatteryVoltageData.q31_battery_voltage_adc_cumulated >> BatteryVoltageData.ui8_shift);
    BatteryVoltageData.q31_battery_voltage_adc_cumulated += adcData[0];
    BatteryVoltageData.q31_battery_voltage_V_x10 =
        (BatteryVoltageData.q31_battery_voltage_adc_cumulated * CAL_BAT_V / 100) >> BatteryVoltageData.ui8_shift;
}

static void process_chip_temperature(void)
{
    TemperatureData.q31_temperature_adc_cumulated -= (TemperatureData.q31_temperature_adc_cumulated >> TemperatureData.ui8_shift);
    TemperatureData.q31_temperature_adc_cumulated += adcData[TEMP_ADC_INDEX];

    // data sheet STM32F103x4
    // 5.3.19 Temperature sensor characteristics
    // avg voltage at 25 degrees 1.43 V
    // avg solpe 4.3 mV / degree

    // recover voltage (3.3V  ref voltage, 2^12 = 4096)
    // int32_t v_temp = adcData[TEMP_ADC_INDEX] * 3300 >> 12; // voltage in mV
    int32_t v_temp = (TemperatureData.q31_temperature_adc_cumulated >> TemperatureData.ui8_shift) * 3300 >> 12; // voltage in mV
    //
    v_temp = (1430 - v_temp) * 10 / 43 + 25;
    if (v_temp > 0)
    {
        TemperatureData.q31_temperature_degrees = v_temp;
    }
    else
    {
        TemperatureData.q31_temperature_degrees = 0;
    }

}

static void fast_loop_log(void)
{
#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG && defined(FAST_LOOP_LOG))
    if (ui8_fast_loop_log_state == 2 && ui8_g_UART_TxCplt_flag)
    {
        if (k < FAST_LOOP_LOG_SIZE)
        {
            sprintf_(buffer, "%d, %d\n", e_log[k][0], e_log[k][1]);
            i = 0;
            while (buffer[i] != '\0')
            {
                i++;
            }
            ui8_g_UART_TxCplt_flag = 0;
            HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&buffer, i);
            k++;
        }
        //
        if (k >= FAST_LOOP_LOG_SIZE)
        {
            k = 0;
            ui8_fast_loop_log_state = 0;
        }
    }
#endif
}

static void debug_comm(void)
{
//#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG && !defined(FAST_LOOP_LOG))
#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)

    static uint8_t ui8_dbg_print_counter = 0;

    //print values for debugging

    if (ui8_dbg_print_counter == 0)
    {

        switch (MS.ui8_dbg_log_value)
        {
        case 0:
#ifndef BLUETOOTH_SERIALIZE_DISPLAY
            // plot anything here
            //sprintf_(buffer, "%u %u T: %u  U: %u\n", enum_motor_error_state, ui8_six_step_hall_count, (uint16_t)TemperatureData.q31_temperature_degrees, (uint16_t)(BatteryVoltageData.q31_battery_voltage_V_x10 / 10));
            //uint32_t ui32_KV = ((ui16_timertics * BatteryVoltageData.q31_battery_voltage_V_x10) >> 11) * MS.u_q;
            //sprintf_(buffer, "kv: %lu\n", ui32_KV);
            //
            //uint32_t ui32_KV = 260000;
            //uint32_t u_q = ui32_KV * _T / (BatteryVoltageData.q31_battery_voltage_V_x10 * ui16_timertics);
            //sprintf_(buffer, "%lu %lu | %u %lu\n", u_q, MS.u_q, ui16_timertics, uint32_tics_filtered >> 3);
            //
            //sprintf_(buffer, "%u | %u | %u %u\n", ui16_timertics, enum_motor_error_state, enum_hall_angle_state, ui8_motor_error_state_hall_count);
            sprintf_(buffer, "%u %lu %lu\n", ui16_reg_adc_value, PedalData.uint32_torque_Nm_x10, PedalData.uint32_PAS);
//
#endif
            break;
        case 1:
        {
            uint16_t velocity_kmh = 2234 * 50 * 36 / (6 * GEAR_RATIO * (uint32_tics_filtered >> 3));
#ifdef BLUETOOTH_SERIALIZE_DISPLAY
            sprintf_(buffer, "Graph:%u$", velocity_kmh);
#else
            sprintf_(buffer, "v_kmh: %u\n", velocity_kmh);
#endif
            break;
        }
        case 2:
#ifdef BLUETOOTH_SERIALIZE_DISPLAY
            sprintf_(buffer, "Graph:%u|%u$", (uint16_t)TemperatureData.q31_temperature_degrees, (uint16_t)(BatteryVoltageData.q31_battery_voltage_V_x10 / 10));
#else
            sprintf_(buffer, "T: %u | U: %u\n", (uint16_t)TemperatureData.q31_temperature_degrees, (uint16_t)(BatteryVoltageData.q31_battery_voltage_V_x10 / 10));
#endif
            break;
        case 3:
#ifdef BLUETOOTH_SERIALIZE_DISPLAY
            sprintf_(buffer, "Graph:%ld$", MS.u_q);
#else
            sprintf_(buffer, "u_q: %ld\n", MS.u_q);
#endif
            break;
        case 4:
#ifdef BLUETOOTH_SERIALIZE_DISPLAY
            sprintf_(buffer, "Graph:%d$", q31_degree_to_degree(MS.foc_alpha));
#else
            sprintf_(buffer, "foc_alpha: %d\n", q31_degree_to_degree(MS.foc_alpha));
#endif
            break;
        case 5:
        {
            //uint32_t phase_current_x10 = CurrentData.q31_battery_current_mA / 100;
            //phase_current_x10 = phase_current_x10 * 4 * _T / (3 * MS.u_q);
            q31_t phase_current_x10 = (CAL_I * MS.i_q) / (32 * 100);
#ifdef BLUETOOTH_SERIALIZE_DISPLAY
            sprintf_(buffer, "Graph:%ld|%ld$", CurrentData.q31_battery_current_mA / 100, phase_current_x10);
#else
            sprintf_(buffer, "cur: %ld | ph_cur: %ld (A x10)\n", CurrentData.q31_battery_current_mA / 100, phase_current_x10);
#endif
            break;
        }
        case 6:
#ifdef BLUETOOTH_SERIALIZE_DISPLAY
            sprintf_(buffer, "Graph:%ld|%ld$", MS.i_q, MS.i_d);
#else
            sprintf_(buffer, "i_q: %ld | i_d: %ld\n", MS.i_q, MS.i_d);
#endif
            break;
        default:

            sprintf_(buffer, ".");
            break;
        }

        debug_print((uint8_t *)buffer, strnlen(buffer, 100));

#ifdef BLUETOOTH_SERIALIZE_DISPLAY
        ui8_dbg_print_counter = 0;
#else
        ui8_dbg_print_counter = 16;
#endif
    }
    else
    {
        --ui8_dbg_print_counter;
    }

#endif
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim == &htim3) 
	{
		if(ui32_tim3_counter < 32000) ui32_tim3_counter++;
		//
		if(PedalData.uint32_PAS_counter < PAS_TIMEOUT)
		{
			PedalData.uint32_PAS_counter++;
			if(HAL_GPIO_ReadPin(PAS_GPIO_Port, PAS_Pin)) 
            {
                PedalData.uint32_PAS_HIGH_counter++;
            }
		}
		//
		if (WheelSpeedData.uint32_external_SPEED_counter < 128000)
        {
            WheelSpeedData.uint32_external_SPEED_counter++;
        }
		if(uint16_full_rotation_counter<8000)uint16_full_rotation_counter++;	//full rotation counter for motor standstill detection
		if(uint16_half_rotation_counter<8000)uint16_half_rotation_counter++;	//half rotation counter for motor standstill detection

	}

    if(htim == &htim2)
    {
        // HALL_TIMEOUT
        ui8_hall_timeout_flag = 1;
        ui16_timertics = HALL_TIMEOUT;
        uint32_tics_filtered = HALL_TIMEOUT << 3;
    }
}



// regular ADC callback
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    static uint32_t ui32_reg_adc_value_filter = 0;
#ifdef TS_MODE

	uint16_t torque_adc = adcData[TQ_ADC_INDEX];

	if(torque_adc > ui16_torque_offset)
	{
		torque_adc = adcData[TQ_ADC_INDEX] - ui16_torque_offset;
		ui32_reg_adc_value_filter -= ui32_reg_adc_value_filter >> 5;
	
		ui32_reg_adc_value_filter += torque_adc;
	
		ui16_reg_adc_value = ui32_reg_adc_value_filter >> 5;
	}
	else
	{
		ui32_reg_adc_value_filter = 0;
		ui16_reg_adc_value = 0;
	}
		
		
#else

    // todo: program whatever you need here for throttle mode

    ui16_reg_adc_value = 0; 
    ui32_reg_adc_value_filter = 0;

#endif
	
    ui8_adc_regular_flag=1;

}

//injected ADC

void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	//for oszi-check of used time in FOC procedere
	//HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	  ui32_tim1_counter++;

	if(!ui8_adc_offset_done_flag)
	{
	i16_ph1_current = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
	i16_ph2_current = HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_1);

	ui8_adc_inj_flag=1;
	}
	else{

#ifdef DISABLE_DYNAMIC_ADC

		i16_ph1_current = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
		i16_ph2_current = HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_1);


#else
	switch (MS.char_dyn_adc_state) //read in according to state
		{
		case 1: //Phase C at high dutycycles, read from A+B directly
			{
				temp1=(q31_t)HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
				i16_ph1_current = temp1 ;

				temp2=(q31_t)HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_1);
				i16_ph2_current = temp2;
			}
			break;
		case 2: //Phase A at high dutycycles, read from B+C (A = -B -C)
			{

				temp2=(q31_t)HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_1);
				i16_ph2_current = temp2;

				temp1=(q31_t)HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
				i16_ph1_current = -i16_ph2_current-temp1;

			}
			break;
		case 3: //Phase B at high dutycycles, read from A+C (B=-A-C)
			{
				temp1=(q31_t)HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
				i16_ph1_current = temp1 ;
				temp2=(q31_t)HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_1);
				i16_ph2_current = -i16_ph1_current-temp2;
			}
			break;

		case 0: //timeslot too small for ADC
			{
				//do nothing
			}
			break;




		} // end case
#endif




	//extrapolate recent rotor position
	ui16_tim2_recent = __HAL_TIM_GET_COUNTER(&htim2); // read in timertics since last event
                

    if(MS.hall_angle_detect_flag)
    {   
        //autodetect is not active
    
        // Motor Blocked error
        //if(ui16_tim2_recent > ui16_timertics + (ui16_timertics >> 2))  // ui16_timertics * 5/4 was too sensitive
        if(ui16_tim2_recent > (ui16_timertics * 2) )       
        {
            trigger_motor_error(MOTOR_STATE_BLOCKED);
        }

        switch(enum_hall_angle_state)
        {
            case HALL_STATE_SIXSTEP:
                q31_rotorposition_absolute = q31_rotorposition_hall + (DEG_plus60 >> 1);
                q31_rotorposition_PLL = q31_rotorposition_hall;

                // set speed pll integral part
                q31_speed_pll_i = DEG_plus60 / ui16_timertics;
                q31_pll_angle_per_tic = q31_speed_pll_i;

                ui8_extrapolation_hall_count = 20;
	        
                if(ui16_timertics < SIXSTEPTHRESHOLD_UP && ui8_six_step_hall_count == 0)
                {
                    enum_hall_angle_state = HALL_STATE_EXTRAPOLATION;
                }
                break;

            case HALL_STATE_EXTRAPOLATION:
                
                // extrapolation method
                // interpolate angle between two hallevents by scaling timer2 tics, 10923<<16 is 715827883 = 60°
                q31_rotorposition_absolute = q31_rotorposition_hall + (q31_t)(i16_hall_order * i8_recent_rotor_direction * ((10923 * ui16_tim2_recent) / ui16_timertics) << 16);
                // let the pll run in parallel (einschwingen)
                q31_rotorposition_PLL += q31_pll_angle_per_tic;

                if(ui16_timertics > SIXSTEPTHRESHOLD_DOWN)
                {
                    ui8_six_step_hall_count = 3;
                    enum_hall_angle_state = HALL_STATE_SIXSTEP;
                }

                if(ui8_extrapolation_hall_count == 0)
                {
                    if(q31_pll_abs_delta < Q31_DEGREE * 10)
                    {
                        enum_hall_angle_state = HALL_STATE_PLL;
                    }
                    else
                    {
                        ui8_extrapolation_hall_count = 30;
                    }
                }

                break;

            case HALL_STATE_PLL:
                // PLL
                q31_rotorposition_PLL += q31_pll_angle_per_tic;
                q31_rotorposition_absolute = q31_rotorposition_PLL;
	            
                
                if(ui16_timertics > SIXSTEPTHRESHOLD_DOWN)
                {
                    ui8_six_step_hall_count = 3;
                    enum_hall_angle_state = HALL_STATE_SIXSTEP;
                }
                break;

            default:
                break;
        }
			
            

    } //end hall processing

#ifndef DISABLE_DYNAMIC_ADC

	//get the Phase with highest duty cycle for dynamic phase current reading
	dyn_adc_state(q31_rotorposition_absolute);
	//set the according injected channels to read current at Low-Side active time

	if (MS.char_dyn_adc_state!=char_dyn_adc_state_old)
    {
	    set_inj_channel(MS.char_dyn_adc_state);
        char_dyn_adc_state_old = MS.char_dyn_adc_state;
	}
#endif

	//int16_current_target=0;
	// call FOC procedure if PWM is enabled

    // always do the foc calculation
	//if (READ_BIT(TIM1->BDTR, TIM_BDTR_MOE))
    {
	    FOC_calculation(i16_ph1_current, i16_ph2_current, q31_rotorposition_absolute, &MS);
	}
	//temp5=__HAL_TIM_GET_COUNTER(&htim1);
	//set PWM

    // only update switchtimes if pwm is enabled
    if(ui8_pwm_enabled_flag)
    {
	    TIM1->CCR1 =  (uint16_t) switchtime[0];
	    TIM1->CCR2 =  (uint16_t) switchtime[1];
	    TIM1->CCR3 =  (uint16_t) switchtime[2];
    }



	//HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

	} // end else

}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint8_t ui8_hall_error_count = 0;
    //                                                      1  2  3  4  5  6
    const uint8_t next_expected_forward_hall_state[7] = {0, 3, 6, 2, 5, 1, 4};
	//Hall sensor event processing
	if(GPIO_Pin == GPIO_PIN_0 || GPIO_Pin == GPIO_PIN_1 || GPIO_Pin == GPIO_PIN_2) //check for right interrupt source
	{

    	ui16_tim2_recent = __HAL_TIM_GET_COUNTER(&htim2); // read in timertics since last hall event

        // 250 tics -> 110 kmh
        if(ui16_tim2_recent > 250) // debounce -> should affect the entire hall processing, not only part of it!
        {
       	    ui8_hall_state = GPIOA->IDR & 0b111; //Mask input register with Hall 1 - 3 bits

            if(ui8_hall_state == next_expected_forward_hall_state[ui8_hall_state_old])
            {

            	ui8_hall_case=ui8_hall_state_old*10+ui8_hall_state;
            	if(MS.hall_angle_detect_flag)
                { 
                    //only process, if autodetect procedere is fininshed
            	    ui8_hall_state_old=ui8_hall_state;
            	}

                if(ui8_hall_timeout_flag)
                {
                    // no hall event for a long time @ see HAL_TIM_PeriodElapsedCallback
                    // setting it to tim2_recent is unreliable
                    ui8_hall_timeout_flag = 0;
            	    __HAL_TIM_SET_COUNTER(&htim2, 0); //reset tim2 counter
                }
            	else
                {
            		ui16_timertics = ui16_tim2_recent; //save timertics since last hall event
            		uint32_tics_filtered-=uint32_tics_filtered>>3;
            		uint32_tics_filtered+=ui16_timertics;
            	    __HAL_TIM_SET_COUNTER(&htim2,0); //reset tim2 counter
            	    ui8_internal_SPEED_control_flag=1;

                    if(ui8_extrapolation_hall_count > 0) --ui8_extrapolation_hall_count;
                    if(ui8_six_step_hall_count > 0) -- ui8_six_step_hall_count;

                    // clear motor error after a certain amount of valid hall transitions
                    if(ui8_motor_error_state_hall_count > 0) -- ui8_motor_error_state_hall_count;
                    if(ui8_motor_error_state_hall_count == 0) enum_motor_error_state = MOTOR_STATE_NORMAL;
            	}

            	switch (ui8_hall_case) //12 cases for each transition from one stage to the next. 6x forward, 6x reverse
        		{
        		//6 cases for forward direction
        		case 64:
        			q31_rotorposition_hall = DEG_plus120*i16_hall_order + q31_rotorposition_motor_specific;
        			i8_recent_rotor_direction=1;
        			uint16_full_rotation_counter=0;
        			tic_array[0]=ui16_timertics;
        			break;
        		case 45:
        			q31_rotorposition_hall = DEG_plus180*i16_hall_order + q31_rotorposition_motor_specific;
        			i8_recent_rotor_direction=1;
        			tic_array[1]=ui16_timertics;
        			break;
        		case 51:
        			q31_rotorposition_hall = DEG_minus120*i16_hall_order + q31_rotorposition_motor_specific;
        			i8_recent_rotor_direction=1;
        			tic_array[2]=ui16_timertics;
        			break;
        		case 13:
        			q31_rotorposition_hall = DEG_minus60*i16_hall_order + q31_rotorposition_motor_specific;
        			i8_recent_rotor_direction=1;
        			uint16_half_rotation_counter=0;
        			tic_array[3]=ui16_timertics;
        			break;
        		case 32:
        			q31_rotorposition_hall = DEG_0*i16_hall_order + q31_rotorposition_motor_specific;
        			i8_recent_rotor_direction=1;
        			tic_array[4]=ui16_timertics;
        			break;
        		case 26:
        			q31_rotorposition_hall = DEG_plus60*i16_hall_order + q31_rotorposition_motor_specific;
        			i8_recent_rotor_direction=1;
        			tic_array[5]=ui16_timertics;
        			break;

        		//6 cases for reverse direction
        		/*case 46:
        			q31_rotorposition_hall = DEG_plus120*i16_hall_order + q31_rotorposition_motor_specific;
        			i8_recent_rotor_direction=-1;
        			tic_array[0]=ui16_timertics;
        			break;
        		case 62:
        			q31_rotorposition_hall = DEG_plus60*i16_hall_order + q31_rotorposition_motor_specific;
        			i8_recent_rotor_direction=-1;
        			tic_array[1]=ui16_timertics;
        			break;
        		case 23:
        			q31_rotorposition_hall = DEG_0*i16_hall_order + q31_rotorposition_motor_specific;
        			i8_recent_rotor_direction=-1;
        			uint16_half_rotation_counter=0;
        			tic_array[2]=ui16_timertics;
        			break;
        		case 31:
        			q31_rotorposition_hall = DEG_minus60*i16_hall_order + q31_rotorposition_motor_specific;
        			i8_recent_rotor_direction=-1;
        			tic_array[3]=ui16_timertics;
        			break;
        		case 15:
        			q31_rotorposition_hall = DEG_minus120*i16_hall_order + q31_rotorposition_motor_specific;
        			i8_recent_rotor_direction=-1;
        			tic_array[4]=ui16_timertics;
        			break;
        		case 54:
        			q31_rotorposition_hall = DEG_plus180*i16_hall_order + q31_rotorposition_motor_specific;
        			i8_recent_rotor_direction=-1;
        			uint16_full_rotation_counter=0;
        			tic_array[5]=ui16_timertics;
        			break;*/

        		} // end case

#ifdef SPEED_PLL
                if(enum_hall_angle_state != HALL_STATE_SIXSTEP)
                {
        		    q31_pll_angle_per_tic = speed_PLL(q31_rotorposition_PLL, q31_rotorposition_hall);
                }

#endif

                //if(i8_recent_rotor_direction == -1)
                //{
                    // set velocity to zero
                    //uint32_tics_filtered = HALL_TIMEOUT << 3;

                    // XXX - turn of pwm here ?
                    // motor reverse error?
                //}
                ui8_hall_error_count = 0;
            } // end if  expected_forward_hall_state
            else
            {
                // benno: noticed that it happens from time to time that a hall state appears twice consecutively
                // guess: the signal is bouncing and we read the pin value with a slight delay in the interrupt
                // possible solution: use inputcaptue?
                //
                // exemplary hall sequence with error: 4 5 5 3 2
                // if that happens, the code enters this else statement
                // try to recover from it by setting ui8_hall_state_old to the expected hall state
                // the PLL loop should survive a missed hall event, q31_pll_angle_per_tic is simply not updated this time
                // the tim2 counter is reset (otherwise the MOTOR_BLOCKED error is likely to trigger)
                // in order to keep the extrapolation running, q31_rotorposition_hall is advanced by 60 degrees.
                // if everything goes well, the correct hall sequence is recovered in the next interrupt
                // if not -> trigger MOTOR_STATE_HALL_ERROR..

                // if(MS.hall_angle_detect_flag) ??
                // also: not updating ui16_timertics..

                if(ui8_hall_error_count >= 3)
                {
                    if(!enum_motor_error_state)
                    {
                        // if the error state is not checked this error triggers all the time while pushing the bike backwards
                        trigger_motor_error(MOTOR_STATE_HALL_ERROR);
                    }
                }
                else
                {
                    ++ui8_hall_error_count;
                }

                if(ui8_hall_error_count == 1)
                {
                    ui8_hall_state_old = next_expected_forward_hall_state[ui8_hall_state];
                }
                else
                {
                    ui8_hall_state_old = ui8_hall_state;
                }
                q31_rotorposition_hall += DEG_plus60;
        	    __HAL_TIM_SET_COUNTER(&htim2,0); //reset tim2 counter
            }

        } // end if debounce


	} //end if - Hall Sensor Event Processing

	//PAS processing
	if(GPIO_Pin == PAS_EXTI8_Pin)
	{
		ui8_PAS_flag = 1;
	}

	//Speed processing
	if(GPIO_Pin == Speed_EXTI5_Pin)
	{
        ui8_external_SPEED_control_flag = 1; //with debounce
	}

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
//#if ( (DISPLAY_TYPE != DISPLAY_TYPE_AUREUS) && (DISPLAY_TYPE != DISPLAY_TYPE_DEBUG) ) // -> UART_IdleItCallback
//	ui8_UART_flag=1;
//#endif
//

// benno: disabled dma channel 5 interrupts -> see MX_DMA_Init
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
	ui8_g_UART_TxCplt_flag=1;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle) 
{
    Display_Init(&MS);
}


int32_t map (int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
  // if input is smaller/bigger than expected return the min/max out ranges value
  if (x < in_min)
    return out_min;
  else if (x > in_max)
    return out_max;

  // map the input to the output range.
  // round up if mapping bigger ranges to smaller ranges
  else  if ((in_max - in_min) > (out_max - out_min))
    return (x - in_min) * (out_max - out_min + 1) / (in_max - in_min + 1) + out_min;
  // round down if mapping smaller ranges to bigger ranges
  else
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//assuming, a proper AD conversion takes 350 timer tics, to be confirmed. DT+TR+TS deadtime + noise subsiding + sample time
/*void dyn_adc_state(q31_t angle){
	if (switchtime[2]>switchtime[0] && switchtime[2]>switchtime[1]){
		MS.char_dyn_adc_state = 1; // -90° .. +30°: Phase C at high dutycycles
		if(switchtime[2]>1500)TIM1->CCR4 =  switchtime[2]-TRIGGER_OFFSET_ADC;
		else TIM1->CCR4 = TRIGGER_DEFAULT;
	}

	if (switchtime[0]>switchtime[1] && switchtime[0]>switchtime[2]) {
		MS.char_dyn_adc_state = 2; // +30° .. 150° Phase A at high dutycycles
		if(switchtime[0]>1500)TIM1->CCR4 =  switchtime[0]-TRIGGER_OFFSET_ADC;
		else TIM1->CCR4 = TRIGGER_DEFAULT;
	}

	if (switchtime[1]>switchtime[0] && switchtime[1]>switchtime[2]){
		MS.char_dyn_adc_state = 3; // +150 .. -90° Phase B at high dutycycles
		if(switchtime[1]>1500)TIM1->CCR4 =  switchtime[1]-TRIGGER_OFFSET_ADC;
		else TIM1->CCR4 = TRIGGER_DEFAULT;
	}
}*/



void dyn_adc_state(q31_t angle)
{
    uint16_t tph1N = _T - switchtime[0];
    uint16_t tph2N = _T - switchtime[1];
    uint16_t tph3N = _T - switchtime[2];


	if (switchtime[2] > switchtime[0] && switchtime[2] > switchtime[1])
    {
		MS.char_dyn_adc_state = 1; // 180 - 300 degrees -> phase 3 at high duty cycles
        // measure phase 1 & 2
		if(switchtime[2] > 1500)
        {
            if( (tph1N > (2 * (tph3N + PWM_DEAD_TIME))) && (tph2N > (2 * (tph3N + PWM_DEAD_TIME))) )
                TIM1->CCR4 =  switchtime[2] - TRIGGER_OFFSET_ADC;
            else
		        TIM1->CCR4 = TRIGGER_DEFAULT;
        }
		else
        {
            TIM1->CCR4 = TRIGGER_DEFAULT;
        }
	}
	else if (switchtime[0] > switchtime[1] && switchtime[0] > switchtime[2]) 
    {
		MS.char_dyn_adc_state = 2; // -60 - +60 degrees -> phase 1 at high duty cycles
        // measure phase 2 & 3
		if(switchtime[0] > 1500)
        {
            if( (tph2N > (2 * (tph1N + PWM_DEAD_TIME))) &&  (tph3N > (2 * (tph1N + PWM_DEAD_TIME))) )
                TIM1->CCR4 =  switchtime[0] - TRIGGER_OFFSET_ADC;
            else
                TIM1->CCR4 = TRIGGER_DEFAULT;
        }
		else
        {
            TIM1->CCR4 = TRIGGER_DEFAULT;
        }
	}
    else //if (switchtime[1]>switchtime[0] && switchtime[1]>switchtime[2])
    {
		MS.char_dyn_adc_state = 3; // 60 - 180 degrees -> phase 2 at high duty cycles
        // measure phase 3 & 1
		if(switchtime[1] > 1500)
        {
            if( (tph1N > (2 * (tph2N + PWM_DEAD_TIME))) && (tph3N > (2 * (tph2N + PWM_DEAD_TIME))) )
                TIM1->CCR4 =  switchtime[1] - TRIGGER_OFFSET_ADC;
            else
                TIM1->CCR4 = TRIGGER_DEFAULT;
        }
		else
        {
            TIM1->CCR4 = TRIGGER_DEFAULT;
        }
	}
}

static void set_inj_channel(char state){
	switch (state)
	{
	case 1: //Phase C at high dutycycles, read current from phase A + B
		 {
			 ADC1->JSQR=0b00100000000000000000; //ADC1 injected reads phase A JL = 0b00, JSQ4 = 0b00100 (decimal 4 = channel 4)
			 ADC1->JOFR1 = ui16_ph1_offset;
			 ADC2->JSQR=0b00101000000000000000; //ADC2 injected reads phase B, JSQ4 = 0b00101, decimal 5
			 ADC2->JOFR1 = ui16_ph2_offset;


		 }
			break;
	case 2: //Phase A at high dutycycles, read current from phase C + B
			 {
				 ADC1->JSQR=0b00110000000000000000; //ADC1 injected reads phase C, JSQ4 = 0b00110, decimal 6
				 ADC1->JOFR1 = ui16_ph3_offset;
				 ADC2->JSQR=0b00101000000000000000; //ADC2 injected reads phase B, JSQ4 = 0b00101, decimal 5
				 ADC2->JOFR1 = ui16_ph2_offset;


			 }
				break;

	case 3: //Phase B at high dutycycles, read current from phase A + C
			 {
				 ADC1->JSQR=0b00100000000000000000; //ADC1 injected reads phase A JL = 0b00, JSQ4 = 0b00100 (decimal 4 = channel 4)
				 ADC1->JOFR1 = ui16_ph1_offset;
				 ADC2->JSQR=0b00110000000000000000; //ADC2 injected reads phase C, JSQ4 = 0b00110, decimal 6
				 ADC2->JOFR1 = ui16_ph3_offset;


			 }
				break;


	}


}

void autodetect()
{
    printf_("\nAUTODETECT\n");
   	MS.hall_angle_detect_flag = 0;
    // effects of hall_angle_detect_flag = 0
    //  - q31_rotorposition_absolute is not updated in HAL_ADCEx_InjectedConvCpltCallback
    //  - ui8_hall_state_old is not set in HAL_GPIO_EXTI_Callback (no sure if this is necessary)
    //  - runPIcontrol: does nothing and returns immediately (open loop control -> u_d is set in this function)

   	q31_rotorposition_absolute = DEG_plus180;
   	uint8_t zerocrossing = 0;  // nulldurchgang
   	q31_t diffangle = 0;
    //
    MS.u_q = 0;
    MS.u_d = 250;
    MS.foc_alpha = 0;
    enable_pwm();
    //
   	HAL_Delay(5);
   	for(i = 0; i < 1080; i++)
    {
   		q31_rotorposition_absolute += 11930465; //drive motor in open loop with steps of 1°
   		HAL_Delay(5);
   		if (q31_rotorposition_absolute > -60 && q31_rotorposition_absolute < 300)
        {
            // the rotor is at zero degrees absolute position
            // ui8_hall_case -> the last recorded hall transition
            // zerocrossing -> the next expected hall transition
            // example: after the transition 64, the transition 45 should follow
            // diffangle: angle of the 'zerocrossing' hall event
   			switch (ui8_hall_case) //12 cases for each transition from one stage to the next. 6x forward, 6x reverse
   		        {
   					//6 cases for forward direction
   					case 64:
   						zerocrossing = 45;
   						diffangle=DEG_plus180;
   						break;
   					case 45:
   						zerocrossing = 51;
   						diffangle=DEG_minus120;
   						break;
   					case 51:
   						zerocrossing = 13;
   						diffangle=DEG_minus60;
   						break;
   					case 13:
   						zerocrossing = 32;
   						diffangle=DEG_0;
   						break;
   					case 32:
   						zerocrossing = 26;
   						diffangle=DEG_plus60;
   						break;
   					case 26:
   						zerocrossing = 64;
   						diffangle=DEG_plus120;
   						break;

   					//6 cases for reverse direction
   					case 46:
   						zerocrossing = 62;
   						diffangle=-DEG_plus60;
   						break;
   					case 62:
   						zerocrossing = 23;
   						diffangle=-DEG_0;
   						break;
   					case 23:
   						zerocrossing = 31;
   						diffangle=-DEG_minus60;
   						break;
   					case 31:
   						zerocrossing = 15;
   						diffangle=-DEG_minus120;
   						break;
   					case 15:
   						zerocrossing = 54;
   						diffangle=-DEG_plus180;
   						break;
   					case 54:
   						zerocrossing = 46;
   						diffangle=-DEG_plus120;
   						break;

   					} // end case
   		}

   		if(ui8_hall_state_old != ui8_hall_state)
        {
       		ui8_hall_state_old = ui8_hall_state;
   		    printf_("angle: %d, hallstate:  %d, hallcase %d \n", q31_degree_to_degree(q31_rotorposition_absolute), ui8_hall_state , ui8_hall_case);

   		    if(ui8_hall_case == zerocrossing)
   		    {
   		    	//q31_rotorposition_motor_specific = q31_rotorposition_absolute-diffangle-(1<<31);
   		    	q31_rotorposition_motor_specific = q31_rotorposition_absolute - diffangle - DEG_plus180;
   		    	printf_("   ZEROCROSSING: angle: %d, hallstate:  %d, hallcase %d \n", q31_degree_to_degree(q31_rotorposition_absolute), ui8_hall_state , ui8_hall_case);
   		    }
   		}
   	}
    MS.u_d = 0;
    disable_pwm();
    MS.hall_angle_detect_flag=1;

    HAL_FLASH_Unlock();
    EE_WriteVariable(EEPROM_POS_SPEC_ANGLE, q31_rotorposition_motor_specific>>16);
    if(i8_recent_rotor_direction == 1){
    	EE_WriteVariable(EEPROM_POS_HALL_ORDER, 1);
    	i16_hall_order = 1;
    }
    else{
    	EE_WriteVariable(EEPROM_POS_HALL_ORDER, -1);
    	i16_hall_order = -1;
    }

    HAL_FLASH_Lock();


#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)
    printf_("Motor specific angle:  %d (q31_degree), %d (degree), direction %d \n ", q31_rotorposition_motor_specific, q31_degree_to_degree(q31_rotorposition_motor_specific), i16_hall_order);
#endif

    HAL_Delay(5);
}

void get_standstill_position(){
	  HAL_Delay(100);
	  HAL_GPIO_EXTI_Callback(GPIO_PIN_0); //read in initial rotor position
		switch (ui8_hall_state)
			{
			//6 cases for forward direction
			case 2:
				q31_rotorposition_hall = DEG_0 + q31_rotorposition_motor_specific;
				break;
			case 6:
				q31_rotorposition_hall = DEG_plus60 + q31_rotorposition_motor_specific;
				break;
			case 4:
				q31_rotorposition_hall = DEG_plus120 + q31_rotorposition_motor_specific;
				break;
			case 5:
				q31_rotorposition_hall = DEG_plus180 + q31_rotorposition_motor_specific;
				break;
			case 1:
				q31_rotorposition_hall = DEG_minus120 + q31_rotorposition_motor_specific;
				break;
			case 3:
				q31_rotorposition_hall = DEG_minus60 + q31_rotorposition_motor_specific;
				break;

			}

        ui8_hall_state_old = ui8_hall_state;
		//q31_rotorposition_absolute = q31_rotorposition_hall;
        // add 30 degrees
        q31_rotorposition_absolute = q31_rotorposition_hall + (DEG_plus60 >> 1);
}

int32_t speed_to_tics(uint8_t speed)
{
    return WHEEL_CIRCUMFERENCE * 5 * 3600 / (6 * GEAR_RATIO * speed * 10);
}

int8_t tics_to_speed(uint32_t tics)
{
    return WHEEL_CIRCUMFERENCE * 5 * 3600 / (6 * GEAR_RATIO * tics * 10);
}

int16_t internal_tics_to_speedx100(uint32_t tics)
{
    // returns the speed in kmh x 100
    return WHEEL_CIRCUMFERENCE * 50 * 3600 / (6 * GEAR_RATIO * tics);
}

int16_t external_tics_to_speedx100(uint32_t tics)
{
    return WHEEL_CIRCUMFERENCE * 8 * 360 / (PULSES_PER_REVOLUTION * tics);
}

int16_t q31_degree_to_degree(q31_t q31_degree)
{
    return ((q31_degree >> 23) * 180) >> 8;
}

static q31_t get_battery_power()
{
    // returns battery_power in [W] x 10

    // battery to phase current conversion
    // I_B : battery current
    // i_ph : phase current
    // s : pwm duty cycle (= u_q / _T)
    // theory: I_B = (3/4) * s * i_ph

    // MS.i_q = direct adc value (x32 -> ui8_foc_counter!!)
    // (MS.u_q >> 11) -> pwm duty cycle 's'

    // MS.i_q * CAL_I = phase current in mA x32 (ui8_foc_counter!!)
    //

    // battery current
    q31_t battery_current_mA = CAL_I * ((MS.i_q * MS.u_q) >> 11);           // battery current in mA x 32 (foc_counter)
    battery_current_mA = (battery_current_mA * 3) >> (5 + 2);               // battery current in mA   /   >> 5 because of sampling (32), >> 2 factor 3/4
    //
    CurrentData.q31_battery_current_mA_cumulated -= CurrentData.q31_battery_current_mA_cumulated >> CurrentData.ui8_battery_current_shift;
    CurrentData.q31_battery_current_mA_cumulated += battery_current_mA;
    battery_current_mA = CurrentData.q31_battery_current_mA_cumulated >> CurrentData.ui8_battery_current_shift;  // battery current in mA   // filtered
    CurrentData.q31_battery_current_mA = battery_current_mA;
    
    //q31_t battery_voltage = (MS.Voltage * CAL_BAT_V) >> 5;   // battery voltage in mV

    // battery power in W x 10
    q31_t battery_power_x10 = (battery_current_mA * BatteryVoltageData.q31_battery_voltage_V_x10) / 1000;

    return battery_power_x10;
}

static q31_t get_target_power()
{
    // return requested power in [W] x 10

    // --------------------------------------------------------
    // power via UART

    /*if(DD.go)
    {
        return DD.ui16_value * 10;
    }
    else
    {
        return 0;
    }*/


    
    // --------------------------------------------------------
    // regular power control

    if(PedalData.uint32_PAS < PAS_TIMEOUT)
    {


        //return 1500;

        //return DA.Rx.AssistLevel * 10;

        uint32_t PAS_mod = PedalData.uint32_PAS;
        if(PAS_mod > 660)
        {
            PAS_mod = 660;  // 33 rpm from the beginning!
        }


        
        // tics to omega -> omega = 2285 / tics
        // tics to rpm -> rpm = 21818 / tics 
        //
#if DISPLAY_TYPE == DISPLAY_TYPE_AUREUS
        uint32_t pas_omega_x10 = (2285 * MS.ui8_assist_level) / PAS_mod;               // including the assistfactor x10
#else
        uint32_t pas_omega_x10 = (2285 * (MS.ui16_dbg_value)) / PAS_mod;                // including the assistfactor x10
#endif
        //uint32_t pas_omega_x10 = (2285 * (28)) / uint32_PAS;                // including the assistfactor x10
        //uint32_t pas_omega_x10 = (2285 * (DD.ui16_value)) / uint32_PAS;                // including the assistfactor x10
        //uint32_t pas_omega_x10 = (2285 * (1.0)) / PAS_mod;                // including the assistfactor x10
    	//uint16_t torque_nm = ui16_reg_adc_value >> 4; // very rough estimate, todo verify again
    	//uint16_t torque_nm = (uint32_torque_cumulated >> 4) >> 4;
    	q31_t pedal_power_x10 = (pas_omega_x10 * PedalData.uint32_torque_Nm_x10) / 10;
        return pedal_power_x10;
    }
    else
    {
        return 0;
    }

}

static void limit_target_power(q31_t* target_power)
{
    // ---------------------------------------
    // limit power
    if(*target_power > 10000)
    {
        *target_power = 10000;
    }

    if(*target_power < 0)
    {
        *target_power = 0;
    }

    // ---------------------------------------
    // limit battery current
    // q31_t battery_voltage = (MS.Voltage * CAL_BAT_V) >> 5;  // battery voltage in mv
    q31_t limit_x10 = BatteryVoltageData.q31_battery_voltage_V_x10 * BATTERYCURRENT_MAX / 10;
    if(*target_power > limit_x10)
    {
        *target_power = limit_x10;
    }
    
    // ---------------------------------------
    // limit phase current
    // I_B : battery current, s : duty cycle (u_q / _T), i_ph = phase current
    // assuming cos(phi) = 1 -> I_B = (3 / 4) * s * i_ph
    //
    // I_B_max = (3/4) * s * i_ph_max
        
    q31_t bat_max_current_x10 = (3 * PH_CURRENT_MAX * MS.u_q) >> (11 + 2);
    limit_x10 = BatteryVoltageData.q31_battery_voltage_V_x10 * bat_max_current_x10 / 10;
    if(MS.u_q > 300)
    {
        // if u_q = 0 -> the limit is zero!
        if(*target_power > limit_x10)
        {
            *target_power = limit_x10;
        }
    }
    else
    {
        // lets hope nothing happens below u_q = 300
    }


    // ---------------------------------------
    // limit velocity

    uint16_t speed_kmh_x10 = (WheelSpeedData.uint32_SPEEDx100_kmh_cumulated >> WheelSpeedData.ui8_speed_shift) / 10;
    //uint16_t V2 = SPEEDLIMIT * 10 + 32;  // 482
    uint16_t V2 = SPEEDLIMIT * 10 + 16;   //  466
    uint16_t V1 = SPEEDLIMIT * 10;       // 450

    if (speed_kmh_x10 > V2)
    {
        *target_power = 0;
    }
    else if(speed_kmh_x10 > V1)
    {
        //*target_power = *target_power * (V2 - speed_x10) / 32;
        *target_power = *target_power * (V2 - speed_kmh_x10) / 16;
    }

    // ---------------------------------------
    // todo: limit temperature
}

// todo rename this function to amplitude control
static void i_q_control()
{
    static uint8_t i_q_control_state = 0;       // idle

    static uint32_t print_count = 0;

    q31_t battery_power = get_battery_power();
    q31_t target_power = get_target_power();
    limit_target_power(&target_power);
    //
    static q31_t u_q_temp = 0;

    if(enum_motor_error_state)
    {
        i_q_control_state = 0;
    }

    if(i_q_control_state == 0)
    {
        /* IDLE STATE */

        u_q_temp = 0;

        PI_iq.integral_part = 0;
        CurrentData.q31_battery_current_mA_cumulated = 0;
        CurrentData.q31_battery_current_mA = 0;
        MS.i_q = 0;
        MS.i_d = 0;

        if(ui8_pwm_enabled_flag)
        {
            disable_pwm();
        }
        //

        // enable pwm if power is requested or velocity goes above a certain limit
        if(target_power > 0  || (ui16_timertics < MOTOR_AUTO_ENABLE_THRESHOLD) )
        {
            if(!enum_motor_error_state)
            {
                if(ui16_timertics > (MOTOR_ENABLE_THRESHOLD))      // do not enable pwm above a ceratin limit velocity
                {
                    enable_pwm();
                    // i plan to disable the pwm as soon as ui16_timertics > SIXSTEPTHRESHOLD_DOWN
                    // the timeout gives the motor time to accelerate the bike above this threshold
                    // otherwise it immediately disables the pwm again 
                    ui16_motor_init_state_timeout = 16 * 3; // 3s timeout
                    i_q_control_state = 1; 
                }
            }
        }
    }

    if(i_q_control_state == 1)
    {
        /* PI-CONTROL */

        if(target_power < 200)
        {
            // minimum of 20W assist, just to make sure the motor does not slow down the ride
            target_power = 200;
        }

        PI_iq.setpoint = target_power;
        PI_iq.recent_value = battery_power;
        u_q_temp = PI_control(&PI_iq);


        if( ui16_timertics > SIXSTEPTHRESHOLD_DOWN && ui16_motor_init_state_timeout == 0 )
        {
            i_q_control_state = 0;
        }


        if((uint16_full_rotation_counter>7999||uint16_half_rotation_counter>7999))
        {
            i_q_control_state = 0;
		}

        //u_q_temp = u_q_target;
    }


    if(u_q_temp > _U_MAX)
    {
        u_q_temp = _U_MAX;
    }
    if(u_q_temp < 0)
    {
        u_q_temp = 0;
    }

    MS.u_q = u_q_temp;
    MS.u_d = 0;
    
    //if(ui8_g_UART_TxCplt_flag && (print_count >= 10) && ui8_pwm_enabled_flag)
    //if(ui8_g_UART_TxCplt_flag && (print_count >= 200))
    //{
        //sprintf_(buffer, "%u %d %d %d\n", i_q_control_state, u_q_temp, battery_power, target_power);
        //debug_print(buffer, strlen(buffer));
        //print_count = 0;
    //}

    ++print_count;
}

// todo: rename this function to angle control
static void i_d_control()
{
    static uint8_t i_d_control_state = 0;       // idle

    static uint32_t print_count = 0;



    q31_t i_d = (MS.i_d >> 5) * CAL_I; // i_d in mA
    
    q31_i_d_cumulated -= q31_i_d_cumulated >> 3;
    q31_i_d_cumulated += i_d;

    q31_t alpha_temp = 0;
    //alpha_temp = q31_degree * DD.ui16_value2;
    
    if(enum_motor_error_state)
    {
        i_d_control_state = 0;
    }

    if(i_d_control_state == 0)
    {
        PI_id.integral_part = 0;
        q31_i_d_cumulated = 0;
        MS.foc_alpha = 0;
        alpha_temp = 0;

        // todo: if targetpower > 100W ... and HALL_STATE != SIXSTEP..
        if( (enum_hall_angle_state == HALL_STATE_PLL) && ui8_pwm_enabled_flag)
        {
            if(!enum_motor_error_state)
            {
                i_d_control_state = 1;
            }
        }
    }

    if(i_d_control_state == 1)
    {
        // if targetpower < 50 W ..
        if( (enum_hall_angle_state == HALL_STATE_SIXSTEP) || !ui8_pwm_enabled_flag)
        {
            i_d_control_state = 0;
        }
        else
        {
            PI_id.setpoint = 0;
            //PI_id.recent_value = i_d;
            PI_id.recent_value = q31_i_d_cumulated >> 3;
            alpha_temp = PI_control(&PI_id);
            alpha_temp = -alpha_temp;
        }
    }
    

    q31_t max_alpha =  Q31_DEGREE * 25;
    q31_t min_alpha =  -Q31_DEGREE * 25;


    if(alpha_temp > max_alpha)
    {
        alpha_temp = max_alpha;
    }
    if(alpha_temp < min_alpha)
    {
        alpha_temp = min_alpha;
    }
    
    if(ui8_g_UART_TxCplt_flag && (print_count >= 10) && ui8_pwm_enabled_flag)
    {
        //q31_t deg = (alpha_temp * 10) / Q31_DEGREE * 10;
        //sprintf_(buffer, "%d %d %d\n", i_q, i_d, deg);
        //debug_print(buffer, strlen(buffer));
        //print_count = 0;
    }

    MS.foc_alpha = alpha_temp;
    MS.u_d = 0;

    ++print_count;

}


void runPIcontrol()
{
    if(MS.hall_angle_detect_flag == 0)
    {
        return;
    }
    
    i_q_control();
    i_d_control();
    
    // testing - directly setting duty cycle
    /*if(DD.go)
    {
        MS.u_q = DD.ui16_value;
        MS.u_d = 0;
        MS.foc_alpha = 0;

        if(!ui8_pwm_enabled_flag)
        {
            enable_pwm();
        }
    }
    else
    {
        MS.u_q = 0;
        MS.u_d = 0;
        MS.foc_alpha = 0;
        if(ui8_pwm_enabled_flag)
        {
            disable_pwm();
        }
    }*/

    PI_flag = 0;
}


q31_t speed_PLL (q31_t ist, q31_t soll)
{
    q31_t delta = soll - ist;

    q31_t q31_speed_pll_p = delta >> P_FACTOR_PLL;   				//7 for Shengyi middrive, 10 for BionX IGH3
    q31_speed_pll_i += delta >> I_FACTOR_PLL;				//11 for Shengyi middrive, 10 for BionX IGH3
        
    if(delta < 0) 
    {
        q31_pll_abs_delta = -delta;
    }
    else
    {
        q31_pll_abs_delta = delta;
    }

    if(enum_hall_angle_state == HALL_STATE_PLL)
    {
        if(q31_pll_abs_delta > (Q31_DEGREE * 25))
        {
            // ERROR
            //trigger_motor_error(MOTOR_STATE_PLL_ERROR)

            // PLL seems to be in trouble
            // fallback to extraploation
            ui8_extrapolation_hall_count = 30;
            enum_hall_angle_state = HALL_STATE_EXTRAPOLATION;
        }
    }

    return q31_speed_pll_i + q31_speed_pll_p;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
