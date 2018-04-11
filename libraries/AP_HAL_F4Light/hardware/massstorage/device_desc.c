/**
(c) 2017 night_ghost@ykoctpa.ru
 
based on:

  ******************************************************************************
  * @file           : usbd_desc.c
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
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_req.h"
#include "usbd_conf.h"
#include "usb_regs.h"

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_DESC 
  * @brief USBD descriptors module
  * @{
  */ 

/** @defgroup USBD_DESC_Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup USBD_DESC_Private_Defines
  * @{
  */ 

#define USBD_VID                   0x0483
#define USBD_PID                   0x5720

#define USBD_LANGID_STRING         0x409
#define USBD_MANUFACTURER_STRING   "STMicroelectronics"


#define USBD_PRODUCT_HS_STRING        "Mass Storage in HS Mode"
#define USBD_SERIALNUMBER_HS_STRING   "00000000001A"
#define USBD_PRODUCT_FS_STRING        "Mass Storage in FS Mode"
#define USBD_SERIALNUMBER_FS_STRING   "00000000001B"
#define USBD_CONFIGURATION_HS_STRING  "MSC Config"
#define USBD_INTERFACE_HS_STRING      "MSC Interface"
#define USBD_CONFIGURATION_FS_STRING  "MSC Config"
#define USBD_INTERFACE_FS_STRING      "MSC Interface"
/**
  * @}
  */ 


/** @defgroup USBD_DESC_Private_Macros
  * @{
  */ 
/**
  * @}
  */ 


uint8_t *     USBD_MSC_DeviceDescriptor( uint8_t speed , uint16_t *length);
uint8_t *     USBD_MSC_LangIDStrDescriptor( uint8_t speed , uint16_t *length);
uint8_t *     USBD_MSC_ManufacturerStrDescriptor ( uint8_t speed , uint16_t *length);
uint8_t *     USBD_MSC_ProductStrDescriptor ( uint8_t speed , uint16_t *length);
uint8_t *     USBD_MSC_SerialStrDescriptor( uint8_t speed , uint16_t *length);
uint8_t *     USBD_MSC_ConfigStrDescriptor( uint8_t speed , uint16_t *length);
uint8_t *     USBD_MSC_InterfaceStrDescriptor( uint8_t speed , uint16_t *length);

#ifdef USB_SUPPORT_USER_STRING_DESC
uint8_t *     USBD_MSC_USRStringDesc (uint8_t speed, uint8_t idx , uint16_t *length);
#endif /* USB_SUPPORT_USER_STRING_DESC */  

/** @defgroup USBD_DESC_Private_Variables
  * @{
  */ 

const USBD_DEVICE USR_MSC_desc =
{
  USBD_MSC_DeviceDescriptor,
  USBD_MSC_LangIDStrDescriptor, 
  USBD_MSC_ManufacturerStrDescriptor,
  USBD_MSC_ProductStrDescriptor,
  USBD_MSC_SerialStrDescriptor,
  USBD_MSC_ConfigStrDescriptor,
  USBD_MSC_InterfaceStrDescriptor,
  
};

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
/* USB Standard Device Descriptor */
const __ALIGN_BEGIN uint8_t USBD_MSC_DeviceDesc[USB_SIZ_DEVICE_DESC] __ALIGN_END =
  {
    0x12,                       /*bLength */
    USB_DEVICE_DESCRIPTOR_TYPE, /*bDescriptorType*/
    0x00,                       /*bcdUSB */
    0x02,
    0x00,                       /*bDeviceClass*/
    0x00,                       /*bDeviceSubClass*/
    0x00,                       /*bDeviceProtocol*/
    USB_OTG_MAX_EP0_SIZE,      /*bMaxPacketSize*/
    LOBYTE(USBD_VID),           /*idVendor*/
    HIBYTE(USBD_VID),           /*idVendor*/
    LOBYTE(USBD_PID),           /*idVendor*/
    HIBYTE(USBD_PID),           /*idVendor*/
    0x00,                       /*bcdDevice rel. 2.00*/
    0x02,
    USBD_IDX_MFC_STR,           /*Index of manufacturer  string*/
    USBD_IDX_PRODUCT_STR,       /*Index of product string*/
    USBD_IDX_SERIAL_STR,        /*Index of serial number string*/
    USBD_CFG_MAX_NUM            /*bNumConfigurations*/
  } ; /* USB_DeviceDescriptor */

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
/* USB Standard Device Descriptor */
__ALIGN_BEGIN uint8_t USBD_MSC_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
/* USB Standard Device Descriptor */
const __ALIGN_BEGIN uint8_t USBD_MSC_LangIDDesc[USB_SIZ_STRING_LANGID] __ALIGN_END =
{
     USB_SIZ_STRING_LANGID,         
     USB_DESC_TYPE_STRING,       
     LOBYTE(USBD_LANGID_STRING),
     HIBYTE(USBD_LANGID_STRING), 
};
/**
  * @}
  */ 


/** @defgroup USBD_DESC_Private_FunctionPrototypes
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup USBD_DESC_Private_Functions
  * @{
  */ 

/**
* @brief  USBD_MSC_DeviceDescriptor 
*         return the device descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_MSC_DeviceDescriptor( uint8_t speed , uint16_t *length)
{
  *length = sizeof(USBD_MSC_DeviceDesc);
  return (uint8_t *)USBD_MSC_DeviceDesc;
}

/**
* @brief  USBD_MSC_LangIDStrDescriptor 
*         return the LangID string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_MSC_LangIDStrDescriptor( uint8_t speed , uint16_t *length)
{
  *length =  sizeof(USBD_MSC_LangIDDesc);  
  return (uint8_t *)USBD_MSC_LangIDDesc;
}


/**
* @brief  USBD_MSC_ProductStrDescriptor 
*         return the product string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_MSC_ProductStrDescriptor( uint8_t speed , uint16_t *length)
{
 
  
  if(speed == 0)
  {   
    USBD_GetString ((uint8_t *)USBD_PRODUCT_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString ((uint8_t *)USBD_PRODUCT_FS_STRING, USBD_StrDesc, length);    
  }
  return USBD_StrDesc;
}

/**
* @brief  USBD_MSC_ManufacturerStrDescriptor 
*         return the manufacturer string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_MSC_ManufacturerStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString ((uint8_t *)USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
* @brief  USBD_MSC_SerialStrDescriptor 
*         return the serial number string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_MSC_SerialStrDescriptor( uint8_t speed , uint16_t *length)
{
  if(speed  == USB_OTG_SPEED_HIGH)
  {    
    USBD_GetString ((uint8_t *)USBD_SERIALNUMBER_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString ((uint8_t *)USBD_SERIALNUMBER_FS_STRING, USBD_StrDesc, length);    
  }
  return USBD_StrDesc;
}

/**
* @brief  USBD_MSC_ConfigStrDescriptor 
*         return the configuration string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_MSC_ConfigStrDescriptor( uint8_t speed , uint16_t *length)
{
  if(speed  == USB_OTG_SPEED_HIGH)
  {  
    USBD_GetString ((uint8_t *)USBD_CONFIGURATION_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString ((uint8_t *)USBD_CONFIGURATION_FS_STRING, USBD_StrDesc, length); 
  }
  return USBD_StrDesc;  
}


/**
* @brief  USBD_MSC_InterfaceStrDescriptor 
*         return the interface string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_MSC_InterfaceStrDescriptor( uint8_t speed , uint16_t *length)
{
  if(speed == 0)
  {
    USBD_GetString ((uint8_t *)USBD_INTERFACE_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString ((uint8_t *)USBD_INTERFACE_FS_STRING, USBD_StrDesc, length);
  }
  return USBD_StrDesc;  
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

