/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/**
 *  @brief Arduino-style PWM implementation.
 
see https://github.com/mahowik/MahoRotorF4-Discovery/blob/master/src/drv/drv_pwm_fy90q.c
 
 */

#include "pwm_in.h"
#include <stdbool.h>
#include "hal_types.h"
#include "timer.h"
#include <systick.h>

#include <boards.h>
#include "ring_buffer_pulse.h"

#define MINONWIDTH 920 * 2
#define MAXONWIDTH 2120 * 2
// PATCH FOR FAILSAFE AND FRSKY
#define MINOFFWIDTH 1000 * 2
#define MAXOFFWIDTH 22000 * 2


#define PPM_CHANNELS 2 // independent input pins


typedef void (*rcc_clockcmd)( uint32_t, FunctionalState);
/**************** PWM INPUT **************************************/

// Forward declaration
static inline void pwmIRQHandler(uint32_t v/*TIM_TypeDef *tim */);

static void pwmInitializeInput(uint8_t ppmsum);


extern const struct TIM_Channel PWM_Channels[];


struct PPM_State PPM_Inputs[PPM_CHANNELS] IN_CCM;
static uint16_t num_ppm_channels = 1;


static void pwmIRQHandler(uint32_t v /* TIM_TypeDef *tim */){
        TIM_TypeDef * tim = (TIM_TypeDef *)v;
        uint8_t i;
        uint16_t val = 0;

	for (i = 0; i < num_ppm_channels; i++)  {

            const struct TIM_Channel *channel = &PWM_Channels[i]; 
	    struct PPM_State         *input   = &PPM_Inputs[i];

/*
struct PPM_State  {
    uint8_t state;          // 1 or 0
    uint16_t last_val;      // length 
    uint32_t last_pulse;    // time of edge
    volatile pulse_buffer pulses;   // ring buffer
    Pulse pulse_mem[PULSES_QUEUE_SIZE]; // memory
};
*/
	    if (channel->tim == tim && (TIM_GetITStatus(tim, channel->tim_cc) == SET)) {

                switch (channel->tim_channel)   {
	        case TIM_Channel_1:
	            val = TIM_GetCapture1(channel->tim);
	            break;
	        case TIM_Channel_2:
	            val = TIM_GetCapture2(channel->tim);
	            break;
	        case TIM_Channel_3:
	            val = TIM_GetCapture3(channel->tim);
	            break;
	        case TIM_Channel_4:
	            val = TIM_GetCapture4(channel->tim);
	            break;
	        }


	        input->last_pulse = systick_uptime();
        

                TIM_ICInitTypeDef TIM_ICInitStructure;

                TIM_ICInitStructure.TIM_Channel = channel->tim_channel;
                TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
                TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	        TIM_ICInitStructure.TIM_ICFilter = 0x0;

                uint16_t time;

                if (val > input->last_val)  {
                    time = val - input->last_val;
                } else  {
                    time = ((0xFFFF - input->last_val) + val)+1;
                }
                input->last_val = val;

                if(time>0x7fff) time=0x7fff; // limit to 15 bit

                Pulse p={
                    .length  = time, 
                    .state = input->state
                };

                if(!pb_is_full(&input->pulses)){ // save pulse length and state 
                    pb_insert(&input->pulses, p);
                }


                if (input->state == 0) { // rising edge
	            input->state = 1;

	            TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling; // reprogram timer to falling
//	            timer_cc_set_pol(channel->timer, channel->channel_n, 1);
	        } else  {               // falling edge
	            input->state = 0;
	
	            TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising; // reprogram timer to raising
//	            timer_cc_set_pol(channel->timer, channel->channel_n, 0);
	        }
	        TIM_ICInit(channel->tim, &TIM_ICInitStructure);
                
                if(input->handler) revo_call_handler(input->handler, 0); // call callback on each edge
	    }
	}
}

static inline void pwmInitializeInput(uint8_t ppmsum){
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;

    { // ppm mode
        uint8_t i;
        TIM_TypeDef * last_tim=0;


	for (i = 0; i < num_ppm_channels; i++)   {
            const struct TIM_Channel *channel = &PWM_Channels[i];
	
            TIM_Cmd(channel->tim, DISABLE);

            NVIC_EnableIRQ(channel->tim_irq);
            NVIC_SetPriority(channel->tim_irq, 0); // highest - bit time is ~10uS only - ~1680 commands	

	    if(last_tim != channel->tim) {
	        Revo_hal_handler h = { .isr = pwmIRQHandler };
                timer_attach_all_interrupts(channel->timer, h.h); 

                // timer_reset ******************************************************************
                channel->tim_clkcmd(channel->tim_clk, ENABLE);

                channel->tim->CR1 = TIMER_CR1_ARPE;
                channel->tim->PSC = 1;
                channel->tim->SR = 0;
                channel->tim->DIER = 0;
                channel->tim->EGR = TIMER_EGR_UG;

	        // TIM configuration base *******************************************************
	        TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

                if (channel->tim == TIM1 || channel->tim == TIM8 || channel->tim == TIM9 || channel->tim == TIM10 || channel->tim == TIM11){
	            TIM_TimeBaseStructure.TIM_Prescaler = 84-1; //2MHz
	        }else{
	            TIM_TimeBaseStructure.TIM_Prescaler = 42-1; //2MHz
	        }
                TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
	        TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	        TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	        TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	        TIM_TimeBaseInit(channel->tim, &TIM_TimeBaseStructure);
	        
	        last_tim = channel->tim;
            }
            
	    // PWM input capture ************************************************************
	    TIM_ICInitStructure.TIM_Channel = channel->tim_channel;
	    TIM_ICInitStructure.TIM_ICPolarity  = TIM_ICPolarity_Falling;
	    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	    TIM_ICInitStructure.TIM_ICFilter = 0x0;
	    TIM_ICInit(channel->tim, &TIM_ICInitStructure);

	    // timer_enable *****************************************************************
	    TIM_Cmd(channel->tim, ENABLE);

	    // enable the CC interrupt request **********************************************
	    TIM_ITConfig(channel->tim, channel->tim_cc, ENABLE);

	    // gpio_set_mode ****************************************************************
	    channel->gpio_clkcmd(channel->gpio_clk, ENABLE);

	    GPIO_InitStructure.GPIO_Pin = channel->gpio_pin;
	    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	    GPIO_Init(channel->gpio_port, &GPIO_InitStructure);
	
	    // connect pin to timer -  gpio_set_af_mode *************************************************************
	    GPIO_PinAFConfig(channel->gpio_port, channel->gpio_af, channel->gpio_af_tim);
        }
    }
}

void pwmInit(bool ppmsum) {
    uint8_t i;

    memset(PPM_Inputs, 0, sizeof(PPM_Inputs));

    for (i = 0; i < PPM_CHANNELS; i++){
        struct PPM_State   *input = &PPM_Inputs[i];
	input->state = 1;
        pb_init(&input->pulses, PULSES_QUEUE_SIZE, input->pulse_mem); // init ring buffer
    }

// TODO use parameters!
    num_ppm_channels=PPM_CHANNELS;

    pwmInitializeInput(ppmsum);
}


bool getPPM_Pulse(Pulse *p, uint8_t ch) {
    if(ch>PPM_CHANNELS) return false;
    
    volatile pulse_buffer *bp = &PPM_Inputs[ch].pulses;
    if(pb_is_empty(bp)) return false;

    *p = pb_remove(bp);
    return true;
}

uint16_t getPPM_count(uint8_t ch){
    volatile pulse_buffer *bp = &PPM_Inputs[ch].pulses;
    return pb_full_count(bp);
}