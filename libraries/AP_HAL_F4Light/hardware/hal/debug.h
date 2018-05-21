#pragma once

//DBG_CR
#define DBGMCU_SLEEP                 ((uint32_t)1<<0)
#define DBGMCU_STOP                  ((uint32_t)1<<1)
#define DBGMCU_STANDBY               ((uint32_t)1<<2)

//DBG_APB1_FZ
#define DBGMCU_TIM2_STOP             ((uint32_t)1<<0)
#define DBGMCU_TIM3_STOP             ((uint32_t)1<<1)
#define DBGMCU_TIM4_STOP             ((uint32_t)1<<2)
#define DBGMCU_TIM5_STOP             ((uint32_t)1<<3)
#define DBGMCU_TIM6_STOP             ((uint32_t)1<<4)
#define DBGMCU_TIM7_STOP             ((uint32_t)1<<5)
#define DBGMCU_TIM12_STOP            ((uint32_t)1<<6)
#define DBGMCU_TIM13_STOP            ((uint32_t)1<<7)
#define DBGMCU_TIM14_STOP            ((uint32_t)1<<8)
#define DBGMCU_RTC_STOP              ((uint32_t)1<<9)
// 10 reserved
#define DBGMCU_WWDG_STOP             ((uint32_t)1<<11)
#define DBGMCU_IWDG_STOP             ((uint32_t)1<<12)
//13..20 reserved
#define DBGMCU_I2C1_SMBUS_TIMEOUT    ((uint32_t)1<<21)
#define DBGMCU_I2C2_SMBUS_TIMEOUT    ((uint32_t)1<<22)
#define DBGMCU_I2C3_SMBUS_TIMEOUT    ((uint32_t)1<<23)
//24 reserved
#define DBGMCU_CAN1_STOP             ((uint32_t)1<<25)
#define DBGMCU_CAN2_STOP             ((uint32_t)1<<26)

//DBG_APB2_FZ
#define DBGMCU_TIM1_STOP             ((uint32_t)1<<0)
#define DBGMCU_TIM8_STOP             ((uint32_t)1<<1)
#define DBGMCU_TIM9_STOP             ((uint32_t)1<<16)
#define DBGMCU_TIM10_STOP            ((uint32_t)1<<17)
#define DBGMCU_TIM11_STOP            ((uint32_t)1<<18)
