#ifndef _HAL_H_
#define _HAL_H_

#include <errno.h>

#include <stm32f4xx.h>

#ifndef INLINE
#define INLINE __attribute__ ((always_inline)) inline
#endif
#define WEAK __attribute__((weak))

#ifndef IN_CCM
#define IN_CCM  __attribute__((section(".ccm")))
#endif


#include "hal_types.h"
#include "stm32.h"
#include "stopwatch.h"
#include "util.h"
#include "gpio_hal.h"
#include "delay.h"
#include "adc.h"
#include "systick.h"
//#include "syscalls.h"


#define DEBUG_BUILD 1

//#define ISR_PROF


#define OK	1
#define ERROR	0

#define I2C_OK		0
#define I2C_NO_DEVICE	1
#define I2C_ERROR	2

#ifdef __cplusplus
  extern "C" {
#endif

extern void clock_gettime(uint32_t a1, void *a2);

extern void revo_call_handler(Handler h, uint32_t arg);

#define ADDRESS_IN_RAM(a) ((uint32_t)a >= 0x20000000)

#ifdef __cplusplus
  }
#endif

#ifdef ISR_PROF
    extern uint64_t isr_time;
#endif


union Revo_hal_handler { // кровь кишки ассемблер :) преобразование функторов в унифицированный вид
    voidFuncPtr vp;
//  AP_HAL::MemberProc mp;          это С а не С++ поэтому мы не можем объявить поддержку функторов явно, и вынуждены передавать
    uint64_t h; // treat as handle             <-- как 64-битное число
    uint32_t w[2]; // words, to check. если функтор то старшее - адрес флеша, младшее - адрес в RAM. Если ссылка на функцию то младшее - адрес флеша, старшее 0
};




#endif

