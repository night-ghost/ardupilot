/**
  ******************************************************************************
  * @file    usbd_msc_bot.h
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

#include "usbd_core.h"

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_MSC_BOT_H
#define __USBD_MSC_BOT_H

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */
  
/** @defgroup MSC_BOT
  * @brief This file is the Header file for usbd_bot.c
  * @{
  */ 


/** @defgroup USBD_CORE_Exported_Defines
  * @{
  */ 
 enum BOT_STATE {
    BOT_IDLE           =           0,       /* Idle state */
    BOT_DATA_OUT       =           1,       /* Data Out state */
    BOT_DATA_IN        =           2,       /* Data In state */
    BOT_LAST_DATA_IN   =           3,       /* Last Data In Last */
    BOT_SEND_DATA      =           4,       /* Send Immediate data */
};

#define BOT_CBW_SIGNATURE             0x43425355
#define BOT_CSW_SIGNATURE             0x53425355
#define BOT_CBW_LENGTH                31
#define BOT_CSW_LENGTH                13

/* CSW Status Definitions */
#define CSW_CMD_PASSED                0x00
#define CSW_CMD_FAILED                0x01
#define CSW_PHASE_ERROR               0x02

/* BOT Status */
#define BOT_STATE_NORMAL              0
#define BOT_STATE_RECOVERY            1
#define BOT_STATE_ERROR               2


#define DIR_IN                        0
#define DIR_OUT                       1
#define BOTH_DIR                      2

/**
  * @}
  */ 

/** @defgroup MSC_CORE_Private_TypesDefinitions
  * @{
  */ 

typedef struct _MSC_BOT_CBW
{
  uint32_t dSignature;
  uint32_t dTag;
  uint32_t dDataLength;
  uint8_t  bmFlags;
  uint8_t  bLUN;
  uint8_t  bCBLength;
  uint8_t  CB[16];
}
MSC_BOT_CBW_TypeDef;


typedef struct _MSC_BOT_CSW
{
  uint32_t dSignature;
  uint32_t dTag;
  uint32_t dDataResidue;
  uint8_t  bStatus;
}
MSC_BOT_CSW_TypeDef;

/**
  * @}
  */ 


/** @defgroup USBD_CORE_Exported_Types
  * @{
  */

extern uint8_t              MSC_BOT_Data[];
extern uint16_t             MSC_BOT_DataLen;
extern uint8_t              MSC_BOT_State;
extern uint8_t              MSC_BOT_BurstMode;
extern MSC_BOT_CBW_TypeDef  MSC_BOT_cbw;
extern MSC_BOT_CSW_TypeDef  MSC_BOT_csw;
/**
  * @}
  */ 
/** @defgroup USBD_CORE_Exported_FunctionsPrototypes
  * @{
  */ 
void MSC_BOT_Init (USB_OTG_CORE_HANDLE  *pdev);
void MSC_BOT_Reset (USB_OTG_CORE_HANDLE  *pdev);
void MSC_BOT_DeInit (USB_OTG_CORE_HANDLE  *pdev);
void MSC_BOT_DataIn (USB_OTG_CORE_HANDLE  *pdev, 
                     uint8_t epnum);

void MSC_BOT_DataOut (USB_OTG_CORE_HANDLE  *pdev, 
                      uint8_t epnum);

void MSC_BOT_SendCSW (USB_OTG_CORE_HANDLE  *pdev,
                             uint8_t CSW_Status);

void  MSC_BOT_CplClrFeature (USB_OTG_CORE_HANDLE  *pdev, 
                             uint8_t epnum);
void  MSC_BOT_CBW_finish (USB_OTG_CORE_HANDLE  *pdev);
/**
  * @}
  */ 

#endif /* __USBD_MSC_BOT_H */
/**
  * @}
  */ 

/**
* @}
*/ 
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

