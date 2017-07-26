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
 *  @file pwm.h
 *
 *  @brief Arduino-compatible PWM interface.
 */

#ifndef _PWM_IN_H_
#define _PWM_IN_H_

#include "hal_types.h"
#include <stdbool.h>
#include <strings.h>
#include "timer.h"
#include "hal.h"

typedef void (*rcc_clockcmd)(uint32_t, FunctionalState);

#ifdef __cplusplus
  extern "C" {
#endif

typedef struct PULSE {
    uint16_t length;
    bool state;
} Pulse;

#define PULSES_QUEUE_SIZE (25*12*2*2) // 2 full frames by 25 bytes (12 bits, 2 measures per bit) each

#include "ring_buffer_pulse.h"


#ifdef PWM_SUPPORTED
struct PWM_State  {
    uint8_t state;
    uint16_t lower; //rise;
    uint16_t upper; //fall;
    uint16_t last_val;
    uint16_t capture;
    uint16_t error;
    uint32_t last_pulse;
};

extern struct PWM_State Inputs[];
extern bool _is_ppmsum;


static inline uint16_t pwmRead(uint8_t channel, uint32_t *time){
    if(time) *time=Inputs[channel].last_pulse;
    return Inputs[channel].capture;
}
#endif

struct PPM_State  {
    uint8_t state;          // 1 or 0
    uint16_t last_val;      // length 
    uint32_t last_pulse;    // time of edge
    volatile pulse_buffer pulses;   // ring buffer
    Handler handler;
    Pulse pulse_mem[PULSES_QUEUE_SIZE]; // memory
};

extern struct PPM_State PPM_Inputs[];


struct TIM_Channel {
        TIM_TypeDef * tim;
        uint32_t tim_clk;
        rcc_clockcmd tim_clkcmd;
        IRQn_Type tim_irq;

        uint16_t tim_channel;
        uint16_t tim_cc;

        GPIO_TypeDef * gpio_port;
        uint32_t gpio_clk;
        rcc_clockcmd gpio_clkcmd;

        uint16_t gpio_pin;
        uint8_t  gpio_af;
        uint8_t  gpio_af_tim;
        const timer_dev * timer;
        uint8_t channel_n; // for work with Timer driver
}; 

extern const struct TIM_Channel PWM_Channels[];
#define PWM_CHANNELS (sizeof(PWM_Channels) / sizeof(struct TIM_Channel) )
 

void pwmInit(bool ppmsum);

bool getPPM_Pulse(Pulse *p, uint8_t ch);

static inline void pwm_setHandler(Handler handler, uint8_t ch){
    PPM_Inputs[ch].handler = handler;
}

#ifdef __cplusplus
  }
#endif

#endif

