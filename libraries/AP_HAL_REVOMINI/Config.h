// no includes from here! only defines in one place
#pragma once

#define DEBUG_BUILD 1


//#define I2C_DEBUG
//#define DEBUG_SPI

#define  RC_DEAD_TIME 60000 // 60 seconds no data changes

// profiling
//#define ISR_PERF - we moves out all time-consuming calculations from ISR to io_completion level
#define SEM_PROF 
#define SHED_PROF 
#define MTASK_PROF
