// no includes from here! only defines in one place
#pragma once

#define DEBUG_BUILD 1

#define USART_SAFE_INSERT // ignore received bytes if buffer overflows

#define USE_WFE


#define  RC_DEAD_TIME 60000 // 60 seconds no data changes
#define REVOMINI_RC_INPUT_MIN_CHANNELS 4
#define REVOMINI_RC_INPUT_NUM_CHANNELS 20

#define USE_MPU // guard page in process stack



#ifdef DEBUG_BUILD
// profiling
//#define ISR_PERF - now all time-consuming calculations moved out from ISR to io_completion level
//#define SEM_PROF - now semaphores are part of scheduler
#define SHED_PROF 
#define MTASK_PROF

//#define SHED_DEBUG
//#define SEM_DEBUG
//#define MPU_DEBUG
//#define I2C_DEBUG
//#define DEBUG_SPI

#endif

/*
 interrupts priorities:
*/

#define PWM_INT_PRIORITY       0 // PWM input (10uS between interrupts)
#define SOFT_UART_INT_PRIORITY 1 // soft_uart
#define I2C_INT_PRIORITY       2 // i2c 
#define MPU_INT_PRIORITY       3 // micros() Timer5 / MPU DataReady
#define TIMER_I2C_INT_PRIORITY 4 // timer_i2C (2uS between interrupts)
#define SYSTICK_INT_PRIORITY   5 // SysTick
#define UART_INT_PRIORITY      6 // uart
#define GPIO_INT_PRIORITY      7 // gpio pin
#define VSI_INT_PRIORITY       8 // OSD VSI
#define DMA_IOC_INT_PRIORITY   9 // dma IO complete
//                            10
#define USB_INT_PRIORITY      11 // usb
#define IOC_INT_PRIORITY      12 // driver's io_completion
//                            13
#define SVC_INT_PRIORITY      14 // scheduler - Timer7, tail timer, svc
#define PENDSV_INT_PRIORITY   15 // Pend_Sw


#define SPI_INT_PRIORITY I2C_INT_PRIORITY