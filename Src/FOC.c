/*
 * FOC.c
 *
 *  Created on: 25.01.2019
 *      Author: Stancecoke
 */
#include "main.h"
#include "config.h"
#include "FOC.h"
#include "stm32f1xx_hal.h"
#include <arm_math.h>

//q31_t	T_halfsample = 0.00003125;
//q31_t	counterfrequency = 64000000;
//q31_t	U_max = (1/_SQRT3)*_U_DC;
q31_t	temp1;
q31_t	temp2;
q31_t	temp3;
q31_t	temp4;
q31_t	temp5;
q31_t	temp6;

//q31_t q31_i_q_fil = 0;
//q31_t q31_i_d_fil = 0;

q31_t x1;
q31_t x2;
q31_t teta_obs;

#ifdef FAST_LOOP_LOG
q31_t e_log[300][6];
#endif

q31_t z;
char Obs_flag=1;
uint8_t ui8_debug_state=0;

char PI_flag=0;

//const q31_t _T = 2048;

TIM_HandleTypeDef htim1;


void FOC_calculation(int16_t int16_i_as, int16_t int16_i_bs, q31_t q31_teta, int16_t int16_i_q_target, MotorState_t* MS_FOC);
void svpwm(q31_t q31_u_alpha, q31_t q31_u_beta);

void FOC_calculation(int16_t int16_i_as, int16_t int16_i_bs, q31_t q31_teta, int16_t int16_i_q_target, MotorState_t* MS_FOC)
{

    static uint8_t ui8_foc_counter = 0;
    static q31_t q31_i_d_sum = 0;
    static q31_t q31_i_q_sum = 0;
    

	 q31_t q31_i_alpha = 0;
	 q31_t q31_i_beta = 0;
	 q31_t q31_u_alpha = 0;
	 q31_t q31_u_beta = 0;
	 q31_t q31_i_d = 0;
	 q31_t q31_i_q = 0;

	 q31_t sinevalue=0, cosinevalue = 0;


	// temp5=(q31_t)int16_i_as;
	// temp6=(q31_t)int16_i_bs;

	// Clark transformation
	arm_clarke_q31((q31_t)int16_i_as, (q31_t)int16_i_bs, &q31_i_alpha, &q31_i_beta);

	arm_sin_cos_q31(q31_teta, &sinevalue, &cosinevalue);


	// Park transformation
	arm_park_q31(q31_i_alpha, q31_i_beta, &q31_i_d, &q31_i_q, sinevalue, cosinevalue);

    q31_i_d_sum += q31_i_d;
    q31_i_q_sum += q31_i_q;

    if(ui8_foc_counter == 31)
    {
        MS_FOC->i_q = q31_i_d_sum;  //>> 5;
        MS_FOC->i_d = q31_i_q_sum;  //>> 5;
        q31_i_d_sum = 0;
        q31_i_q_sum = 0;
        ui8_foc_counter = 0;
        PI_flag = 1;
    }


	//q31_i_q_fil -= q31_i_q_fil>>4;
	//q31_i_q_fil += q31_i_q;
	//MS_FOC->i_q=q31_i_q_fil>>4;

	//q31_i_d_fil -= q31_i_d_fil>>4;
	//q31_i_d_fil += q31_i_d;
	//MS_FOC->i_d=q31_i_d_fil>>4;

	//Control iq

	//PI_flag=1;


//set static voltage for hall angle detection
	//if(!MS_FOC->hall_angle_detect_flag){
	//	MS_FOC->u_q=0;
	//	MS_FOC->u_d=200;
	//}
	//else
    //{
        //runPIcontrol();
    //}

	//inverse Park transformation
	arm_inv_park_q31(MS_FOC->u_d, MS_FOC->u_q, &q31_u_alpha, &q31_u_beta, -sinevalue, cosinevalue);



#ifdef FAST_LOOP_LOG
	temp1=int16_i_as;
	temp2=int16_i_bs;
	temp3=MS_FOC->i_d;
	temp4=MS_FOC->i_q;
	temp5=MS_FOC->u_d;
	temp6=MS_FOC->u_q;

	if(uint32_PAS_counter < PAS_TIMEOUT&&ui8_debug_state==0)
			{
		e_log[z][0]=temp1;//fl_e_alpha_obs;
		e_log[z][1]=temp2;//fl_e_beta_obs;
		e_log[z][2]=temp3;//(q31_t)q31_teta_obs>>24;
		e_log[z][3]=temp4;
		e_log[z][4]=temp5;
		e_log[z][5]=temp6;
		z++;
		if(z>150) Obs_flag=1;
		if (z>299)
		{z=0;

		ui8_debug_state=2;}
			}
	else {if(ui8_debug_state==2)ui8_debug_state=3;;}

#endif

	//call SVPWM calculation
	svpwm(q31_u_alpha, q31_u_beta);
	//temp6=__HAL_TIM_GET_COUNTER(&htim1);

    ++ui8_foc_counter;

}


q31_t PI_control (PI_control_t* PI_c)
{

    // comments:
    // proportional part is set to zero -> only integral control
    // the integration stops as soon as the max duty-cylce reached

    // maximum delta to expect (extreme case)
    // 800W -> delta = 8000
    // 8000 * I_FACTOR >> shift = 8000 * 20 >> 10 = 156


    q31_t q31_delta = PI_c->setpoint - PI_c->recent_value;

    //q31_t q31_p = q31_delta * PI_c->gain_p;
    q31_t q31_p = 0;
    q31_t q31_i = PI_c->integral_part * PI_c->gain_i;

    q31_t max_out_shifted = PI_c->limit_output_max << PI_c->shift;
    q31_t min_out_shifted = PI_c->limit_output_min << PI_c->shift;

    uint8_t integrate = 1;

    q31_t out_shifted = q31_p + q31_i;

    if(out_shifted >= max_out_shifted)
    {
        out_shifted = max_out_shifted;
        if(q31_delta > 0)
        {
            // saturation
            integrate = 0;
        }
    }

    if(out_shifted <= min_out_shifted)
    {
        out_shifted = min_out_shifted;
        if(q31_delta < 0)
        {
            // saturation
            integrate = 0;
        }
    }

    if(integrate)
    {
        PI_c->integral_part += q31_delta;
    }

    PI_c->out = out_shifted >> PI_c->shift;
    return PI_c->out;
}


void svpwm(q31_t q31_u_alpha, q31_t q31_u_beta)	{

//SVPWM according to chapter 4.9 of UM1052


	q31_t q31_U_alpha = (_SQRT3 *_T * q31_u_alpha)>>4;
	q31_t q31_U_beta = -_T * q31_u_beta;
	q31_t X = q31_U_beta;
	q31_t Y = (q31_U_alpha+q31_U_beta)>>1;
	q31_t Z = (q31_U_beta-q31_U_alpha)>>1;

	//Sector 1 & 4
	if ((Y>=0 && Z<0 && X>0)||(Y < 0 && Z>=0 && X<=0)){
		switchtime[0] = ((_T+X-Z)>>12) + (_T>>1); //right shift 11 for dividing by peroid (=2^11), right shift 1 for dividing by 2
		switchtime[1] = switchtime[0] + (Z>>11);
		switchtime[2] = switchtime[1] - (X>>11);
		//temp4=1;
	}

	//Sector 2 & 5
	if ((Y>=0 && Z>=0) || (Y<0 && Z<0) ){
		switchtime[0] = ((_T+Y-Z)>>12) + (_T>>1);
		switchtime[1] = switchtime[0] + (Z>>11);
		switchtime[2] = switchtime[0] - (Y>>11);
		//temp4=2;
	}

	//Sector 3 & 6
	if ((Y<0 && Z>=0 && X>0)||(Y >= 0 && Z<0 && X<=0)){
		switchtime[0] = ((_T+Y-X)>>12) + (_T>>1);
		switchtime[2] = switchtime[0] - (Y>>11);
		switchtime[1] = switchtime[2] + (X>>11);
		//temp4=3;
	}


}


