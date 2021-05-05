
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


#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
  #include "display_kingmeter.h"
#endif

#if (DISPLAY_TYPE & DISPLAY_TYPE_AUREUS)
  #include "display_aureus.h"
#endif

#if (DISPLAY_TYPE & DISPLAY_TYPE_DEBUG)
  #include "display_debug.h"
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
  #include "display_bafang.h"
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_KUNTENG)
  #include "display_kunteng.h"
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_EBiCS)
  #include "display_ebics.h"
#endif


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
uint16_t ui16_timertics=5000; 					//timertics between two hall events for 60° interpolation
uint16_t ui16_reg_adc_value; // torque
uint32_t ui32_reg_adc_value_filter;
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
uint8_t ui8_slowloop_counter=0;
volatile uint8_t ui8_adc_inj_flag=0;
volatile uint8_t ui8_adc_regular_flag=0;
uint8_t ui8_speedcase=0;
uint8_t ui8_speedfactor=0;
int8_t i8_direction= REVERSE; //for permanent reverse direction
int8_t i8_reverse_flag = 1; //for temporaribly reverse direction

volatile uint8_t ui8_adc_offset_done_flag=0;
volatile uint8_t ui8_UART_flag=0;
volatile uint8_t ui8_Push_Assist_flag=0;
volatile uint8_t ui8_UART_TxCplt_flag=1;
volatile uint8_t ui8_PAS_flag=0;
volatile uint8_t ui8_SPEED_flag=0;
volatile uint8_t ui8_SPEED_control_flag=0;
volatile uint8_t ui8_BC_limit_flag=0;  //flag for Battery current limitation
volatile uint8_t ui8_6step_flag=0;
uint32_t uint32_PAS_counter = PAS_TIMEOUT;
uint32_t uint32_PAS_HIGH_counter= 0;
uint32_t uint32_PAS_HIGH_accumulated= 32000;
uint32_t uint32_PAS_fraction= 100;
uint32_t uint32_SPEED_counter=32000;
uint32_t uint32_SPEEDx100_cumulated=0;
uint32_t uint32_PAS=32000;

uint32_t uint32_PAS_raw=32000;
uint8_t uint8_pedaling = 0;

q31_t q31_rotorposition_PLL = 0;
q31_t q31_pll_angle_per_tic = 0;

uint8_t ui8_UART_Counter=0;
int8_t i8_recent_rotor_direction=1;
int16_t i16_hall_order=1;

uint32_t uint32_torque_cumulated=0;
uint32_t uint32_PAS_cumulated=32000;
uint16_t uint16_mapped_throttle=0;
uint16_t uint16_mapped_PAS=0;
uint16_t uint16_half_rotation_counter=0;
uint16_t uint16_full_rotation_counter=0;
int32_t int32_current_target=0;
int32_t int32_temp_current_target=0;

q31_t q31_t_Battery_Current_accumulated=0;

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

//Rotor angle scaled from degree to q31 for arm_math. -180°-->-2^31, 0°-->0, +180°-->+2^31
const q31_t DEG_0 = 0;
const q31_t DEG_plus60 = 715827883; //744755980
const q31_t DEG_plus120= 1431655765; //1442085801
const q31_t DEG_plus180= 2147483647; //2143375202
const q31_t DEG_minus60= -715827883; //-704844603
const q31_t DEG_minus120= -1431655765; //-1400256473
    
const q31_t Q31_DEGREE = 11930464;

const uint32_t ui32_wheel_speed_tics_lower_limit  = WHEEL_CIRCUMFERENCE * 5 * 3600 / (6 * GEAR_RATIO * SPEEDLIMIT * 10); //tics=wheelcirc*timerfrequency/(no. of hallevents per rev*gear-ratio*speedlimit)*3600/1000000
const uint32_t ui32_wheel_speed_tics_higher_limit = WHEEL_CIRCUMFERENCE * 5 * 3600 / (6 * GEAR_RATIO * (SPEEDLIMIT + 3) * 10);

uint32_t uint32_tics_filtered=1000000;

uint16_t VirtAddVarTab[NB_OF_VAR] = {0x01, 0x02, 0x03};


//variables for display communication
#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
KINGMETER_t KM;
#endif

#if (DISPLAY_TYPE & DISPLAY_TYPE_AUREUS)
DISPLAY_AUREUS_t DA;
#endif

#if (DISPLAY_TYPE & DISPLAY_TYPE_DEBUG)
DISPLAY_DEBUG_t DD;
#endif

//variables for display communication
#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
BAFANG_t BF;
#endif

#if (DISPLAY_TYPE & DISPLAY_TYPE_EBiCS)
uint8_t ui8_main_LEV_Page_counter=0;
uint8_t ui8_additional_LEV_Page_counter=0;
uint8_t ui8_LEV_Page_to_send=1;
#endif



MotorState_t MS;
MotorParams_t MP;

//structs for PI_control
PI_control_t PI_iq;
PI_control_t PI_id;
PI_control_t PI_speed;


int16_t battery_percent_fromcapacity = 50; 			//Calculation of used watthours not implemented yet
int16_t wheel_time = 1000;							//duration of one wheel rotation for speed calculation
int16_t current_display;							//pepared battery current for display


typedef enum {SIXSTEP = 0, PLL = 1} hall_angle_state_t;
static hall_angle_state_t enum_hall_angle_state = SIXSTEP;

typedef enum {NORMAL = 0, BLOCKED = 1} motor_error_state_t;
static motor_error_state_t enum_motor_error_state = NORMAL;
static uint16_t ui16_motor_error_state_timeout = 0;

static q31_t q31_speed_pll_i = 0;


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
#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
void kingmeter_update(void);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
void bafang_update(void);
#endif

static void dyn_adc_state(q31_t angle);
static void set_inj_channel(char state);
void get_standstill_position();
q31_t speed_PLL (q31_t ist, q31_t soll);
int32_t map (int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);
int32_t speed_to_tics (uint8_t speed);
int8_t tics_to_speed (uint32_t tics);
int16_t internal_tics_to_speedx100 (uint32_t tics);
int16_t external_tics_to_speedx100 (uint32_t tics);


static uint8_t ui8_pwm_enabled_flag = 0;

static void enable_pwm()
{
	uint16_half_rotation_counter=0;
	uint16_full_rotation_counter=0;
    // why 1023?
	TIM1->CCR1 = 1023;
	TIM1->CCR2 = 1023;
	TIM1->CCR3 = 1023;
	SET_BIT(TIM1->BDTR, TIM_BDTR_MOE);
	//__HAL_TIM_SET_COUNTER(&htim2,0); //reset tim2 counter
	//ui16_timertics=20000; //set interval between two hallevents to a large value
	//i8_recent_rotor_direction=i8_direction*i8_reverse_flag;
	//get_standstill_position();
    ui8_pwm_enabled_flag = 1;
}

static void disable_pwm()
{
   	CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE);
    // why 1023?
    //TIM1->CCR1 = 1023;
    //TIM1->CCR2 = 1023;
    //TIM1->CCR3 = 1023;
    uint32_tics_filtered=1000000; // set velocity to zero
    ui8_pwm_enabled_flag = 0;
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
  MS.hall_angle_detect_flag=1;
  MS.Speed=128000;
  MS.assist_level=1;
  MS.regen_level=7;
  MP.pulses_per_revolution = PULSES_PER_REVOLUTION;
  MP.wheel_cirumference = WHEEL_CIRCUMFERENCE;
  MP.speedLimit=SPEEDLIMIT;

  //init PI structs
  #define SHIFT_ID 0
  PI_id.gain_i=I_FACTOR_I_D;
  PI_id.gain_p=P_FACTOR_I_D;
  PI_id.setpoint = 0;
  PI_id.limit_output_max_shifted = Q31_DEGREE * 20 << SHIFT_ID;
  PI_id.limit_output_min_shifted = -Q31_DEGREE * 20 << SHIFT_ID;
  PI_id.max_step_shifted=Q31_DEGREE << SHIFT_ID;   // shifted value
  PI_id.shift=0;

  #define SHIFT_IQ 10
  PI_iq.gain_i=I_FACTOR_I_Q;
  PI_iq.gain_p=P_FACTOR_I_Q;
  PI_iq.setpoint = 0;
  PI_iq.limit_output_max_shifted = _U_MAX << SHIFT_IQ;
  PI_iq.limit_output_min_shifted = 0 << SHIFT_IQ;        // currently no regeneration
  PI_iq.max_step_shifted= 4 << SHIFT_IQ;      // shifted value
  PI_iq.shift=10;

#ifdef SPEEDTHROTTLE

  PI_speed.gain_i=I_FACTOR_SPEED;
  PI_speed.gain_p=P_FACTOR_SPEED;
  PI_speed.setpoint = 0;
  PI_speed.limit_output = PH_CURRENT_MAX;
  PI_speed.max_step=5000;
  PI_speed.shift=12;
  PI_speed.limit_i=PH_CURRENT_MAX;

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
 SET_BIT(ADC1->CR2, ADC_CR2_JEXTTRIG);//external trigger enable
 __HAL_ADC_ENABLE_IT(&hadc1,ADC_IT_JEOC);
 SET_BIT(ADC2->CR2, ADC_CR2_JEXTTRIG);//external trigger enable
 __HAL_ADC_ENABLE_IT(&hadc2,ADC_IT_JEOC);


  //HAL_ADC_Start_IT(&hadc1);
  HAL_ADCEx_MultiModeStart_DMA(&hadc1, (uint32_t*)adcData, 7);
  HAL_ADC_Start_IT(&hadc2);
  MX_TIM1_Init(); //Hier die Reihenfolge getauscht!
  MX_TIM2_Init();
  MX_TIM3_Init();

 // Start Timer 1
    if(HAL_TIM_Base_Start_IT(&htim1) != HAL_OK)
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

    //TIM1->BDTR |= 1L<<15;
   // TIM1->BDTR &= ~(1L<<15); //reset MOE (Main Output Enable) bit to disable PWM output
    // Start Timer 2
       if(HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
         {
           /* Counter Enable Error */
           Error_Handler();
         }

       // Start Timer 3

       if(HAL_TIM_Base_Start_IT(&htim3) != HAL_OK)
            {
              /* Counter Enable Error */
              Error_Handler();
            }

#if (DISPLAY_TYPE & DISPLAY_TYPE_AUREUS)
			DisplayAureus_Init(&DA);
#endif

#if (DISPLAY_TYPE & DISPLAY_TYPE_DEBUG)
			DisplayDebug_Init(&DD);
    DD.go = 0;
    DD.log = 1;
    DD.light = 0;
    DD.ui16_value = 0;
    DD.ui16_value2 = 0;
            
#endif

#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
       KingMeter_Init (&KM);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
       Bafang_Init (&BF);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_KUNTENG)
       kunteng_init();
       check_message(&MS, &MP);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_EBiCS)
       ebics_init();
#endif


    TIM1->CCR1 = 1023; //set initial PWM values
    TIM1->CCR2 = 1023;
    TIM1->CCR3 = 1023;



    CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE);//Disable PWM

    HAL_Delay(200); //wait for stable conditions

    MS.Voltage = 0;
    MS.Temperature = 0;

    for(i = 0; i < 32; i++)
	{
    	while(!ui8_adc_regular_flag){}
    	ui16_ph1_offset += adcData[2];
    	ui16_ph2_offset += adcData[3];
    	ui16_ph3_offset += adcData[4];
		ui16_torque_offset += adcData[TQ_ADC_INDEX];
        MS.Voltage += adcData[0];
        MS.Temperature += adcData[TEMP_ADC_INDEX];
    	ui8_adc_regular_flag = 0;

    }
    ui16_ph1_offset = ui16_ph1_offset >> 5;
    ui16_ph2_offset = ui16_ph2_offset >> 5;
    ui16_ph3_offset = ui16_ph3_offset >> 5;
	ui16_torque_offset = ui16_torque_offset >> 5;
	ui16_torque_offset += 30; // hardcoded offset -> move to config.h



   	ui8_adc_offset_done_flag = 1;

#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)
   	printf_("phase current offsets:  %d, %d, %d \n ", ui16_ph1_offset, ui16_ph2_offset, ui16_ph3_offset);
   	printf_("torque offset:  %d\n", ui16_torque_offset);
#if (AUTODETECT == 1)
   	autodetect();
	while(1) { };
#endif

#endif
#if (DISPLAY_TYPE != DISPLAY_TYPE_DEBUG || !AUTODETECT)
   	EE_ReadVariable(EEPROM_POS_SPEC_ANGLE, &MP.spec_angle);

   	// set motor specific angle to value from emulated EEPROM only if valid
   	if(MP.spec_angle!=0xFFFF) {
   		q31_rotorposition_motor_specific = MP.spec_angle<<16;
   		EE_ReadVariable(EEPROM_POS_HALL_ORDER, &i16_hall_order);
   	}
#endif

	//q31_rotorposition_motor_specific = -167026406;
	q31_rotorposition_motor_specific = -1789569706;  // -150 degrees
	//q31_rotorposition_motor_specific = -1801499903;  // -152 degrees
	//q31_rotorposition_motor_specific = 0;
	i16_hall_order = 1;
 // set absolute position to corresponding hall pattern.

#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)
    printf_("Lishui FOC v0.9 \n ");
    printf_("Motor specific angle:  %d, direction %d \n ", q31_rotorposition_motor_specific, i16_hall_order);
#endif


    CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE);//Disable PWM

	get_standstill_position();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    if(PI_flag || !ui8_pwm_enabled_flag)
    {
        // only pi control can enable the pwm
        runPIcontrol();
    }

	  //display message processing
	  if(ui8_UART_flag){
	  //{
#if (DISPLAY_TYPE & DISPLAY_TYPE_AUREUS)
		// period [s] = tics x 6 x GEAR_RATIO / frequency    (frequency = 500kHz)
		//DA.Tx.Wheeltime_ms = (uint32_tics_filtered>>3) * 6 * GEAR_RATIO / 500;
        // 05.05.21 -> 20% error ?! -> x 6/5      using 23 /20 = 1.15 
		DA.Tx.Wheeltime_ms = (uint32_tics_filtered>>3) * 6 * GEAR_RATIO * 6 / (500 * 5);
		if(DA.Tx.Wheeltime_ms > 0x0DAC)
		{
			DA.Tx.Wheeltime_ms = 0x0DAC;
		}
		DisplayAureus_Service(&DA);
		if(DA.Rx.Headlight)
		{
   		    HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_SET);
        	//HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
		}
		else
		{
   		    HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_RESET);
		}
  
		  
#endif

#if (DISPLAY_TYPE & DISPLAY_TYPE_DEBUG)
		DisplayDebug_Service(&DD);
		if(DD.light)
		{
   		    HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_SET);
        	//HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
		}
		else
		{
   		    HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_RESET);
		}
#endif


#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
	  kingmeter_update();
#endif


#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
	  bafang_update();
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_KUNTENG)
	  check_message(&MS, &MP);
	  if(MS.assist_level==6)ui8_Push_Assist_flag=1;
	  else ui8_Push_Assist_flag=0;
#endif

#if (DISPLAY_TYPE & DISPLAY_TYPE_EBiCS)
	  process_ant_page(&MS, &MP);
#endif

	  ui8_UART_flag=0;
	  } // end UART processing


    // --------------------------------------------------
	// PAS signal processing
	//
	  
    if(ui8_PAS_flag)
    {
		if(uint32_PAS_counter>100)
        { 
            //debounce
		    uint32_PAS_cumulated -= uint32_PAS_cumulated>>2;
		    uint32_PAS_cumulated += uint32_PAS_counter;
		    uint32_PAS = uint32_PAS_cumulated>>2;
	        uint32_PAS_raw = uint32_PAS_counter;

    	    uint32_PAS_HIGH_accumulated-=uint32_PAS_HIGH_accumulated>>2;
		    uint32_PAS_HIGH_accumulated+=uint32_PAS_HIGH_counter;

		    uint32_PAS_fraction=(uint32_PAS_HIGH_accumulated>>2)*100/uint32_PAS;
		    uint32_PAS_HIGH_counter=0;
		    uint32_PAS_counter =0;
		    ui8_PAS_flag=0;
		    //read in and sum up torque-signal within one crank revolution (for sempu sensor 32 PAS pulses/revolution, 2^5=32)

		    //uint32_torque_cumulated -= uint32_torque_cumulated >> 5;
            //uint32_torque_cumulated += ui16_reg_adc_value;

            uint32_t ui32_reg_adc_value_shifted = ui16_reg_adc_value << 4;

            if(ui32_reg_adc_value_shifted > uint32_torque_cumulated)
            {
                // accept rising values unfiltered
                uint32_torque_cumulated = ui32_reg_adc_value_shifted;
            }
            else
            {
                // filter falling values
		        uint32_torque_cumulated -= uint32_torque_cumulated >> 4;
                uint32_torque_cumulated += ui16_reg_adc_value;
            }
	    }
    }

	if(uint32_PAS_counter >= PAS_TIMEOUT)
	{
	    uint32_PAS = PAS_TIMEOUT;
		uint32_PAS_raw = PAS_TIMEOUT;
		uint32_PAS_cumulated = PAS_TIMEOUT << 2;
		uint32_torque_cumulated = 0;
	}

	if(uint32_PAS_raw >= PAS_TIMEOUT)
	{
	    uint8_pedaling = 0;
	}
	else
	{
		uint8_pedaling = 1;
	}


    
    // --------------------------------------------------
	// SPEED signal processing
    //

	  if(ui8_SPEED_flag){   // benno: this part seems to be for the external sensor

		  if(uint32_SPEED_counter>200){ //debounce
		  MS.Speed = uint32_SPEED_counter;
		  //HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		  uint32_SPEED_counter =0;
		  ui8_SPEED_flag=0;

#if (SPEEDSOURCE == EXTERNAL)
		uint32_SPEEDx100_cumulated -=uint32_SPEEDx100_cumulated>>SPEEDFILTER;
		uint32_SPEEDx100_cumulated +=external_tics_to_speedx100(MS.Speed);
#endif

		  }
	  }

	if(ui8_SPEED_control_flag)
    {
#if (SPEEDSOURCE == INTERNAL)
		uint32_SPEEDx100_cumulated -= uint32_SPEEDx100_cumulated >> SPEEDFILTER;
		uint32_SPEEDx100_cumulated += internal_tics_to_speedx100(uint32_tics_filtered >> 3);
#endif
		ui8_SPEED_control_flag=0;
	} // end speed processing


//------------------------------------------------------------------------------------------------------------

#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG && defined(FAST_LOOP_LOG))
		if(ui8_debug_state==3 && ui8_UART_TxCplt_flag){
	        sprintf_(buffer, "%d, %d, %d, %d, %d, %d\r\n", e_log[k][0], e_log[k][1], e_log[k][2],e_log[k][3],e_log[k][4],e_log[k][5]); //>>24
			i=0;
			while (buffer[i] != '\0')
			{i++;}
			ui8_UART_TxCplt_flag=0;
			HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&buffer, i);
			k++;
			if (k>299){
				k=0;
				ui8_debug_state=0;
				//Obs_flag=0;
			}
		}
#endif

//--------------------------------------------------------------------------------------------------------------------------------------------------
    
    //current target calculation (removed 02.05.21)

    // code blelow is unused, just left it for reference

//------------------------------------------------------------------------------------------------------------
				//enable PWM if power is wanted
		int32_current_target = 0;  // xx
        //int32_current_target = DD.go;
        // todo - remove code below
	    if (int32_current_target>0&&!READ_BIT(TIM1->BDTR, TIM_BDTR_MOE))
        {
		    //speed_PLL(0,0);//reset integral part

		    uint16_half_rotation_counter=0;
		    uint16_full_rotation_counter=0;
		    TIM1->CCR1 = 1023; //set initial PWM values
		    TIM1->CCR2 = 1023;
		    TIM1->CCR3 = 1023;
		    SET_BIT(TIM1->BDTR, TIM_BDTR_MOE);
		    __HAL_TIM_SET_COUNTER(&htim2,0); //reset tim2 counter
		    ui16_timertics=20000; //set interval between two hallevents to a large value
		    i8_recent_rotor_direction=i8_direction*i8_reverse_flag;
		    get_standstill_position();
	    }


//----------------------------------------------------------------------------------------------------------------------------------------------------------
	  //slow loop procedere @16Hz, for LEV standard every 4th loop run, send page,
	  if(ui32_tim3_counter>500){
            
        // benno 05.05 
        // motor blocked -> 2s timeout ..
          if(ui16_motor_error_state_timeout > 0)
          {
              --ui16_motor_error_state_timeout;
          }
          else
          {
              enum_motor_error_state = NORMAL;
          }

		  //MS.Temperature = adcData[TEMP_ADC_INDEX]*41>>8; //0.16 is calibration constant: Analog_in[10mV/°C]/ADC value. Depending on the sensor LM35)

          // storing raw adc data
          MS.Temperature -= (MS.Temperature >> 5);
          MS.Temperature += adcData[TEMP_ADC_INDEX];

          // storing raw adc data
          MS.Voltage -= (MS.Voltage >> 5);
		  MS.Voltage += adcData[0];

		  if(uint32_SPEED_counter>127999){
			  MS.Speed =128000;
#if (SPEEDSOURCE == EXTERNAL)
			  uint32_SPEEDx100_cumulated=0;
#endif
		  }

#ifdef INDIVIDUAL_MODES
		  // GET recent speedcase for assist profile
		  if (uint32_tics_filtered>>3 > speed_to_tics(assist_profile[0][1]))ui8_speedcase=0;
		  else if (uint32_tics_filtered>>3 < speed_to_tics(assist_profile[0][1]) && uint32_tics_filtered>>3 > speed_to_tics(assist_profile[0][2]))ui8_speedcase=1;
		  else if (uint32_tics_filtered>>3 < speed_to_tics(assist_profile[0][2]) && uint32_tics_filtered>>3 > speed_to_tics(assist_profile[0][3]))ui8_speedcase=2;
		  else if (uint32_tics_filtered>>3 < speed_to_tics(assist_profile[0][3]) && uint32_tics_filtered>>3 > speed_to_tics(assist_profile[0][4]))ui8_speedcase=3;
		  else if (uint32_tics_filtered>>3 < speed_to_tics(assist_profile[0][4]))ui8_speedcase=4;

		  ui8_speedfactor = map(uint32_tics_filtered>>3,speed_to_tics(assist_profile[0][ui8_speedcase+1]),speed_to_tics(assist_profile[0][ui8_speedcase]),assist_profile[1][ui8_speedcase+1],assist_profile[1][ui8_speedcase]);


#endif


		  //if((uint16_full_rotation_counter>7999||uint16_half_rotation_counter>7999)&&READ_BIT(TIM1->BDTR, TIM_BDTR_MOE)){

              // todo: check if this is still necessary
			  //CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE); //Disable PWM if motor is not turning
			  //uint32_tics_filtered=1000000;
			  //get_standstill_position();

		  //}

#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG && !defined(FAST_LOOP_LOG))
		  //print values for debugging


		//uint8_t pin_state = HAL_GPIO_ReadPin(PAS_GPIO_Port, PAS_Pin);
		//sprintf_(buffer, "%u %u\n", uint32_PAS, uint32_PAS_raw); 
		//sprintf_(buffer, "%u\n", pin_state); 
		//sprintf_(buffer, "%u\n", ui16_reg_adc_value); 
		//sprintf_(buffer, "%u %u %u %u %u\n", pin_state, adcData[0], adcData[1], adcData[5], adcData[6]);
		//uint32_t pas_rpm = 21818 / uint32_PAS;
        //uint32_t velocity_kmh = 37238 / (uint32_tics_filtered>>3);
        //uint32_t temperature = MS.Temperature >> 5;
		//uint32_t pas_omega = 2285 / uint32_PAS;
		//uint16_t torque_nm = ui16_reg_adc_value >> 4; // very rough estimate, todo verify again
		//uint16_t pedal_power = pas_omega * torque_nm;
        //q31_t battery_voltage = (MS.Voltage * CAL_BAT_V) >> 5;
		//sprintf_(buffer, "%u %u\n", pin_state, pedal_power);
		//sprintf_(buffer, "%u %u\n", READ_BIT(TIM1->BDTR, TIM_BDTR_MOE), DD.go);
		//sprintf_(buffer, "%d \n", MS.Battery_Current);
		//sprintf_(buffer, "%u %u %u \n", uint32_SPEEDx100_cumulated >> 2, velocity_kmh, temperature);
		sprintf_(buffer, "%u %u \n", ui16_reg_adc_value, uint32_torque_cumulated >> 4);

		 //sprintf_(buffer, "%d, %d, %d, %d, %d, %d, %d, %d, %d\r\n", ui16_timertics, MS.i_q, int32_current_target,((temp6 >> 23) * 180) >> 8, (uint16_t)adcData[1], MS.Battery_Current,internal_tics_to_speedx100(uint32_tics_filtered>>3),external_tics_to_speedx100(MS.Speed),uint32_SPEEDx100_cumulated>>SPEEDFILTER);
		 //sprintf_(buffer, "%d, %d, %d, %d\n", int32_temp_current_target, uint32_torque_cumulated, uint32_PAS, MS.assist_level);
		 // sprintf_(buffer, "%d, %d, %d, %d, %d, %d\r\n",ui8_hall_state,(uint16_t)adcData[1],(uint16_t)adcData[2],(uint16_t)adcData[3],(uint16_t)(adcData[4]),(uint16_t)(adcData[5])) ;
		 // sprintf_(buffer, "%d, %d, %d, %d, %d, %d\r\n",tic_array[0],tic_array[1],tic_array[2],tic_array[3],tic_array[4],tic_array[5]) ;

        if(ui8_UART_TxCplt_flag && DD.log)
        {
		    HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&buffer, strlen(buffer));
            ui8_UART_TxCplt_flag = 0;
        }
         
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_EBiCS)
		  ui8_slowloop_counter++;
		  if(ui8_slowloop_counter>3){
			  ui8_slowloop_counter = 0;

			  switch (ui8_main_LEV_Page_counter){
			  case 1: {
				  ui8_LEV_Page_to_send = 1;
			  	  }
			  	  break;
			  case 2: {
				  ui8_LEV_Page_to_send = 2;
			  	  }
			  	  break;
			  case 3: {
				  ui8_LEV_Page_to_send = 3;
			  	  }
			  	  break;
			  case 4: {
				  //to do, define other pages
				  ui8_LEV_Page_to_send = 4;
			  	  }
			  	  break;
			  }//end switch

			  send_ant_page(ui8_LEV_Page_to_send, &MS, &MP);

			  ui8_main_LEV_Page_counter++;
			  if(ui8_main_LEV_Page_counter>4)ui8_main_LEV_Page_counter=1;
		  }

#endif
		  ui32_tim3_counter=0;
	  }// end of slow loop

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
sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;//ADC_SAMPLETIME_239CYCLES_5;
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
sConfig.Channel = ADC_CHANNEL_8;
sConfig.Rank = ADC_REGULAR_RANK_6; // connector AD2
sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;//ADC_SAMPLETIME_239CYCLES_5;
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
  sBreakDeadTimeConfig.DeadTime = 32;
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
  htim2.Init.Period = 64000;
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

#if ((DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER) ||DISPLAY_TYPE==DISPLAY_TYPE_KUNTENG||DISPLAY_TYPE==DISPLAY_TYPE_EBiCS || DISPLAY_TYPE & DISPLAY_TYPE_AUREUS)
  huart1.Init.BaudRate = 9600;
#elif (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
  huart1.Init.BaudRate = 1200;
#else
  //huart1.Init.BaudRate = 115200;
  huart1.Init.BaudRate = 57600;
#endif


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
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

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

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim == &htim3) 
	{
		if(ui32_tim3_counter < 32000) ui32_tim3_counter++;
		//
		if(uint32_PAS_counter < PAS_TIMEOUT)
		{
			uint32_PAS_counter++;
			if(HAL_GPIO_ReadPin(PAS_GPIO_Port, PAS_Pin)) uint32_PAS_HIGH_counter++;
		}
		//
		if (uint32_SPEED_counter<128000)uint32_SPEED_counter++;					//counter for external Speedsensor
		if(uint16_full_rotation_counter<8000)uint16_full_rotation_counter++;	//full rotation counter for motor standstill detection
		if(uint16_half_rotation_counter<8000)uint16_half_rotation_counter++;	//half rotation counter for motor standstill detection

	}
}



// regular ADC callback
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
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
		
		
	ui8_adc_regular_flag=1;

}

//injected ADC

void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	//for oszi-check of used time in FOC procedere
	//HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	  ui32_tim1_counter++;

	/*  else {
	  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	  uint32_SPEED_counter=0;
	  }*/

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
                
    if(ui16_tim2_recent > ui16_timertics + (ui16_timertics >> 2))
    {
        disable_pwm();
        enum_motor_error_state = BLOCKED;
        ui16_motor_error_state_timeout = 16 * 2;

        // TODO - motor blocked flag -> immediately turn off pwm
        enum_hall_angle_state = SIXSTEP; 
    }

    if(MS.hall_angle_detect_flag)
    {   
        //autodetect is not active

        switch(enum_hall_angle_state)
        {
            case SIXSTEP:
                q31_rotorposition_absolute = q31_rotorposition_hall + (DEG_plus60 >> 1);

                // set speed pll integral part
                q31_speed_pll_i = DEG_plus60 / ui16_timertics;
                q31_pll_angle_per_tic = q31_speed_pll_i;
	        
                if(ui16_timertics < SIXSTEPTHRESHOLD_UP)
                {
                    enum_hall_angle_state = PLL;
                }
                break;

            case PLL:
                // PLL
                q31_rotorposition_absolute += q31_pll_angle_per_tic;
                q31_rotorposition_PLL = q31_rotorposition_absolute;
	            
                // extrapolation method
                // interpolate angle between two hallevents by scaling timer2 tics, 10923<<16 is 715827883 = 60°
                //q31_rotorposition_absolute = q31_rotorposition_hall + (q31_t)(i16_hall_order * i8_recent_rotor_direction * ((10923 * ui16_tim2_recent) / ui16_timertics) << 16);
                
                if(ui16_timertics > SIXSTEPTHRESHOLD_DOWN)
                {
                    enum_hall_angle_state = SIXSTEP;
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
	    FOC_calculation(i16_ph1_current, i16_ph2_current, q31_rotorposition_absolute, (((int16_t)i8_direction*i8_reverse_flag)*int32_current_target), &MS);
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
	//Hall sensor event processing
	if(GPIO_Pin == GPIO_PIN_0||GPIO_Pin == GPIO_PIN_1||GPIO_Pin == GPIO_PIN_2) //check for right interrupt source
	{
	ui8_hall_state = GPIOA->IDR & 0b111; //Mask input register with Hall 1 - 3 bits


	ui8_hall_case=ui8_hall_state_old*10+ui8_hall_state;
	if(MS.hall_angle_detect_flag){ //only process, if autodetect procedere is fininshed
	ui8_hall_state_old=ui8_hall_state;
	}

	ui16_tim2_recent = __HAL_TIM_GET_COUNTER(&htim2); // read in timertics since last hall event


	if(ui16_tim2_recent>100){//debounce
		ui16_timertics = ui16_tim2_recent; //save timertics since last hall event
		uint32_tics_filtered-=uint32_tics_filtered>>3;
		uint32_tics_filtered+=ui16_timertics;
	   __HAL_TIM_SET_COUNTER(&htim2,0); //reset tim2 counter
	   ui8_SPEED_control_flag=1;

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
		case 46:
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
			break;

		} // end case

#ifdef SPEED_PLL
        if(enum_hall_angle_state == PLL)
        {
		    q31_pll_angle_per_tic = speed_PLL(q31_rotorposition_PLL, q31_rotorposition_hall);
        }

#endif

        if(i8_recent_rotor_direction == -1)
        {
            // set velocity to zero
            uint32_tics_filtered = 1000000;
        }

	} //end if

	//PAS processing
	if(GPIO_Pin == PAS_EXTI8_Pin)
	{
		ui8_PAS_flag = 1;
	}

	//Speed processing
	if(GPIO_Pin == Speed_EXTI5_Pin)
	{

			ui8_SPEED_flag = 1; //with debounce

	}

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
#if ( (DISPLAY_TYPE != DISPLAY_TYPE_AUREUS) && (DISPLAY_TYPE != DISPLAY_TYPE_DEBUG) ) // -> My_UART_IdleItCallback
	ui8_UART_flag=1;
#endif

}

void My_UART_IdleItCallback(void)
{
#if (DISPLAY_TYPE == DISPLAY_TYPE_AUREUS)
	Aureus_UART_IdleItCallback();
#elif (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)
	Debug_UART_IdleItCallback();
#endif
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
	ui8_UART_TxCplt_flag=1;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle) {
#if (DISPLAY_TYPE == DISPLAY_TYPE_AUREUS)
	DisplayAureus_Init(&DA);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)
	DisplayDebug_Init(&DD);
#endif

#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
       KingMeter_Init (&KM);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
       Bafang_Init (&BF);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_KUNTENG)
       kunteng_init();
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_EBiCS)
       ebics_init();
#endif

}



#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
void kingmeter_update(void)
{
    /* Prepare Tx parameters */

    if(battery_percent_fromcapacity > 10)
    {
        KM.Tx.Battery = KM_BATTERY_NORMAL;
    }
    else
    {
        KM.Tx.Battery = KM_BATTERY_LOW;
    }

    if(__HAL_TIM_GET_COUNTER(&htim2) < 12000)
    {
        // Adapt wheeltime to match displayed speedo value according config.h setting
    	KM.Tx.Wheeltime_ms = ((MS.Speed>>3)*PULSES_PER_REVOLUTION); //>>3 because of 8 kHz counter frequency, so 8 tics per ms
    }
    else
    {
        KM.Tx.Wheeltime_ms = 64000;
    }


    //KM.Tx.Wheeltime_ms = 25;

    KM.Tx.Error = KM_ERROR_NONE;

    KM.Tx.Current_x10 = (uint16_t) (MS.Battery_Current/100); //MS.Battery_Current is in mA


    /* Receive Rx parameters/settings and send Tx parameters */
    KingMeter_Service(&KM);


    /* Apply Rx parameters */

    MS.assist_level = KM.Rx.AssistLevel;

    if(KM.Rx.Headlight == KM_HEADLIGHT_OFF)
        {
        	HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_RESET);
        	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
        }
        else // KM_HEADLIGHT_ON, KM_HEADLIGHT_LOW, KM_HEADLIGHT_HIGH
        {
        	HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        }


    if(KM.Rx.PushAssist == KM_PUSHASSIST_ON)
    {
    	ui8_Push_Assist_flag=1;
    }
    else
    {
    	ui8_Push_Assist_flag=0;
    }



}

#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
void bafang_update(void)
{
    /* Prepare Tx parameters */

	if(adcData[0]*CAL_BAT_V>BATTERY_LEVEL_5)battery_percent_fromcapacity=75;
	else if(adcData[0]*CAL_BAT_V>BATTERY_LEVEL_4)battery_percent_fromcapacity=50;
	else if(adcData[0]*CAL_BAT_V>BATTERY_LEVEL_3)battery_percent_fromcapacity=30;
	else if(adcData[0]*CAL_BAT_V>BATTERY_LEVEL_2)battery_percent_fromcapacity=10;
	else if(adcData[0]*CAL_BAT_V>BATTERY_LEVEL_1)battery_percent_fromcapacity=5;
	else battery_percent_fromcapacity=0;


    	BF.Tx.Battery = battery_percent_fromcapacity;


    if(__HAL_TIM_GET_COUNTER(&htim2) < 12000)
    {
        // Adapt wheeltime to match displayed speedo value according config.h setting
        BF.Tx.Wheeltime_ms = WHEEL_CIRCUMFERENCE*216/(MS.Speed*PULSES_PER_REVOLUTION); // Geschwindigkeit ist Weg pro Zeit Radumfang durch Dauer einer Radumdrehung --> Umfang * 8000*3600/(n*1000000) * Skalierung Bafang Display 200/26,6

    }
    else
    {
        BF.Tx.Wheeltime_ms = 0; //64000;
    }


       BF.Tx.Power = MS.i_q*MS.Voltage;


    /* Receive Rx parameters/settings and send Tx parameters */
    Bafang_Service(&BF,1);



    /* Apply Rx parameters */

//No headlight supported on my controller hardware.
    if(BF.Rx.Headlight)
    {
       // digitalWrite(lights_pin, 0);
    }
    else
    {
       // digitalWrite(lights_pin, 1);
    }


    if(BF.Rx.PushAssist) ui8_Push_Assist_flag=1;
    else ui8_Push_Assist_flag=0;

    MS.assist_level=BF.Rx.AssistLevel;
}

#endif

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
void dyn_adc_state(q31_t angle){
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

void autodetect(){
	SET_BIT(TIM1->BDTR, TIM_BDTR_MOE);
   	MS.hall_angle_detect_flag=0; //set uq to contstant value in FOC.c for open loop control
   	q31_rotorposition_absolute=1<<31;
   	uint8_t zerocrossing=0;
   	q31_t diffangle=0;
   	HAL_Delay(5);
   	for(i=0;i<1080;i++){
   		q31_rotorposition_absolute+=11930465; //drive motor in open loop with steps of 1°
   		HAL_Delay(5);
   		if (q31_rotorposition_absolute>-60&&q31_rotorposition_absolute<300){
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

   		if(ui8_hall_state_old!=ui8_hall_state){
   		printf_("angle: %d, hallstate:  %d, hallcase %d \n",(int16_t)(((q31_rotorposition_absolute>>23)*180)>>8), ui8_hall_state , ui8_hall_case);

   		if(ui8_hall_case==zerocrossing)
   		{
   			q31_rotorposition_motor_specific = q31_rotorposition_absolute-diffangle-(1<<31);
   			printf_("   ZEROCROSSING: angle: %d, hallstate:  %d, hallcase %d \n",(int16_t)(((q31_rotorposition_motor_specific>>23)*180)>>8), ui8_hall_state , ui8_hall_case);
   		}


   		ui8_hall_state_old=ui8_hall_state;
   		}
   	}
   	CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE); //Disable PWM if motor is not turning
    TIM1->CCR1 = 1023; //set initial PWM values
    TIM1->CCR2 = 1023;
    TIM1->CCR3 = 1023;
    MS.hall_angle_detect_flag=1;
    MS.i_d = 0;
    MS.i_q = 0;
    uint32_tics_filtered=1000000;


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
    printf_("Motor specific angle:  %d, direction %d \n ", (int16_t)(((q31_rotorposition_motor_specific>>23)*180)>>8), i16_hall_order);
	printf_("benno: %d\n", q31_rotorposition_motor_specific);
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

		//q31_rotorposition_absolute = q31_rotorposition_hall;
        // add 30 degrees
        q31_rotorposition_absolute = q31_rotorposition_hall + (DEG_plus60 >> 1);
}

int32_t speed_to_tics (uint8_t speed){
	return WHEEL_CIRCUMFERENCE*5*3600/(6*GEAR_RATIO*speed*10);
}

int8_t tics_to_speed (uint32_t tics){
	return WHEEL_CIRCUMFERENCE*5*3600/(6*GEAR_RATIO*tics*10);;
}

int16_t internal_tics_to_speedx100 (uint32_t tics){
	return WHEEL_CIRCUMFERENCE*50*3600/(6*GEAR_RATIO*tics);;
}

int16_t external_tics_to_speedx100 (uint32_t tics){
	return WHEEL_CIRCUMFERENCE*8*360/(PULSES_PER_REVOLUTION*tics);;
}


static q31_t get_battery_power()
{
    // returns battery_power in [W] x 10

    // battery current
    static q31_t battery_current_cumulated = 0;		  
    battery_current_cumulated -= battery_current_cumulated >> 3;
    q31_t battery_current = (MS.i_q * MS.u_q * 38) >> 11;
    battery_current = (battery_current * 3) >> 6; // >> 5 because of sampling (32), >> 1 factor 3/2
    battery_current_cumulated += battery_current;
    battery_current = battery_current_cumulated >> 3;  // battery current in mA
    
    q31_t battery_voltage = (MS.Voltage * CAL_BAT_V) >> 5;   // battery voltage in mV

    q31_t battery_power_x10 = (battery_current >> 5) * (battery_voltage >> 5);    // battery power in mW, left shift from sampling
    battery_power_x10 = battery_power_x10 / 100;      // battery power in W x 10

    return battery_power_x10;
}

static q31_t get_target_power()
{
    // return requested power in [W] x 10

#if DISPLAY_TYPE == DISPLAY_TYPE_DEBUG
    // --------------------------------------------------------
    // power via UART
    if(DD.go)
    {
        return DD.ui16_value * 10;
    }
    else
    {
        return 0;
    }


#elif DISPLAY_TYPE == DISPLAY_TYPE_AUREUS
    
    // --------------------------------------------------------
    // regular power control

    if(uint32_PAS < PAS_TIMEOUT)
    {


        //return 1500;

        //return DA.Rx.AssistLevel * 10;

        uint32_t PAS_mod = uint32_PAS;
        if(PAS_mod < PAS_TIMEOUT && PAS_mod > 660)
        {
            PAS_mod = 660;  // 30 rpm from the beginning!
        }


        
        //uint32_t pas_rpm = 21818 / uint32_PAS;
        //
        uint32_t pas_omega_x10 = (2285 * (DA.Rx.AssistLevel >> 3)) / uint32_PAS;                // including the assistfactor x10
        //uint32_t pas_omega_x10 = (2285 * (1.0)) / PAS_mod;                // including the assistfactor x10
    	//uint16_t torque_nm = ui16_reg_adc_value >> 4; // very rough estimate, todo verify again
    	uint16_t torque_nm = (uint32_torque_cumulated >> 4) >> 4;
    	uint16_t pedal_power_x10 = pas_omega_x10 * torque_nm;
        return pedal_power_x10;
    }
    else
    {
        return 0;
    }
#endif

}

static void limit_target_power(q31_t* target_power)
{
    // ---------------------------------------
    // limit power
    if(*target_power > 8000)
    {
        *target_power = 8000;
    }

    if(*target_power < 0)
    {
        *target_power = 0;
    }

    // ---------------------------------------
    // todo: limit current


    // ---------------------------------------
    // limit velocity

    uint16_t speed_x10 = (uint32_SPEEDx100_cumulated >> SPEEDFILTER) / 10;
    //uint16_t V2 = SPEEDLIMIT * 10 + 32;  // 482
    uint16_t V2 = SPEEDLIMIT * 10 + 16;   //  466
    uint16_t V1 = SPEEDLIMIT * 10;       // 450

    if (speed_x10 > V2)
    {
        *target_power = 0;
    }
    else if(speed_x10 > V1)
    {
        //*target_power = *target_power * (V2 - speed_x10) / 32;
        *target_power = *target_power * (V2 - speed_x10) / 16;
    }

    //if( (uint32_tics_filtered>>3) < ui32_wheel_speed_tics_higher_limit)
    //{
    //    *target_power = 0;
    //}

    // ---------------------------------------
    // todo: limit temperature
}

static void i_q_control()
{
    static uint8_t i_q_control_state = 0;       // idle

    static uint32_t print_count = 0;

    q31_t battery_power = get_battery_power();
    q31_t target_power = get_target_power();
    limit_target_power(&target_power);
    //
    
    /* direct u-q control */
    //q31_t u_q_target = DD.go ? DD.ui16_value : 0;
    static q31_t u_q_temp = 0;

    // todo: error state -> shut down
    if(enum_motor_error_state)
    {
        i_q_control_state = 0;
    }

    if(i_q_control_state == 0)
    {
        /* IDLE STATE */

        u_q_temp = 0;

        PI_iq.integral_part = 0;
        if(ui8_pwm_enabled_flag)
        {
            disable_pwm();
        }
        //

        if(target_power > 0)
        //if(target_power > 0  || (ui16_timertics < SIXSTEPTHRESHOLD_UP))
        {
            if(!enum_motor_error_state)
            {
                // transition to ramp-up if power is requested
                enable_pwm();
                i_q_control_state = 1; 
            }
        }
    }

    if(i_q_control_state == 1)
    {
        /* PI-CONTROL */

        PI_iq.setpoint = target_power;
        PI_iq.recent_value = battery_power;
        u_q_temp = PI_control(&PI_iq);


        // todo error states
        if(ui16_timertics > SIXSTEPTHRESHOLD_DOWN)
        {
            //i_q_control_state = 0;
        }
		
        if((uint16_full_rotation_counter>7999||uint16_half_rotation_counter>7999))
        {
            i_q_control_state = 0;
		}

        //u_q_temp = u_q_target;
    }



    // testing - directly setting duty cycle
    /*if(DD.go)
    {
        MS.u_q = DD.ui16_value;
    }
    else
    {
        MS.u_q = 0;
    }*/
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
    
    if(ui8_UART_TxCplt_flag && (print_count >= 10) && ui8_pwm_enabled_flag)
    //if(ui8_UART_TxCplt_flag && (print_count >= 200))
    {
        //sprintf_(buffer, "%u %d %d %d\n", i_q_control_state, u_q_temp, battery_power, target_power);
        //debug_print(buffer, strlen(buffer));
        //print_count = 0;
    }

    ++print_count;
}

static void i_d_control()
{
    static uint8_t i_d_control_state = 0;       // idle

    static uint32_t print_count = 0;

    

    q31_t i_d = (MS.i_d >> 5) * 38; // i_d in mA
    q31_t i_q = (MS.i_q >> 5) * 38;

    q31_t alpha_temp = 0;
    //alpha_temp = q31_degree * DD.ui16_value2;
    
    if(enum_motor_error_state)
    {
        i_d_control_state = 0;
    }

    if(i_d_control_state == 0)
    {
        PI_id.integral_part = 0;
        alpha_temp = 0;

        if( (enum_hall_angle_state == PLL) && ui8_pwm_enabled_flag)
        {
            if(!enum_motor_error_state)
            {
                i_d_control_state = 1;
            }
        }
    }

    if(i_d_control_state == 1)
    {
        if( (enum_hall_angle_state == SIXSTEP) || !ui8_pwm_enabled_flag)
        {
            i_d_control_state = 0;
        }
        else
        {
            PI_id.setpoint = 0;
            PI_id.recent_value = i_d;
            alpha_temp = PI_control(&PI_id);
            alpha_temp = -alpha_temp;
        }
    }
    

    q31_t max_alpha =  Q31_DEGREE * 20;
    q31_t min_alpha =  -Q31_DEGREE * 20;


    if(alpha_temp > max_alpha)
    {
        alpha_temp = max_alpha;
    }
    if(alpha_temp < min_alpha)
    {
        alpha_temp = min_alpha;
    }
    
    if(ui8_UART_TxCplt_flag && (print_count >= 10) && ui8_pwm_enabled_flag)
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
    
    //else
    {
        i_q_control();
        i_d_control();
        //MS.foc_alpha = 0;
        //MS.u_d = 0;
        PI_flag = 0;
    }

    // todo: revise _U_MAX in FOC.h
    // also: check umax computation in original foc computation
}

/*void runPIcontrol2(){

    //if(ui8_UART_TxCplt_flag)
    //{
    //    sprintf_(buffer, "*%d\n", MS.i_q);
    //    debug_print(buffer, strlen(buffer));
    //}

		  q31_t_Battery_Current_accumulated -= q31_t_Battery_Current_accumulated>>8;
		  q31_t_Battery_Current_accumulated += ((MS.i_q*MS.u_abs)>>11)*(uint16_t)(CAL_I>>8);

		  MS.Battery_Current = (q31_t_Battery_Current_accumulated>>8)*i8_direction*i8_reverse_flag; //Battery current in mA
		  //Check battery current limit
		  if(MS.Battery_Current>BATTERYCURRENT_MAX) ui8_BC_limit_flag=1;
		  if(MS.Battery_Current<-REGEN_CURRENT_MAX) ui8_BC_limit_flag=1;
		  //reset battery current flag with small hysteresis
		  if(HAL_GPIO_ReadPin(Brake_GPIO_Port, Brake_Pin)){
			  if(((int32_current_target*MS.u_abs)>>11)*(uint16_t)(CAL_I>>8)<(BATTERYCURRENT_MAX*7)>>3)ui8_BC_limit_flag=0;
		  }
		  else{
			  if(((int32_current_target*MS.u_abs)>>11)*(uint16_t)(CAL_I>>8)>(-REGEN_CURRENT_MAX*7)>>3)ui8_BC_limit_flag=0;
		  }
    
		  //control iq

		  //if
		  if (!ui8_BC_limit_flag){
			  PI_iq.recent_value = MS.i_q;
			  PI_iq.setpoint = i8_direction*i8_reverse_flag*int32_current_target;
		  }
		  else{
			  if(HAL_GPIO_ReadPin(Brake_GPIO_Port, Brake_Pin)){
				  PI_iq.recent_value=  (MS.Battery_Current>>6)*i8_direction*i8_reverse_flag;
				  PI_iq.setpoint = (BATTERYCURRENT_MAX>>6)*i8_direction*i8_reverse_flag;
			  	}
			  else{
				  PI_iq.recent_value=  (MS.Battery_Current>>6)*i8_direction*i8_reverse_flag;
				  PI_iq.setpoint = (-BATTERYCURRENT_MAX>>6)*i8_direction*i8_reverse_flag;
			    }
		  }
		  q31_u_q_temp =  PI_control(&PI_iq);



		  //Control id
		  PI_id.recent_value = MS.i_d;
		  q31_u_d_temp = -PI_control(&PI_id); //control direct current to zero


		  	//limit voltage in rotating frame, refer chapter 4.10.1 of UM1052
		  //MS.u_abs = (q31_t)hypot((double)q31_u_d_temp, (double)q31_u_q_temp); //absolute value of U in static frame
			arm_sqrt_q31((q31_u_d_temp*q31_u_d_temp+q31_u_q_temp*q31_u_q_temp)<<1,&MS.u_abs);
			MS.u_abs = (MS.u_abs>>16)+1;


			if (MS.u_abs > _U_MAX){
				MS.u_q = (q31_u_q_temp*_U_MAX)/MS.u_abs; //division!
				MS.u_d = (q31_u_d_temp*_U_MAX)/MS.u_abs; //division!
				MS.u_abs = _U_MAX;
			}
			else{
				MS.u_q=q31_u_q_temp;
				MS.u_d=q31_u_d_temp;
			}
            
            if(DD.go)
            {
                MS.u_q = DD.ui16_value;
            }
            else
            {
                MS.u_q = 0;
            }
            MS.u_d = 0;


		  	PI_flag=0;
	  }*/

q31_t speed_PLL (q31_t ist, q31_t soll)
{
    q31_t q31_speed_pll_p = (soll - ist) >> P_FACTOR_PLL;   				//7 for Shengyi middrive, 10 for BionX IGH3
    q31_speed_pll_i += (soll - ist) >> I_FACTOR_PLL;				//11 for Shengyi middrive, 10 for BionX IGH3

    //clamp i part to twice the theoretical value from hall interrupts
    //if(q31_d_i>((DEG_plus60>>19)*500/ui16_timertics)<<16)q31_d_i=((DEG_plus60>>19)*500/ui16_timertics)<<16;
    //if(q31_d_i<-((DEG_plus60>>19)*500/ui16_timertics)<<16)q31_d_i=-((DEG_plus60>>19)*500/ui16_timertics)<<16;

    //if (!READ_BIT(TIM1->BDTR, TIM_BDTR_MOE))
    //{
    //    q31_speed_pll_i = 0;
    //}

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
