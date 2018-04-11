/**
  ******************************************************************************
  * @file    usbd_msc_data.c
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

/* Includes ------------------------------------------------------------------*/
#include "usbd_msc_data.h"


/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */


/** @defgroup MSC_DATA 
  * @brief Mass storage info/data module
  * @{
  */ 

/** @defgroup MSC_DATA_Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup MSC_DATA_Private_Defines
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup MSC_DATA_Private_Macros
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup MSC_DATA_Private_Variables
  * @{
  */ 


/* USB Mass storage Page 0 Inquiry Data */
const uint8_t  MSC_Page00_Inquiry_Data[] = {//7						
	0x00,		
	0x00, 
	0x00, 
	(LENGTH_INQUIRY_PAGE00 - 4),
	0x00, 
	0x80, 
	0x83 
};  
/* USB Mass storage sense 6  Data */
const uint8_t  MSC_Mode_Sense6_data[] = {
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00, 
	0x00,
	0x00
};	
/* USB Mass storage sense 10  Data */
const uint8_t  MSC_Mode_Sense10_data[] = {
	0x00,
	0x06, 
	0x00, 
	0x00, 
	0x00, 
	0x00, 
	0x00, 
	0x00
};
/**
  * @}
  */ 


/** @defgroup MSC_DATA_Private_FunctionPrototypes
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup MSC_DATA_Private_Functions
  * @{
  */ 

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
