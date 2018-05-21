#ifndef _RCC_H
#define _RCC_H

#include "hal_types.h"
#include "bitband.h"

#ifdef __cplusplus
  extern "C" {
#endif


#define RCC_RTCCLKSource_LSI             ((uint32_t)1<<9)

#define RCC_HSE_OFF                      ((uint8_t)0x00)
#define RCC_HSE_ON                       ((uint8_t)0x01)

#define RCC_BIT_HSE_RDY                  (1<<17)

#define RCC_APB1_bit_TIM2              ((uint32_t)1<<0)
#define RCC_APB1_bit_TIM3              ((uint32_t)1<<1)
#define RCC_APB1_bit_TIM4              ((uint32_t)1<<2)
#define RCC_APB1_bit_TIM5              ((uint32_t)1<<3)
#define RCC_APB1_bit_TIM6              ((uint32_t)1<<4)
#define RCC_APB1_bit_TIM7              ((uint32_t)1<<5)
#define RCC_APB1_bit_TIM12             ((uint32_t)1<<6)
#define RCC_APB1_bit_TIM13             ((uint32_t)1<<7)
#define RCC_APB1_bit_TIM14             ((uint32_t)1<<8)
// 9..10 reserved
#define RCC_APB1_bit_WWDG              ((uint32_t)1<<11)
// 12..13 reserved
#define RCC_APB1_bit_SPI2              ((uint32_t)1<<14)
#define RCC_APB1_bit_SPI3              ((uint32_t)1<<15)
// 16 reserved
#define RCC_APB1_bit_USART2            ((uint32_t)1<<17)
#define RCC_APB1_bit_USART3            ((uint32_t)1<<18)
#define RCC_APB1_bit_UART4             ((uint32_t)1<<19)
#define RCC_APB1_bit_UART5             ((uint32_t)1<<20)
#define RCC_APB1_bit_I2C1              ((uint32_t)1<<21)
#define RCC_APB1_bit_I2C2              ((uint32_t)1<<22)
#define RCC_APB1_bit_I2C3              ((uint32_t)1<<23)
// 24 reserved
#define RCC_APB1_bit_CAN1              ((uint32_t)1<<25)
#define RCC_APB1_bit_CAN2              ((uint32_t)1<<26)
// 27 reserved
#define RCC_APB1_bit_PWR               ((uint32_t)1<<28)
#define RCC_APB1_bit_DAC               ((uint32_t)1<<29)
#define RCC_APB1_bit_UART7             ((uint32_t)1<<30)
#define RCC_APB1_bit_UART8             ((uint32_t)1<<31)

#define RCC_APB2_bit_TIM1              ((uint32_t)1<<0)
#define RCC_APB2_bit_TIM8              ((uint32_t)1<<1)
//2..3 reserved
#define RCC_APB2_bit_USART1            ((uint32_t)1<<4)
#define RCC_APB2_bit_USART6            ((uint32_t)1<<5)
//6..7 reserved
#define RCC_APB2_bit_ADC1              ((uint32_t)1<<8)
#define RCC_APB2_bit_ADC2              ((uint32_t)1<<9)
#define RCC_APB2_bit_ADC3              ((uint32_t)1<<10)
#define RCC_APB2_bit_ADC               (RCC_APB2_bit_ADC1 | RCC_APB2_bit_ADC2 | RCC_APB2_bit_ADC3)
#define RCC_APB2_bit_SDIO              ((uint32_t)1<<11)
#define RCC_APB2_bit_SPI1              ((uint32_t)1<<12)
#define RCC_APB2_bit_SPI4              ((uint32_t)1<<13)
#define RCC_APB2_bit_SYSCFG            ((uint32_t)1<<14)
// 15 reserved
#define RCC_APB2_bit_TIM9              ((uint32_t)1<<16)
#define RCC_APB2_bit_TIM10             ((uint32_t)1<<17)
#define RCC_APB2_bit_TIM11             ((uint32_t)1<<18)
// 19 reserved
#define RCC_APB2_bit_SPI5              ((uint32_t)1<<20)
#define RCC_APB2_bit_SPI6              ((uint32_t)1<<21)

#define RCC_AHB1_bit_GPIOA             ((uint32_t)1<<0)
#define RCC_AHB1_bit_GPIOB             ((uint32_t)1<<1)
#define RCC_AHB1_bit_GPIOC             ((uint32_t)1<<2)
#define RCC_AHB1_bit_GPIOD             ((uint32_t)1<<3)
#define RCC_AHB1_bit_GPIOE             ((uint32_t)1<<4)
#define RCC_AHB1_bit_GPIOF             ((uint32_t)1<<5)
#define RCC_AHB1_bit_GPIOG             ((uint32_t)1<<6)
#define RCC_AHB1_bit_GPIOH             ((uint32_t)1<<7)
#define RCC_AHB1_bit_GPIOI             ((uint32_t)1<<8)
// 9..11 reserved
#define RCC_AHB1_bit_CRC               ((uint32_t)1<<12)
//13..14 reserved
#define RCC_AHB1_bit_FLITF             ((uint32_t)1<<15)
#define RCC_AHB1_bit_SRAM1             ((uint32_t)1<<16)
#define RCC_AHB1_bit_SRAM2             ((uint32_t)1<<17)
#define RCC_AHB1_bit_BKPSRAM           ((uint32_t)1<<18)
#define RCC_AHB1_bit_SRAM3             ((uint32_t)1<<19)
#define RCC_AHB1_bit_CCMDATARAMEN      ((uint32_t)1<<20)
#define RCC_AHB1_bit_DMA1              ((uint32_t)1<<21)
#define RCC_AHB1_bit_DMA2              ((uint32_t)1<<22)
//23..24 reserved
#define RCC_AHB1_bit_ETH_MAC           ((uint32_t)1<<25)
#define RCC_AHB1_bit_ETH_MAC_Tx        ((uint32_t)1<<26)
#define RCC_AHB1_bit_ETH_MAC_Rx        ((uint32_t)1<<27)
#define RCC_AHB1_bit_ETH_MAC_PTP       ((uint32_t)1<<28)
#define RCC_AHB1_bit_OTG_HS            ((uint32_t)1<<29)
#define RCC_AHB1_bit_OTG_HS_ULPI       ((uint32_t)1<<30)

#define RCC_AHB2_bit_DCMI              ((uint32_t)1<<0)
#define RCC_AHB2_bit_CRYP              ((uint32_t)1<<4)
#define RCC_AHB2_bit_HASH              ((uint32_t)1<<5)
#define RCC_AHB2_bit_RNG               ((uint32_t)1<<6)
#define RCC_AHB2_bit_OTG_FS            ((uint32_t)1<<7)


typedef struct {
  uint32_t HCLK_Frequency;   //  HCLK  clock frequency, Hz  
  uint32_t PCLK1_Frequency;  //  PCLK1 clock frequency, Hz 
  uint32_t PCLK2_Frequency;  //  PCLK2 clock frequency, Hz 
} RCC_Clocks_t;


void RCC_configRTC(uint32_t RCC_RTCCLKSource);
bool RCC_WaitForHSEStartUp(void);
bool RCC_GetFlagStatus(uint8_t RCC_FLAG);
void RCC_enableHSE(uint8_t hse);

void RCC_GetClocksFreq(RCC_Clocks_t* RCC_Clocks);

void RCC_doAPB1_reset(uint32_t dev_bit);
void RCC_doAPB2_reset(uint32_t dev_bit);
void RCC_doAHB1_reset(uint32_t dev_bit);


static inline void RCC_enableRTCclk(bool enable){  *bb_perip(&(RCC->BDCR), 15) = enable?1:0; }

static inline void RCC_enableAHB1_clk(uint32_t dev_bit) {    RCC->AHB1ENR |= dev_bit; }
static inline void RCC_enableAHB2_clk(uint32_t dev_bit) {    RCC->AHB2ENR |= dev_bit; }
static inline void RCC_enableAPB1_clk(uint32_t dev_bit) {    RCC->APB1ENR |= dev_bit; }
static inline void RCC_enableAPB2_clk(uint32_t dev_bit) {    RCC->APB2ENR |= dev_bit; }

static inline void RCC_disableAHB2_clk(uint32_t dev_bit){    RCC->AHB2ENR &= ~dev_bit;}


#ifdef __cplusplus
  }
#endif


#endif

