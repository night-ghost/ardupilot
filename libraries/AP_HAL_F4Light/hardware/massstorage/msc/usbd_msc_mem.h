/**
  ******************************************************************************
  * @file    usbd_msc_mem.h
  * @version        : v1.0_Cube
  * @brief          : This file implements the USB Device descriptors
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  * 1. Redistributions of source code must retain the above copyright notice,
  * this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  * this list of conditions and the following disclaimer in the documentation
  * and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of its contributors
  * may be used to endorse or promote products derived from this software
  * without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __USBD_MEM_H
#define __USBD_MEM_H
/* Includes ------------------------------------------------------------------*/
#include "usbd_def.h"


/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */
  
/** @defgroup USBD_MEM
  * @brief header file for the storage disk file
  * @{
  */ 

/** @defgroup USBD_MEM_Exported_Defines
  * @{
  */ 
#define USBD_STD_INQUIRY_LENGTH		36
/**
  * @}
  */ 


/** @defgroup USBD_MEM_Exported_TypesDefinitions
  * @{
  */

typedef struct _USBD_STORAGE
{
  int8_t (* Init) (uint8_t lun);
  int8_t (* GetCapacity) (uint8_t lun, uint32_t *block_num, uint32_t *block_size);
  int8_t (* IsReady) (uint8_t lun);
  int8_t (* IsWriteProtected) (uint8_t lun);
  int8_t (* Read) (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
  int8_t (* Write)(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
  int8_t (* GetMaxLun)(void);
  int8_t *pInquiry;
  
}USBD_STORAGE_cb_TypeDef;
/**
  * @}
  */ 



/** @defgroup USBD_MEM_Exported_Macros
  * @{
  */ 

/**
  * @}
  */ 

/** @defgroup USBD_MEM_Exported_Variables
  * @{
  */ 

/**
  * @}
  */ 

/** @defgroup USBD_MEM_Exported_FunctionsPrototype
  * @{
  */ 
extern const USBD_STORAGE_cb_TypeDef * const USBD_STORAGE_fops;
/**
  * @}
  */ 

#endif /* __USBD_MEM_H */
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
