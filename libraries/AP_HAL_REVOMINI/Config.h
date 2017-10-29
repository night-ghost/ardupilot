// no includes from here! only defines in one place
#pragma once

#define DEBUG_BUILD 1

#define USART_SAFE_INSERT // ignore received bytes if buffer overflows

#define USE_WFE

//#define I2C_DEBUG
//#define DEBUG_SPI

#define  RC_DEAD_TIME 60000 // 60 seconds no data changes
#define REVOMINI_RC_INPUT_MIN_CHANNELS 4
#define REVOMINI_RC_INPUT_NUM_CHANNELS 20


#ifdef DEBUG_BUILD
// profiling
//#define ISR_PERF - now all time-consuming calculations moved out from ISR to io_completion level
//#define SEM_PROF - now semaphores are part of scheduler
#define SHED_PROF 
#define MTASK_PROF
//#define SHED_DEBUG


//#define SEM_DEBUG

//#define MPU_DEBUG

#endif

/*
interrupts priorities:
0 PWM input (10uS between interrupts)
1 soft_uart
2 i2c 
3 micros() Timer5 / MPU DataReady
4 timer_i2C
5 SysTick
6 uart
7 gpio pin
8 OSD VSI
9 dma IO complete
a
b usb
c driver's io_completion
d
e scheduler - Timer7, tail timer, svc
f Pend_Sw
*/

#define PWM_INT_PRIORITY       0
#define SOFT_UART_INT_PRIORITY 1
#define I2C_INT_PRIORITY       2 
#define MPU_INT_PRIORITY       3
#define TIMER_I2C_INT_PRIORITY 4
#define SYSTICK_INT_PRIORITY   5
#define UART_INT_PRIORITY      6
#define GPIO_INT_PRIORITY      7
#define VSI_INT_PRIORITY       8
#define DMA_IOC_INT_PRIORITY   9
#define USB_INT_PRIORITY      11
#define IOC_INT_PRIORITY      12
#define SVC_INT_PRIORITY      14
#define PENDSV_INT_PRIORITY   15
