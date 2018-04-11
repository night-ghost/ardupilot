/**
  ******************************************************************************
  * @file    usbd_msc_data.h
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

#ifndef _USBD_MSC_DATA_H_
#define _USBD_MSC_DATA_H_

/* Includes ------------------------------------------------------------------*/
#include "usbd_conf.h"

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */
  
/** @defgroup USB_INFO
  * @brief general defines for the usb device library file
  * @{
  */ 

/** @defgroup USB_INFO_Exported_Defines
  * @{
  */ 
#define MODE_SENSE6_LEN			 8
#define MODE_SENSE10_LEN		 8
#define LENGTH_INQUIRY_PAGE00		 7
#define LENGTH_FORMAT_CAPACITIES    	20

/**
  * @}
  */ 


/** @defgroup USBD_INFO_Exported_TypesDefinitions
  * @{
  */
/**
  * @}
  */ 



/** @defgroup USBD_INFO_Exported_Macros
  * @{
  */ 

/**
  * @}
  */ 

/** @defgroup USBD_INFO_Exported_Variables
  * @{
  */ 
extern const uint8_t MSC_Page00_Inquiry_Data[];  
extern const uint8_t MSC_Mode_Sense6_data[];
extern const uint8_t MSC_Mode_Sense10_data[] ;

/**
  * @}
  */ 

/** @defgroup USBD_INFO_Exported_FunctionsPrototype
  * @{
  */ 

/**
  * @}
  */ 

#endif /* _USBD_MSC_DATA_H_ */

/**
  * @}
  */ 

/**
* @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
