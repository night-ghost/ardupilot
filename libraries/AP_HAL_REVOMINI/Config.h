// no includes from here! only defines in one place
#pragma once

#define DEBUG_BUILD 1

#define USART_SAFE_INSERT // ignore received bytes if buffer overflows

//#define I2C_DEBUG
//#define DEBUG_SPI

#define  RC_DEAD_TIME 60000 // 60 seconds no data changes
#define REVOMINI_RC_INPUT_MIN_CHANNELS 4
#define REVOMINI_RC_INPUT_NUM_CHANNELS 20

// profiling
//#define ISR_PERF - now all time-consuming calculations moved out from ISR to io_completion level
//#define SEM_PROF 
#define SHED_PROF 
#define MTASK_PROF
//#define SHED_DEBUG

#define USE_WFE

//#define MPU_DEBUG
