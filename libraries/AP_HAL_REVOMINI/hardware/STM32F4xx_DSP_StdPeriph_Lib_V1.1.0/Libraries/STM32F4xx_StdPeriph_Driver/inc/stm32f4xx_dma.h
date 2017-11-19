/**
  ******************************************************************************
  * @file    stm32f4xx_dma.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    11-January-2013
  * @brief   This file contains all the functions prototypes for the DMA firmware 
  *          library.
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
#ifndef __STM32F4xx_DMA_H
#define __STM32F4xx_DMA_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

/** @addtogroup STM32F4xx_StdPeriph_Driver
  * @{
  */

/** @addtogroup DMA
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** 
  * @brief  DMA Init structure definition
  */

typedef struct
{
  uint32_t DMA_Channel;            /*!< Specifies the channel used for the specified stream. 
                                        This parameter can be a value of @ref DMA_channel */
 
  uint32_t DMA_PeripheralBaseAddr; /*!< Specifies the peripheral base address for DMAy Streamx. */

  uint32_t DMA_Memory0BaseAddr;    /*!< Specifies the memory 0 base address for DMAy Streamx. 
                                        This memory is the default memory used when double buffer mode is
                                        not enabled. */

  uint32_t DMA_DIR;                /*!< Specifies if the data will be transferred from memory to peripheral, 
                                        from memory to memory or from peripheral to memory.
                                        This parameter can be a value of @ref DMA_data_transfer_direction */

  uint32_t DMA_BufferSize;         /*!< Specifies the buffer size, in data unit, of the specified Stream. 
                                        The data unit is equal to the configuration set in DMA_PeripheralDataSize
                                        or DMA_MemoryDataSize members depending in the transfer direction. */

  uint32_t DMA_PeripheralInc;      /*!< Specifies whether the Peripheral address register should be incremented or not.
                                        This parameter can be a value of @ref DMA_peripheral_incremented_mode */

  uint32_t DMA_MemoryInc;          /*!< Specifies whether the memory address register should be incremented or not.
                                        This parameter can be a value of @ref DMA_memory_incremented_mode */

  uint32_t DMA_PeripheralDataSize; /*!< Specifies the Peripheral data width.
                                        This parameter can be a value of @ref DMA_peripheral_data_size */

  uint32_t DMA_MemoryDataSize;     /*!< Specifies the Memory data width.
                                        This parameter can be a value of @ref DMA_memory_data_size */

  uint32_t DMA_Mode;               /*!< Specifies the operation mode of the DMAy Streamx.
                                        This parameter can be a value of @ref DMA_circular_normal_mode
                                        @note The circular buffer mode cannot be used if the memory-to-memory
                                              data transfer is configured on the selected Stream */

  uint32_t DMA_Priority;           /*!< Specifies the software priority for the DMAy Streamx.
                                        This parameter can be a value of @ref DMA_priority_level */

  uint32_t DMA_FIFOMode;          /*!< Specifies if the FIFO mode or Direct mode will be used for the specified Stream.
                                        This parameter can be a value of @ref DMA_fifo_direct_mode
                                        @note The Direct mode (FIFO mode disabled) cannot be used if the 
                                               memory-to-memory data transfer is configured on the selected Stream */

  uint32_t DMA_FIFOThreshold;      /*!< Specifies the FIFO threshold level.
                                        This parameter can be a value of @ref DMA_fifo_threshold_level */

  uint32_t DMA_MemoryBurst;        /*!< Specifies the Burst transfer configuration for the memory transfers. 
                                        It specifies the amount of data to be transferred in a single non interruptable 
                                        transaction. This parameter can be a value of @ref DMA_memory_burst 
                                        @note The burst mode is possible only if the address Increment mode is enabled. */

  uint32_t DMA_PeripheralBurst;    /*!< Specifies the Burst transfer configuration for the peripheral transfers. 
                                        It specifies the amount of data to be transferred in a single non interruptable 
                                        transaction. This parameter can be a value of @ref DMA_peripheral_burst
                                        @note The burst mode is possible only if the address Increment mode is enabled. */  
}DMA_InitTypeDef;

/* Exported constants --------------------------------------------------------*/

/** @defgroup DMA_Exported_Constants
  * @{
  */

#define IS_DMA_ALL_PERIPH(PERIPH) (((PERIPH) == DMA1_Stream0) || \
                                   ((PERIPH) == DMA1_Stream1) || \
                                   ((PERIPH) == DMA1_Stream2) || \
                                   ((PERIPH) == DMA1_Stream3) || \
                                   ((PERIPH) == DMA1_Stream4) || \
                                   ((PERIPH) == DMA1_Stream5) || \
                                   ((PERIPH) == DMA1_Stream6) || \
                                   ((PERIPH) == DMA1_Stream7) || \
                                   ((PERIPH) == DMA2_Stream0) || \
                                   ((PERIPH) == DMA2_Stream1) || \
                                   ((PERIPH) == DMA2_Stream2) || \
                                   ((PERIPH) == DMA2_Stream3) || \
                                   ((PERIPH) == DMA2_Stream4) || \
                                   ((PERIPH) == DMA2_Stream5) || \
                                   ((PERIPH) == DMA2_Stream6) || \
                                   ((PERIPH) == DMA2_Stream7))

#define IS_DMA_ALL_CONTROLLER(CONTROLLER) (((CONTROLLER) == DMA1) || \
                                           ((CONTROLLER) == DMA2))

/** @defgroup DMA_channel 
  * @{
  */ 
#define DMA_Channel_0                     ((uint32_t)0x00000000)
#define DMA_Channel_1                     ((uint32_t)0x02000000)
#define DMA_Channel_2                     ((uint32_t)0x04000000)
#define DMA_Channel_3                     ((uint32_t)0x06000000)
#define DMA_Channel_4                     ((uint32_t)0x08000000)
#define DMA_Channel_5                     ((uint32_t)0x0A000000)
#define DMA_Channel_6                     ((uint32_t)0x0C000000)
#define DMA_Channel_7                     ((uint32_t)0x0E000000)

#define IS_DMA_CHANNEL(CHANNEL) (((CHANNEL) == DMA_Channel_0) || \
                                 ((CHANNEL) == DMA_Channel_1) || \
                                 ((CHANNEL) == DMA_Channel_2) || \
                                 ((CHANNEL) == DMA_Channel_3) || \
                                 ((CHANNEL) == DMA_Channel_4) || \
                                 ((CHANNEL) == DMA_Channel_5) || \
                                 ((CHANNEL) == DMA_Channel_6) || \
                                 ((CHANNEL) == DMA_Channel_7))
/**
  * @}
  */ 


/** @defgroup DMA_data_transfer_direction 
  * @{
  */ 
#define DMA_DIR_PeripheralToMemory        ((uint32_t)0x00000000)
#define DMA_DIR_MemoryToPeripheral        ((uint32_t)0x00000040) 
#define DMA_DIR_MemoryToMemory            ((uint32_t)0x00000080)

#define IS_DMA_DIRECTION(DIRECTION) (((DIRECTION) == DMA_DIR_PeripheralToMemory ) || \
                                     ((DIRECTION) == DMA_DIR_MemoryToPeripheral)  || \
                                     ((DIRECTION) == DMA_DIR_MemoryToMemory)) 
/**
  * @}
  */ 


/** @defgroup DMA_data_buffer_size 
  * @{
  */ 
#define IS_DMA_BUFFER_SIZE(SIZE) (((SIZE) >= 0x1) && ((SIZE) < 0x10000))
/**
  * @}
  */ 


/** @defgroup DMA_peripheral_incremented_mode 
  * @{
  */ 
#define DMA_PeripheralInc_Enable          ((uint32_t)0x00000200)
#define DMA_PeripheralInc_Disable         ((uint32_t)0x00000000)

#define IS_DMA_PERIPHERAL_INC_STATE(STATE) (((STATE) == DMA_PeripheralInc_Enable) || \
                                            ((STATE) == DMA_PeripheralInc_Disable))
/**
  * @}
  */ 


/** @defgroup DMA_memory_incremented_mode 
  * @{
  */ 
#define DMA_MemoryInc_Enable              ((uint32_t)0x00000400)
#define DMA_MemoryInc_Disable             ((uint32_t)0x00000000)

#define IS_DMA_MEMORY_INC_STATE(STATE) (((STATE) == DMA_MemoryInc_Enable) || \
                                        ((STATE) == DMA_MemoryInc_Disable))
/**
  * @}
  */ 


/** @defgroup DMA_peripheral_data_size 
  * @{
  */ 
#define DMA_PeripheralDataSize_Byte       ((uint32_t)0x00000000) 
#define DMA_PeripheralDataSize_HalfWord   ((uint32_t)0x00000800) 
#define DMA_PeripheralDataSize_Word       ((uint32_t)0x00001000)

#define IS_DMA_PERIPHERAL_DATA_SIZE(SIZE) (((SIZE) == DMA_PeripheralDataSize_Byte)  || \
                                           ((SIZE) == DMA_PeripheralDataSize_HalfWord) || \
                                           ((SIZE) == DMA_PeripheralDataSize_Word))
/**
  * @}
  */ 


/** @defgroup DMA_memory_data_size 
  * @{
  */ 
#define DMA_MemoryDataSize_Byte           ((uint32_t)0x00000000) 
#define DMA_MemoryDataSize_HalfWord       ((uint32_t)0x00002000) 
#define DMA_MemoryDataSize_Word           ((uint32_t)0x00004000)

#define IS_DMA_MEMORY_DATA_SIZE(SIZE) (((SIZE) == DMA_MemoryDataSize_Byte)  || \
                                       ((SIZE) == DMA_MemoryDataSize_HalfWord) || \
                                       ((SIZE) == DMA_MemoryDataSize_Word ))
/**
  * @}
  */ 


/** @defgroup DMA_circular_normal_mode 
  * @{
  */ 
#define DMA_Mode_Normal                   ((uint32_t)0x00000000) 
#define DMA_Mode_Circular                 ((uint32_t)0x00000100)

#define IS_DMA_MODE(MODE) (((MODE) == DMA_Mode_Normal ) || \
                           ((MODE) == DMA_Mode_Circular)) 
/**
  * @}
  */ 


/** @defgroup DMA_priority_level 
  * @{
  */ 
#define DMA_Priority_Low                  ((uint32_t)0x00000000)
#define DMA_Priority_Medium               ((uint32_t)0x00010000) 
#define DMA_Priority_High                 ((uint32_t)0x00020000)
#define DMA_Priority_VeryHigh             ((uint32_t)0x00030000)

#define IS_DMA_PRIORITY(PRIORITY) (((PRIORITY) == DMA_Priority_Low )   || \
                                   ((PRIORITY) == DMA_Priority_Medium) || \
                                   ((PRIORITY) == DMA_Priority_High)   || \
                                   ((PRIORITY) == DMA_Priority_VeryHigh)) 
/**
  * @}
  */ 


/** @defgroup DMA_fifo_direct_mode 
  * @{
  */ 
#define DMA_FIFOMode_Disable              ((uint32_t)0x00000000) 
#define DMA_FIFOMode_Enable               ((uint32_t)0x00000004)

#define IS_DMA_FIFO_MODE_STATE(STATE) (((STATE) == DMA_FIFOMode_Disable ) || \
                                       ((STATE) == DMA_FIFOMode_Enable)) 
/**
  * @}
  */ 


/** @defgroup DMA_fifo_threshold_level 
  * @{
  */ 
#define DMA_FIFOThreshold_1QuarterFull    ((uint32_t)0x00000000)
#define DMA_FIFOThreshold_HalfFull        ((uint32_t)0x00000001) 
#define DMA_FIFOThreshold_3QuartersFull   ((uint32_t)0x00000002)
#define DMA_FIFOThreshold_Full            ((uint32_t)0x00000003)

#define IS_DMA_FIFO_THRESHOLD(THRESHOLD) (((THRESHOLD) == DMA_FIFOThreshold_1QuarterFull ) || \
                                          ((THRESHOLD) == DMA_FIFOThreshold_HalfFull)      || \
                                          ((THRESHOLD) == DMA_FIFOThreshold_3QuartersFull) || \
                                          ((THRESHOLD) == DMA_FIFOThreshold_Full)) 
/**
  * @}
  */ 


/** @defgroup DMA_memory_burst 
  * @{
  */ 
#define DMA_MemoryBurst_Single            ((uint32_t)0x00000000)
#define DMA_MemoryBurst_INC4              ((uint32_t)0x00800000)  
#define DMA_MemoryBurst_INC8              ((uint32_t)0x01000000)
#define DMA_MemoryBurst_INC16             ((uint32_t)0x01800000)

#define IS_DMA_MEMORY_BURST(BURST) (((BURST) == DMA_MemoryBurst_Single) || \
                                    ((BURST) == DMA_MemoryBurst_INC4)  || \
                                    ((BURST) == DMA_MemoryBurst_INC8)  || \
                                    ((BURST) == DMA_MemoryBurst_INC16))
/**
  * @}
  */ 


/** @defgroup DMA_peripheral_burst 
  * @{
  */ 
#define DMA_PeripheralBurst_Single        ((uint32_t)0x00000000)
#define DMA_PeripheralBurst_INC4          ((uint32_t)0x00200000)  
#define DMA_PeripheralBurst_INC8          ((uint32_t)0x00400000)
#define DMA_PeripheralBurst_INC16         ((uint32_t)0x00600000)

#define IS_DMA_PERIPHERAL_BURST(BURST) (((BURST) == DMA_PeripheralBurst_Single) || \
                                        ((BURST) == DMA_PeripheralBurst_INC4)  || \
                                        ((BURST) == DMA_PeripheralBurst_INC8)  || \
                                        ((BURST) == DMA_PeripheralBurst_INC16))
/**
  * @}
  */ 


/** @defgroup DMA_fifo_status_level 
  * @{
  */
#define DMA_FIFOStatus_Less1QuarterFull   ((uint32_t)0x00000000 << 3)
#define DMA_FIFOStatus_1QuarterFull       ((uint32_t)0x00000001 << 3)
#define DMA_FIFOStatus_HalfFull           ((uint32_t)0x00000002 << 3) 
#define DMA_FIFOStatus_3QuartersFull      ((uint32_t)0x00000003 << 3)
#define DMA_FIFOStatus_Empty              ((uint32_t)0x00000004 << 3)
#define DMA_FIFOStatus_Full               ((uint32_t)0x00000005 << 3)

#define IS_DMA_FIFO_STATUS(STATUS) (((STATUS) == DMA_FIFOStatus_Less1QuarterFull ) || \
                                    ((STATUS) == DMA_FIFOStatus_HalfFull)          || \
                                    ((STATUS) == DMA_FIFOStatus_1QuarterFull)      || \
                                    ((STATUS) == DMA_FIFOStatus_3QuartersFull)     || \
                                    ((STATUS) == DMA_FIFOStatus_Full)              || \
                                    ((STATUS) == DMA_FIFOStatus_Empty)) 
/**
  * @}
  */ 

/** @defgroup DMA_flags_definition 
  * @{
  */
#define DMA_FLAG_FEIF0                    ((uint32_t)0x10800001)
#define DMA_FLAG_DMEIF0                   ((uint32_t)0x10800004)
#define DMA_FLAG_TEIF0                    ((uint32_t)0x10000008)
#define DMA_FLAG_HTIF0                    ((uint32_t)0x10000010)
#define DMA_FLAG_TCIF0                    ((uint32_t)0x10000020)
#define DMA_FLAG_FEIF1                    ((uint32_t)0x10000040)
#define DMA_FLAG_DMEIF1                   ((uint32_t)0x10000100)
#define DMA_FLAG_TEIF1                    ((uint32_t)0x10000200)
#define DMA_FLAG_HTIF1                    ((uint32_t)0x10000400)
#define DMA_FLAG_TCIF1                    ((uint32_t)0x10000800)
#define DMA_FLAG_FEIF2                    ((uint32_t)0x10010000)
#define DMA_FLAG_DMEIF2                   ((uint32_t)0x10040000)
#define DMA_FLAG_TEIF2                    ((uint32_t)0x10080000)
#define DMA_FLAG_HTIF2                    ((uint32_t)0x10100000)
#define DMA_FLAG_TCIF2                    ((uint32_t)0x10200000)
#define DMA_FLAG_FEIF3                    ((uint32_t)0x10400000)
#define DMA_FLAG_DMEIF3                   ((uint32_t)0x11000000)
#define DMA_FLAG_TEIF3                    ((uint32_t)0x12000000)
#define DMA_FLAG_HTIF3                    ((uint32_t)0x14000000)
#define DMA_FLAG_TCIF3                    ((uint32_t)0x18000000)
#define DMA_FLAG_FEIF4                    ((uint32_t)0x20000001)
#define DMA_FLAG_DMEIF4                   ((uint32_t)0x20000004)
#define DMA_FLAG_TEIF4                    ((uint32_t)0x20000008)
#define DMA_FLAG_HTIF4                    ((uint32_t)0x20000010)
#define DMA_FLAG_TCIF4                    ((uint32_t)0x20000020)
#define DMA_FLAG_FEIF5                    ((uint32_t)0x20000040)
#define DMA_FLAG_DMEIF5                   ((uint32_t)0x20000100)
#define DMA_FLAG_TEIF5                    ((uint32_t)0x20000200)
#define DMA_FLAG_HTIF5                    ((uint32_t)0x20000400)
#define DMA_FLAG_TCIF5                    ((uint32_t)0x20000800)
#define DMA_FLAG_FEIF6                    ((uint32_t)0x20010000)
#define DMA_FLAG_DMEIF6                   ((uint32_t)0x20040000)
#define DMA_FLAG_TEIF6                    ((uint32_t)0x20080000)
#define DMA_FLAG_HTIF6                    ((uint32_t)0x20100000)
#define DMA_FLAG_TCIF6                    ((uint32_t)0x20200000)
#define DMA_FLAG_FEIF7                    ((uint32_t)0x20400000)
#define DMA_FLAG_DMEIF7                   ((uint32_t)0x21000000)
#define DMA_FLAG_TEIF7                    ((uint32_t)0x22000000)
#define DMA_FLAG_HTIF7                    ((uint32_t)0x24000000)
#define DMA_FLAG_TCIF7                    ((uint32_t)0x28000000)

#define IS_DMA_CLEAR_FLAG(FLAG) ((((FLAG) & 0x30000000) != 0x30000000) && (((FLAG) & 0x30000000) != 0) && \
                                 (((FLAG) & 0xC002F082) == 0x00) && ((FLAG) != 0x00))

#define IS_DMA_GET_FLAG(FLAG) (((FLAG) == DMA_FLAG_TCIF0)  || ((FLAG) == DMA_FLAG_HTIF0)  || \
                               ((FLAG) == DMA_FLAG_TEIF0)  || ((FLAG) == DMA_FLAG_DMEIF0) || \
                               ((FLAG) == DMA_FLAG_FEIF0)  || ((FLAG) == DMA_FLAG_TCIF1)  || \
                               ((FLAG) == DMA_FLAG_HTIF1)  || ((FLAG) == DMA_FLAG_TEIF1)  || \
                               ((FLAG) == DMA_FLAG_DMEIF1) || ((FLAG) == DMA_FLAG_FEIF1)  || \
                               ((FLAG) == DMA_FLAG_TCIF2)  || ((FLAG) == DMA_FLAG_HTIF2)  || \
                               ((FLAG) == DMA_FLAG_TEIF2)  || ((FLAG) == DMA_FLAG_DMEIF2) || \
                               ((FLAG) == DMA_FLAG_FEIF2)  || ((FLAG) == DMA_FLAG_TCIF3)  || \
                               ((FLAG) == DMA_FLAG_HTIF3)  || ((FLAG) == DMA_FLAG_TEIF3)  || \
                               ((FLAG) == DMA_FLAG_DMEIF3) || ((FLAG) == DMA_FLAG_FEIF3)  || \
                               ((FLAG) == DMA_FLAG_TCIF4)  || ((FLAG) == DMA_FLAG_HTIF4)  || \
                               ((FLAG) == DMA_FLAG_TEIF4)  || ((FLAG) == DMA_FLAG_DMEIF4) || \
                               ((FLAG) == DMA_FLAG_FEIF4)  || ((FLAG) == DMA_FLAG_TCIF5)  || \
                               ((FLAG) == DMA_FLAG_HTIF5)  || ((FLAG) == DMA_FLAG_TEIF5)  || \
                               ((FLAG) == DMA_FLAG_DMEIF5) || ((FLAG) == DMA_FLAG_FEIF5)  || \
                               ((FLAG) == DMA_FLAG_TCIF6)  || ((FLAG) == DMA_FLAG_HTIF6)  || \
                               ((FLAG) == DMA_FLAG_TEIF6)  || ((FLAG) == DMA_FLAG_DMEIF6) || \
                               ((FLAG) == DMA_FLAG_FEIF6)  || ((FLAG) == DMA_FLAG_TCIF7)  || \
                               ((FLAG) == DMA_FLAG_HTIF7)  || ((FLAG) == DMA_FLAG_TEIF7)  || \
                               ((FLAG) == DMA_FLAG_DMEIF7) || ((FLAG) == DMA_FLAG_FEIF7))
/**
  * @}
  */ 


/** @defgroup DMA_interrupt_enable_definitions 
  * @{
  */ 
#define DMA_IT_TC                         ((uint32_t)0x00000010)
#define DMA_IT_HT                         ((uint32_t)0x00000008)
#define DMA_IT_TE                         ((uint32_t)0x00000004)
#define DMA_IT_DME                        ((uint32_t)0x00000002)
#define DMA_IT_FE                         ((uint32_t)0x00000080)

#define IS_DMA_CONFIG_IT(IT) ((((IT) & 0xFFFFFF61) == 0x00) && ((IT) != 0x00))
/**
  * @}
  */ 


/** @defgroup DMA_interrupts_definitions 
  * @{
  */ 
#define DMA_IT_FEIF0                      ((uint32_t)0x90000001)
#define DMA_IT_DMEIF0                     ((uint32_t)0x10001004)
#define DMA_IT_TEIF0                      ((uint32_t)0x10002008)
#define DMA_IT_HTIF0                      ((uint32_t)0x10004010)
#define DMA_IT_TCIF0                      ((uint32_t)0x10008020)
#define DMA_IT_FEIF1                      ((uint32_t)0x90000040)
#define DMA_IT_DMEIF1                     ((uint32_t)0x10001100)
#define DMA_IT_TEIF1                      ((uint32_t)0x10002200)
#define DMA_IT_HTIF1                      ((uint32_t)0x10004400)
#define DMA_IT_TCIF1                      ((uint32_t)0x10008800)
#define DMA_IT_FEIF2                      ((uint32_t)0x90010000)
#define DMA_IT_DMEIF2                     ((uint32_t)0x10041000)
#define DMA_IT_TEIF2                      ((uint32_t)0x10082000)
#define DMA_IT_HTIF2                      ((uint32_t)0x10104000)
#define DMA_IT_TCIF2                      ((uint32_t)0x10208000)
#define DMA_IT_FEIF3                      ((uint32_t)0x90400000)
#define DMA_IT_DMEIF3                     ((uint32_t)0x11001000)
#define DMA_IT_TEIF3                      ((uint32_t)0x12002000)
#define DMA_IT_HTIF3                      ((uint32_t)0x14004000)
#define DMA_IT_TCIF3                      ((uint32_t)0x18008000)
#define DMA_IT_FEIF4                      ((uint32_t)0xA0000001)
#define DMA_IT_DMEIF4                     ((uint32_t)0x20001004)
#define DMA_IT_TEIF4                      ((uint32_t)0x20002008)
#define DMA_IT_HTIF4                      ((uint32_t)0x20004010)
#define DMA_IT_TCIF4                      ((uint32_t)0x20008020)
#define DMA_IT_FEIF5                      ((uint32_t)0xA0000040)
#define DMA_IT_DMEIF5                     ((uint32_t)0x20001100)
#define DMA_IT_TEIF5                      ((uint32_t)0x20002200)
#define DMA_IT_HTIF5                      ((uint32_t)0x20004400)
#define DMA_IT_TCIF5                      ((uint32_t)0x20008800)
#define DMA_IT_FEIF6                      ((uint32_t)0xA0010000)
#define DMA_IT_DMEIF6                     ((uint32_t)0x20041000)
#define DMA_IT_TEIF6                      ((uint32_t)0x20082000)
#define DMA_IT_HTIF6                      ((uint32_t)0x20104000)
#define DMA_IT_TCIF6                      ((uint32_t)0x20208000)
#define DMA_IT_FEIF7                      ((uint32_t)0xA0400000)
#define DMA_IT_DMEIF7                     ((uint32_t)0x21001000)
#define DMA_IT_TEIF7                      ((uint32_t)0x22002000)
#define DMA_IT_HTIF7                      ((uint32_t)0x24004000)
#define DMA_IT_TCIF7                      ((uint32_t)0x28008000)

#define IS_DMA_CLEAR_IT(IT) ((((IT) & 0x30000000) != 0x30000000) && \
                             (((IT) & 0x30000000) != 0) && ((IT) != 0x00) && \
                             (((IT) & 0x40820082) == 0x00))

#define IS_DMA_GET_IT(IT) (((IT) == DMA_IT_TCIF0) || ((IT) == DMA_IT_HTIF0)  || \
                           ((IT) == DMA_IT_TEIF0) || ((IT) == DMA_IT_DMEIF0) || \
                           ((IT) == DMA_IT_FEIF0) || ((IT) == DMA_IT_TCIF1)  || \
                           ((IT) == DMA_IT_HTIF1) || ((IT) == DMA_IT_TEIF1)  || \
                           ((IT) == DMA_IT_DMEIF1)|| ((IT) == DMA_IT_FEIF1)  || \
                           ((IT) == DMA_IT_TCIF2) || ((IT) == DMA_IT_HTIF2)  || \
                           ((IT) == DMA_IT_TEIF2) || ((IT) == DMA_IT_DMEIF2) || \
                           ((IT) == DMA_IT_FEIF2) || ((IT) == DMA_IT_TCIF3)  || \
                           ((IT) == DMA_IT_HTIF3) || ((IT) == DMA_IT_TEIF3)  || \
                           ((IT) == DMA_IT_DMEIF3)|| ((IT) == DMA_IT_FEIF3)  || \
                           ((IT) == DMA_IT_TCIF4) || ((IT) == DMA_IT_HTIF4)  || \
                           ((IT) == DMA_IT_TEIF4) || ((IT) == DMA_IT_DMEIF4) || \
                           ((IT) == DMA_IT_FEIF4) || ((IT) == DMA_IT_TCIF5)  || \
                           ((IT) == DMA_IT_HTIF5) || ((IT) == DMA_IT_TEIF5)  || \
                           ((IT) == DMA_IT_DMEIF5)|| ((IT) == DMA_IT_FEIF5)  || \
                           ((IT) == DMA_IT_TCIF6) || ((IT) == DMA_IT_HTIF6)  || \
                           ((IT) == DMA_IT_TEIF6) || ((IT) == DMA_IT_DMEIF6) || \
                           ((IT) == DMA_IT_FEIF6) || ((IT) == DMA_IT_TCIF7)  || \
                           ((IT) == DMA_IT_HTIF7) || ((IT) == DMA_IT_TEIF7)  || \
                           ((IT) == DMA_IT_DMEIF7)|| ((IT) == DMA_IT_FEIF7))
/**
  * @}
  */ 


/** @defgroup DMA_peripheral_increment_offset 
  * @{
  */ 
#define DMA_PINCOS_Psize                  ((uint32_t)0x00000000)
#define DMA_PINCOS_WordAligned            ((uint32_t)0x00008000)

#define IS_DMA_PINCOS_SIZE(SIZE) (((SIZE) == DMA_PINCOS_Psize) || \
                                  ((SIZE) == DMA_PINCOS_WordAligned))
/**
  * @}
  */ 


/** @defgroup DMA_flow_controller_definitions 
  * @{
  */ 
#define DMA_FlowCtrl_Memory               ((uint32_t)0x00000000)
#define DMA_FlowCtrl_Peripheral           ((uint32_t)0x00000020)

#define IS_DMA_FLOW_CTRL(CTRL) (((CTRL) == DMA_FlowCtrl_Memory) || \
                                ((CTRL) == DMA_FlowCtrl_Peripheral))
/**
  * @}
  */ 


/** @defgroup DMA_memory_targets_definitions 
  * @{
  */ 
#define DMA_Memory_0                      ((uint32_t)0x00000000)
#define DMA_Memory_1                      ((uint32_t)0x00080000)

#define IS_DMA_CURRENT_MEM(MEM) (((MEM) == DMA_Memory_0) || ((MEM) == DMA_Memory_1))
/**
  * @}
  */ 

/**
  * @}
  */ 

/* Exported macro ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/ 

/*  Function used to set the DMA configuration to the default reset state *****/ 
void DMA_DeInit(DMA_Stream_TypeDef* DMAy_Streamx);

/* Initialization and Configuration functions *********************************/
void DMA_Init(DMA_Stream_TypeDef* DMAy_Streamx, DMA_InitTypeDef* DMA_InitStruct);
void DMA_StructInit(DMA_InitTypeDef* DMA_InitStruct);



FlagStatus DMA_GetFlagStatus(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_FLAG);
void DMA_ClearFlag(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_FLAG);
void DMA_ITConfig(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_IT, FunctionalState NewState);
ITStatus DMA_GetITStatus(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_IT);
void DMA_ClearITPendingBit(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_IT);


/**
  * @}
  */

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


/***************** from .c  ***********************/


/**
  * @brief  Enables or disables the specified DMAy Streamx.
  * @param  DMAy_Streamx: where y can be 1 or 2 to select the DMA and x can be 0
  *         to 7 to select the DMA Stream.
  * @param  NewState: new state of the DMAy Streamx. 
  *          This parameter can be: ENABLE or DISABLE.
  *
  * @note  This function may be used to perform Pause-Resume operation. When a
  *        transfer is ongoing, calling this function to disable the Stream will
  *        cause the transfer to be paused. All configuration registers and the
  *        number of remaining data will be preserved. When calling again this
  *        function to re-enable the Stream, the transfer will be resumed from
  *        the point where it was paused.          
  *    
  * @note  After configuring the DMA Stream (DMA_Init() function) and enabling the
  *        stream, it is recommended to check (or wait until) the DMA Stream is
  *        effectively enabled. A Stream may remain disabled if a configuration 
  *        parameter is wrong.
  *        After disabling a DMA Stream, it is also recommended to check (or wait
  *        until) the DMA Stream is effectively disabled. If a Stream is disabled 
  *        while a data transfer is ongoing, the current data will be transferred
  *        and the Stream will be effectively disabled only after the transfer of
  *        this single data is finished.            
  *    
  * @retval None
  */
static inline void DMA_Cmd(DMA_Stream_TypeDef* DMAy_Streamx, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    /* Enable the selected DMAy Streamx by setting EN bit */
    DMAy_Streamx->CR |= (uint32_t)DMA_SxCR_EN;
  }
  else
  {
    /* Disable the selected DMAy Streamx by clearing EN bit */
    DMAy_Streamx->CR &= ~(uint32_t)DMA_SxCR_EN;
  }
}

/**
  * @brief  Configures, when the PINC (Peripheral Increment address mode) bit is
  *         set, if the peripheral address should be incremented with the data 
  *         size (configured with PSIZE bits) or by a fixed offset equal to 4
  *         (32-bit aligned addresses).
  *   
  * @note   This function has no effect if the Peripheral Increment mode is disabled.
  *     
  * @param  DMAy_Streamx: where y can be 1 or 2 to select the DMA and x can be 0
  *          to 7 to select the DMA Stream.
  * @param  DMA_Pincos: specifies the Peripheral increment offset size.
  *          This parameter can be one of the following values:
  *            @arg DMA_PINCOS_Psize: Peripheral address increment is done  
  *                                   accordingly to PSIZE parameter.
  *            @arg DMA_PINCOS_WordAligned: Peripheral address increment offset is 
  *                                         fixed to 4 (32-bit aligned addresses). 
  * @retval None
  */
static inline void DMA_PeriphIncOffsetSizeConfig(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_Pincos)
{
  /* Check the parameters */
  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_DMA_PINCOS_SIZE(DMA_Pincos));

  /* Check the needed Peripheral increment offset */
  if(DMA_Pincos != DMA_PINCOS_Psize)
  {
    /* Configure DMA_SxCR_PINCOS bit with the input parameter */
    DMAy_Streamx->CR |= (uint32_t)DMA_SxCR_PINCOS;     
  }
  else
  {
    /* Clear the PINCOS bit: Peripheral address incremented according to PSIZE */
    DMAy_Streamx->CR &= ~(uint32_t)DMA_SxCR_PINCOS;    
  }
}

/**
  * @brief  Configures, when the DMAy Streamx is disabled, the flow controller for
  *         the next transactions (Peripheral or Memory).
  *       
  * @note   Before enabling this feature, check if the used peripheral supports 
  *         the Flow Controller mode or not.    
  *  
  * @param  DMAy_Streamx: where y can be 1 or 2 to select the DMA and x can be 0
  *          to 7 to select the DMA Stream.
  * @param  DMA_FlowCtrl: specifies the DMA flow controller.
  *          This parameter can be one of the following values:
  *            @arg DMA_FlowCtrl_Memory: DMAy_Streamx transactions flow controller is 
  *                                      the DMA controller.
  *            @arg DMA_FlowCtrl_Peripheral: DMAy_Streamx transactions flow controller 
  *                                          is the peripheral.    
  * @retval None
  */
static inline void DMA_FlowControllerConfig(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_FlowCtrl)
{
  /* Check the parameters */
  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_DMA_FLOW_CTRL(DMA_FlowCtrl));

  /* Check the needed flow controller  */
  if(DMA_FlowCtrl != DMA_FlowCtrl_Memory)
  {
    /* Configure DMA_SxCR_PFCTRL bit with the input parameter */
    DMAy_Streamx->CR |= (uint32_t)DMA_SxCR_PFCTRL;   
  }
  else
  {
    /* Clear the PFCTRL bit: Memory is the flow controller */
    DMAy_Streamx->CR &= ~(uint32_t)DMA_SxCR_PFCTRL;    
  }
}
/**
  * @}
  */

/** @defgroup DMA_Group2 Data Counter functions
 *  @brief   Data Counter functions 
 *
@verbatim   
 ===============================================================================
                      ##### Data Counter functions #####
 ===============================================================================  
    [..]
    This subsection provides function allowing to configure and read the buffer size
    (number of data to be transferred). 
    [..]
    The DMA data counter can be written only when the DMA Stream is disabled 
    (ie. after transfer complete event).
    [..]
    The following function can be used to write the Stream data counter value:
      (+) void DMA_SetCurrDataCounter(DMA_Stream_TypeDef* DMAy_Streamx, uint16_t Counter);
      -@- It is advised to use this function rather than DMA_Init() in situations 
          where only the Data buffer needs to be reloaded.
      -@- If the Source and Destination Data Sizes are different, then the value 
          written in data counter, expressing the number of transfers, is relative 
          to the number of transfers from the Peripheral point of view.
          ie. If Memory data size is Word, Peripheral data size is Half-Words, 
          then the value to be configured in the data counter is the number 
          of Half-Words to be transferred from/to the peripheral.
    [..]
    The DMA data counter can be read to indicate the number of remaining transfers for
    the relative DMA Stream. This counter is decremented at the end of each data 
    transfer and when the transfer is complete: 
      (+) If Normal mode is selected: the counter is set to 0.
      (+) If Circular mode is selected: the counter is reloaded with the initial value
          (configured before enabling the DMA Stream)
     [..]
     The following function can be used to read the Stream data counter value:
       (+) uint16_t DMA_GetCurrDataCounter(DMA_Stream_TypeDef* DMAy_Streamx);

@endverbatim
  * @{
  */

/**
  * @brief  Writes the number of data units to be transferred on the DMAy Streamx.
  * @param  DMAy_Streamx: where y can be 1 or 2 to select the DMA and x can be 0
  *          to 7 to select the DMA Stream.
  * @param  Counter: Number of data units to be transferred (from 0 to 65535) 
  *          Number of data items depends only on the Peripheral data format.
  *            
  * @note   If Peripheral data format is Bytes: number of data units is equal 
  *         to total number of bytes to be transferred.
  *           
  * @note   If Peripheral data format is Half-Word: number of data units is  
  *         equal to total number of bytes to be transferred / 2.
  *           
  * @note   If Peripheral data format is Word: number of data units is equal 
  *         to total  number of bytes to be transferred / 4.
  *      
  * @note   In Memory-to-Memory transfer mode, the memory buffer pointed by 
  *         DMAy_SxPAR register is considered as Peripheral.
  *      
  * @retval The number of remaining data units in the current DMAy Streamx transfer.
  */
static inline void DMA_SetCurrDataCounter(DMA_Stream_TypeDef* DMAy_Streamx, uint16_t Counter)
{
  /* Check the parameters */
  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));

  /* Write the number of data units to be transferred */
  DMAy_Streamx->NDTR = (uint16_t)Counter;
}

/**
  * @brief  Returns the number of remaining data units in the current DMAy Streamx transfer.
  * @param  DMAy_Streamx: where y can be 1 or 2 to select the DMA and x can be 0
  *          to 7 to select the DMA Stream.
  * @retval The number of remaining data units in the current DMAy Streamx transfer.
  */
static inline uint16_t DMA_GetCurrDataCounter(DMA_Stream_TypeDef* DMAy_Streamx)
{
  /* Check the parameters */
  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));

  /* Return the number of remaining data units for DMAy Streamx */
  return ((uint16_t)(DMAy_Streamx->NDTR));
}
/**
  * @}
  */

/** @defgroup DMA_Group3 Double Buffer mode functions
 *  @brief   Double Buffer mode functions 
 *
@verbatim   
 ===============================================================================
                    ##### Double Buffer mode functions #####
 ===============================================================================  
    [..]
    This subsection provides function allowing to configure and control the double 
    buffer mode parameters.
    
    [..]
    The Double Buffer mode can be used only when Circular mode is enabled.
    The Double Buffer mode cannot be used when transferring data from Memory to Memory.
    
    [..]
    The Double Buffer mode allows to set two different Memory addresses from/to which
    the DMA controller will access alternatively (after completing transfer to/from 
    target memory 0, it will start transfer to/from target memory 1).
    This allows to reduce software overhead for double buffering and reduce the CPU
    access time.
    
    [..]
    Two functions must be called before calling the DMA_Init() function:
      (+) void DMA_DoubleBufferModeConfig(DMA_Stream_TypeDef* DMAy_Streamx, 
          uint32_t Memory1BaseAddr, uint32_t DMA_CurrentMemory);
      (+) void DMA_DoubleBufferModeCmd(DMA_Stream_TypeDef* DMAy_Streamx, FunctionalState NewState);
      
    [..]
    DMA_DoubleBufferModeConfig() is called to configure the Memory 1 base address 
    and the first Memory target from/to which the transfer will start after 
    enabling the DMA Stream. Then DMA_DoubleBufferModeCmd() must be called 
    to enable the Double Buffer mode (or disable it when it should not be used).
  
    [..]
    Two functions can be called dynamically when the transfer is ongoing (or when the DMA Stream is 
    stopped) to modify on of the target Memories addresses or to check wich Memory target is currently
    used:
      (+) void DMA_MemoryTargetConfig(DMA_Stream_TypeDef* DMAy_Streamx, 
                uint32_t MemoryBaseAddr, uint32_t DMA_MemoryTarget);
      (+) uint32_t DMA_GetCurrentMemoryTarget(DMA_Stream_TypeDef* DMAy_Streamx);
      
    [..]
    DMA_MemoryTargetConfig() can be called to modify the base address of one of 
    the two target Memories.
    The Memory of which the base address will be modified must not be currently 
    be used by the DMA Stream (ie. if the DMA Stream is currently transferring 
    from Memory 1 then you can only modify base address of target Memory 0 and vice versa).
    To check this condition, it is recommended to use the function DMA_GetCurrentMemoryTarget() which
    returns the index of the Memory target currently in use by the DMA Stream.

@endverbatim
  * @{
  */
  
/**
  * @brief  Configures, when the DMAy Streamx is disabled, the double buffer mode 
  *         and the current memory target.
  * @param  DMAy_Streamx: where y can be 1 or 2 to select the DMA and x can be 0
  *          to 7 to select the DMA Stream.
  * @param  Memory1BaseAddr: the base address of the second buffer (Memory 1)  
  * @param  DMA_CurrentMemory: specifies which memory will be first buffer for
  *         the transactions when the Stream will be enabled. 
  *          This parameter can be one of the following values:
  *            @arg DMA_Memory_0: Memory 0 is the current buffer.
  *            @arg DMA_Memory_1: Memory 1 is the current buffer.  
  *       
  * @note   Memory0BaseAddr is set by the DMA structure configuration in DMA_Init().
  *   
  * @retval None
  */
static inline void DMA_DoubleBufferModeConfig(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t Memory1BaseAddr,
                                uint32_t DMA_CurrentMemory)
{  
  /* Check the parameters */
  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_DMA_CURRENT_MEM(DMA_CurrentMemory));

  if (DMA_CurrentMemory != DMA_Memory_0)
  {
    /* Set Memory 1 as current memory address */
    DMAy_Streamx->CR |= (uint32_t)(DMA_SxCR_CT);    
  }
  else
  {
    /* Set Memory 0 as current memory address */
    DMAy_Streamx->CR &= ~(uint32_t)(DMA_SxCR_CT);    
  }

  /* Write to DMAy Streamx M1AR */
  DMAy_Streamx->M1AR = Memory1BaseAddr;
}

/**
  * @brief  Enables or disables the double buffer mode for the selected DMA stream.
  * @note   This function can be called only when the DMA Stream is disabled.  
  * @param  DMAy_Streamx: where y can be 1 or 2 to select the DMA and x can be 0
  *          to 7 to select the DMA Stream.
  * @param  NewState: new state of the DMAy Streamx double buffer mode. 
  *          This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
static inline void DMA_DoubleBufferModeCmd(DMA_Stream_TypeDef* DMAy_Streamx, FunctionalState NewState)
{  
  /* Check the parameters */
  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  /* Configure the Double Buffer mode */
  if (NewState != DISABLE)
  {
    /* Enable the Double buffer mode */
    DMAy_Streamx->CR |= (uint32_t)DMA_SxCR_DBM;
  }
  else
  {
    /* Disable the Double buffer mode */
    DMAy_Streamx->CR &= ~(uint32_t)DMA_SxCR_DBM;
  }
}

/**
  * @brief  Configures the Memory address for the next buffer transfer in double
  *         buffer mode (for dynamic use). This function can be called when the
  *         DMA Stream is enabled and when the transfer is ongoing.  
  * @param  DMAy_Streamx: where y can be 1 or 2 to select the DMA and x can be 0
  *          to 7 to select the DMA Stream.
  * @param  MemoryBaseAddr: The base address of the target memory buffer
  * @param  DMA_MemoryTarget: Next memory target to be used. 
  *         This parameter can be one of the following values:
  *            @arg DMA_Memory_0: To use the memory address 0
  *            @arg DMA_Memory_1: To use the memory address 1
  * 
  * @note    It is not allowed to modify the Base Address of a target Memory when
  *          this target is involved in the current transfer. ie. If the DMA Stream
  *          is currently transferring to/from Memory 1, then it not possible to
  *          modify Base address of Memory 1, but it is possible to modify Base
  *          address of Memory 0.
  *          To know which Memory is currently used, you can use the function
  *          DMA_GetCurrentMemoryTarget().             
  *  
  * @retval None
  */
static inline void DMA_MemoryTargetConfig(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t MemoryBaseAddr,
                           uint32_t DMA_MemoryTarget)
{
  /* Check the parameters */
  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  assert_param(IS_DMA_CURRENT_MEM(DMA_MemoryTarget));
    
  /* Check the Memory target to be configured */
  if (DMA_MemoryTarget != DMA_Memory_0)
  {
    /* Write to DMAy Streamx M1AR */
    DMAy_Streamx->M1AR = MemoryBaseAddr;    
  }  
  else
  {
    /* Write to DMAy Streamx M0AR */
    DMAy_Streamx->M0AR = MemoryBaseAddr;  
  }
}

/**
  * @brief  Returns the current memory target used by double buffer transfer.
  * @param  DMAy_Streamx: where y can be 1 or 2 to select the DMA and x can be 0
  *          to 7 to select the DMA Stream.
  * @retval The memory target number: 0 for Memory0 or 1 for Memory1. 
  */
static inline uint32_t DMA_GetCurrentMemoryTarget(DMA_Stream_TypeDef* DMAy_Streamx)
{
  uint32_t tmp = 0;
  
  /* Check the parameters */
  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));

  /* Get the current memory target */
  if ((DMAy_Streamx->CR & DMA_SxCR_CT) != 0)
  {
    /* Current memory buffer used is Memory 1 */
    tmp = 1;
  }  
  else
  {
    /* Current memory buffer used is Memory 0 */
    tmp = 0;    
  }
  return tmp;
}
/**
  * @}
  */

/** @defgroup DMA_Group4 Interrupts and flags management functions
 *  @brief   Interrupts and flags management functions 
 *
@verbatim   
 ===============================================================================
              ##### Interrupts and flags management functions #####
 ===============================================================================  
    [..]
    This subsection provides functions allowing to
      (+) Check the DMA enable status
      (+) Check the FIFO status 
      (+) Configure the DMA Interrupts sources and check or clear the flags or 
          pending bits status.  
           
    [..]
      (#) DMA Enable status:
          After configuring the DMA Stream (DMA_Init() function) and enabling 
          the stream, it is recommended to check (or wait until) the DMA Stream 
          is effectively enabled. A Stream may remain disabled if a configuration 
          parameter is wrong. After disabling a DMA Stream, it is also recommended 
          to check (or wait until) the DMA Stream is effectively disabled. 
          If a Stream is disabled while a data transfer is ongoing, the current 
          data will be transferred and the Stream will be effectively disabled 
          only after this data transfer completion.
          To monitor this state it is possible to use the following function:
        (++) FunctionalState DMA_GetCmdStatus(DMA_Stream_TypeDef* DMAy_Streamx); 
 
      (#) FIFO Status:
          It is possible to monitor the FIFO status when a transfer is ongoing 
          using the following function:
        (++) uint32_t DMA_GetFIFOStatus(DMA_Stream_TypeDef* DMAy_Streamx); 
 
      (#) DMA Interrupts and Flags:
          The user should identify which mode will be used in his application 
          to manage the DMA controller events: Polling mode or Interrupt mode. 
    
    *** Polling Mode ***
    ====================
    [..]
    Each DMA stream can be managed through 4 event Flags:
    (x : DMA Stream number )
      (#) DMA_FLAG_FEIFx  : to indicate that a FIFO Mode Transfer Error event occurred.
      (#) DMA_FLAG_DMEIFx : to indicate that a Direct Mode Transfer Error event occurred.
      (#) DMA_FLAG_TEIFx  : to indicate that a Transfer Error event occurred.
      (#) DMA_FLAG_HTIFx  : to indicate that a Half-Transfer Complete event occurred.
      (#) DMA_FLAG_TCIFx  : to indicate that a Transfer Complete event occurred .       
    [..]
    In this Mode it is advised to use the following functions:
      (+) FlagStatus DMA_GetFlagStatus(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_FLAG);
      (+) void DMA_ClearFlag(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_FLAG);

    *** Interrupt Mode ***
    ======================
    [..]
    Each DMA Stream can be managed through 4 Interrupts:

    *** Interrupt Source ***
    ========================
    [..]
      (#) DMA_IT_FEIFx  : specifies the interrupt source for the  FIFO Mode Transfer Error event.
      (#) DMA_IT_DMEIFx : specifies the interrupt source for the Direct Mode Transfer Error event.
      (#) DMA_IT_TEIFx  : specifies the interrupt source for the Transfer Error event.
      (#) DMA_IT_HTIFx  : specifies the interrupt source for the Half-Transfer Complete event.
      (#) DMA_IT_TCIFx  : specifies the interrupt source for the a Transfer Complete event. 
    [..]
    In this Mode it is advised to use the following functions:
      (+) void DMA_ITConfig(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_IT, FunctionalState NewState);
      (+) ITStatus DMA_GetITStatus(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_IT);
      (+) void DMA_ClearITPendingBit(DMA_Stream_TypeDef* DMAy_Streamx, uint32_t DMA_IT);

@endverbatim
  * @{
  */

/**
  * @brief  Returns the status of EN bit for the specified DMAy Streamx.
  * @param  DMAy_Streamx: where y can be 1 or 2 to select the DMA and x can be 0
  *          to 7 to select the DMA Stream.
  *   
  * @note    After configuring the DMA Stream (DMA_Init() function) and enabling
  *          the stream, it is recommended to check (or wait until) the DMA Stream
  *          is effectively enabled. A Stream may remain disabled if a configuration
  *          parameter is wrong.
  *          After disabling a DMA Stream, it is also recommended to check (or wait 
  *          until) the DMA Stream is effectively disabled. If a Stream is disabled
  *          while a data transfer is ongoing, the current data will be transferred
  *          and the Stream will be effectively disabled only after the transfer
  *          of this single data is finished.  
  *      
  * @retval Current state of the DMAy Streamx (ENABLE or DISABLE).
  */
static inline FunctionalState DMA_GetCmdStatus(DMA_Stream_TypeDef* DMAy_Streamx)
{
  FunctionalState state = DISABLE;

  /* Check the parameters */
  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));

  if ((DMAy_Streamx->CR & (uint32_t)DMA_SxCR_EN) != 0)
  {
    /* The selected DMAy Streamx EN bit is set (DMA is still transferring) */
    state = ENABLE;
  }
  else
  {
    /* The selected DMAy Streamx EN bit is cleared (DMA is disabled and 
        all transfers are complete) */
    state = DISABLE;
  }
  return state;
}

/**
  * @brief  Returns the current DMAy Streamx FIFO filled level.
  * @param  DMAy_Streamx: where y can be 1 or 2 to select the DMA and x can be 0 
  *         to 7 to select the DMA Stream.
  * @retval The FIFO filling state.
  *           - DMA_FIFOStatus_Less1QuarterFull: when FIFO is less than 1 quarter-full 
  *                                               and not empty.
  *           - DMA_FIFOStatus_1QuarterFull: if more than 1 quarter-full.
  *           - DMA_FIFOStatus_HalfFull: if more than 1 half-full.
  *           - DMA_FIFOStatus_3QuartersFull: if more than 3 quarters-full.
  *           - DMA_FIFOStatus_Empty: when FIFO is empty
  *           - DMA_FIFOStatus_Full: when FIFO is full
  */
static inline uint32_t DMA_GetFIFOStatus(DMA_Stream_TypeDef* DMAy_Streamx)
{
  uint32_t tmpreg = 0;
 
  /* Check the parameters */
  assert_param(IS_DMA_ALL_PERIPH(DMAy_Streamx));
  
  /* Get the FIFO level bits */
  tmpreg = (uint32_t)((DMAy_Streamx->FCR & DMA_SxFCR_FS));
  
  return tmpreg;
}


#ifdef __cplusplus
}
#endif

#endif /*__STM32F4xx_DMA_H */
