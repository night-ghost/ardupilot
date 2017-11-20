/**
  ******************************************************************************
  * @file    stm32f4xx_dbgmcu.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    11-January-2013
  * @brief   This file contains all the functions prototypes for the DBGMCU firmware library.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4xx_DBGMCU_H
#define __STM32F4xx_DBGMCU_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

/** @addtogroup STM32F4xx_StdPeriph_Driver
  * @{
  */

/** @addtogroup DBGMCU
  * @{
  */ 

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** @defgroup DBGMCU_Exported_Constants
  * @{
  */ 
#define DBGMCU_SLEEP                 ((uint32_t)0x00000001)
#define DBGMCU_STOP                  ((uint32_t)0x00000002)
#define DBGMCU_STANDBY               ((uint32_t)0x00000004)
#define IS_DBGMCU_PERIPH(PERIPH) ((((PERIPH) & 0xFFFFFFF8) == 0x00) && ((PERIPH) != 0x00))

#define DBGMCU_TIM2_STOP             ((uint32_t)0x00000001)
#define DBGMCU_TIM3_STOP             ((uint32_t)0x00000002)
#define DBGMCU_TIM4_STOP             ((uint32_t)0x00000004)
#define DBGMCU_TIM5_STOP             ((uint32_t)0x00000008)
#define DBGMCU_TIM6_STOP             ((uint32_t)0x00000010)
#define DBGMCU_TIM7_STOP             ((uint32_t)0x00000020)
#define DBGMCU_TIM12_STOP            ((uint32_t)0x00000040)
#define DBGMCU_TIM13_STOP            ((uint32_t)0x00000080)
#define DBGMCU_TIM14_STOP            ((uint32_t)0x00000100)
#define DBGMCU_RTC_STOP              ((uint32_t)0x00000400)
#define DBGMCU_WWDG_STOP             ((uint32_t)0x00000800)
#define DBGMCU_IWDG_STOP             ((uint32_t)0x00001000)
#define DBGMCU_I2C1_SMBUS_TIMEOUT    ((uint32_t)0x00200000)
#define DBGMCU_I2C2_SMBUS_TIMEOUT    ((uint32_t)0x00400000)
#define DBGMCU_I2C3_SMBUS_TIMEOUT    ((uint32_t)0x00800000)
#define DBGMCU_CAN1_STOP             ((uint32_t)0x02000000)
#define DBGMCU_CAN2_STOP             ((uint32_t)0x04000000)
#define IS_DBGMCU_APB1PERIPH(PERIPH) ((((PERIPH) & 0xF91FE200) == 0x00) && ((PERIPH) != 0x00))

#define DBGMCU_TIM1_STOP             ((uint32_t)0x00000001)
#define DBGMCU_TIM8_STOP             ((uint32_t)0x00000002)
#define DBGMCU_TIM9_STOP             ((uint32_t)0x00010000)
#define DBGMCU_TIM10_STOP            ((uint32_t)0x00020000)
#define DBGMCU_TIM11_STOP            ((uint32_t)0x00040000)
#define IS_DBGMCU_APB2PERIPH(PERIPH) ((((PERIPH) & 0xFFF8FFFC) == 0x00) && ((PERIPH) != 0x00))
/**
  * @}
  */ 



/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


/* ************ from .c **************/


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define IDCODE_DEVID_MASK    ((uint32_t)0x00000FFF)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup DBGMCU_Private_Functions
  * @{
  */ 

/**
  * @brief  Returns the device revision identifier.
  * @param  None
  * @retval Device revision identifier
  */
static inline uint32_t DBGMCU_GetREVID(void)
{
   return(DBGMCU->IDCODE >> 16);
}

/**
  * @brief  Returns the device identifier.
  * @param  None
  * @retval Device identifier
  */
static inline uint32_t DBGMCU_GetDEVID(void)
{
   return(DBGMCU->IDCODE & IDCODE_DEVID_MASK);
}

/**
  * @brief  Configures low power mode behavior when the MCU is in Debug mode.
  * @param  DBGMCU_Periph: specifies the low power mode.
  *   This parameter can be any combination of the following values:
  *     @arg DBGMCU_SLEEP: Keep debugger connection during SLEEP mode              
  *     @arg DBGMCU_STOP: Keep debugger connection during STOP mode               
  *     @arg DBGMCU_STANDBY: Keep debugger connection during STANDBY mode        
  * @param  NewState: new state of the specified low power mode in Debug mode.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
static inline void DBGMCU_Config(uint32_t DBGMCU_Periph, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_DBGMCU_PERIPH(DBGMCU_Periph));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {
    DBGMCU->CR |= DBGMCU_Periph;
  }
  else
  {
    DBGMCU->CR &= ~DBGMCU_Periph;
  }
}

/**
  * @brief  Configures APB1 peripheral behavior when the MCU is in Debug mode.
  * @param  DBGMCU_Periph: specifies the APB1 peripheral.
  *   This parameter can be any combination of the following values:        
  *     @arg DBGMCU_TIM2_STOP: TIM2 counter stopped when Core is halted          
  *     @arg DBGMCU_TIM3_STOP: TIM3 counter stopped when Core is halted          
  *     @arg DBGMCU_TIM4_STOP: TIM4 counter stopped when Core is halted
  *     @arg DBGMCU_TIM5_STOP: TIM5 counter stopped when Core is halted          
  *     @arg DBGMCU_TIM6_STOP: TIM6 counter stopped when Core is halted          
  *     @arg DBGMCU_TIM7_STOP: TIM7 counter stopped when Core is halted
  *     @arg DBGMCU_TIM12_STOP: TIM12 counter stopped when Core is halted  
  *     @arg DBGMCU_TIM13_STOP: TIM13 counter stopped when Core is halted  
  *     @arg DBGMCU_TIM14_STOP: TIM14 counter stopped when Core is halted 
  *     @arg DBGMCU_RTC_STOP: RTC Calendar and Wakeup counter stopped when Core is halted.                                                                                
  *     @arg DBGMCU_WWDG_STOP: Debug WWDG stopped when Core is halted
  *     @arg DBGMCU_IWDG_STOP: Debug IWDG stopped when Core is halted        
  *     @arg DBGMCU_I2C1_SMBUS_TIMEOUT: I2C1 SMBUS timeout mode stopped when Core is halted
  *     @arg DBGMCU_I2C2_SMBUS_TIMEOUT: I2C2 SMBUS timeout mode stopped when Core is halted
  *     @arg DBGMCU_I2C3_SMBUS_TIMEOUT: I2C3 SMBUS timeout mode stopped when Core is halted
  *     @arg DBGMCU_CAN2_STOP: Debug CAN1 stopped when Core is halted           
  *     @arg DBGMCU_CAN1_STOP: Debug CAN2 stopped when Core is halted        
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
static inline void DBGMCU_APB1PeriphConfig(uint32_t DBGMCU_Periph, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_DBGMCU_APB1PERIPH(DBGMCU_Periph));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    DBGMCU->APB1FZ |= DBGMCU_Periph;
  }
  else
  {
    DBGMCU->APB1FZ &= ~DBGMCU_Periph;
  }
}

/**
  * @brief  Configures APB2 peripheral behavior when the MCU is in Debug mode.
  * @param  DBGMCU_Periph: specifies the APB2 peripheral.
  *   This parameter can be any combination of the following values:       
  *     @arg DBGMCU_TIM1_STOP: TIM1 counter stopped when Core is halted                
  *     @arg DBGMCU_TIM8_STOP: TIM8 counter stopped when Core is halted
  *     @arg DBGMCU_TIM9_STOP: TIM9 counter stopped when Core is halted   
  *     @arg DBGMCU_TIM10_STOP: TIM10 counter stopped when Core is halted   
  *     @arg DBGMCU_TIM11_STOP: TIM11 counter stopped when Core is halted                                                                                  
  * @param  NewState: new state of the specified peripheral in Debug mode.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
static inline void DBGMCU_APB2PeriphConfig(uint32_t DBGMCU_Periph, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_DBGMCU_APB2PERIPH(DBGMCU_Periph));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    DBGMCU->APB2FZ |= DBGMCU_Periph;
  }
  else
  {
    DBGMCU->APB2FZ &= ~DBGMCU_Periph;
  }
}

/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */ 

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_DBGMCU_H */
