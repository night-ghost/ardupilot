#pragma GCC optimize ("O2")

/**
  ******************************************************************************
  * @file    stm32f4xx_spi.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    11-January-2013
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of the Serial peripheral interface (SPI):
  *           + Initialization and Configuration
  *           + Data transfers functions
  *           + Hardware CRC Calculation
  *           + DMA transfers management
  *           + Interrupts and flags management 
  *           
@verbatim

 ===================================================================
                  ##### How to use this driver #####
 ===================================================================
 [..]
   (#) Enable peripheral clock using the following functions 
       RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE) for SPI1
       RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE) for SPI2
       RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3, ENABLE) for SPI3
       RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3, ENABLE) for SPI4
       RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3, ENABLE) for SPI5
       RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3, ENABLE) for SPI6.
  
   (#) Enable SCK, MOSI, MISO and NSS GPIO clocks using RCC_AHB1PeriphClockCmd()
       function. In I2S mode, if an external clock source is used then the I2S 
       CKIN pin GPIO clock should also be enabled.
  
   (#) Peripherals alternate function: 
       (++) Connect the pin to the desired peripherals' Alternate Function (AF) 
            using GPIO_PinAFConfig() function
       (++) Configure the desired pin in alternate function by: 
            GPIO_InitStruct->GPIO_Mode = GPIO_Mode_AF
       (++) Select the type, pull-up/pull-down and output speed via GPIO_PuPd, 
            GPIO_OType and GPIO_Speed members
       (++) Call GPIO_Init() function In I2S mode, if an external clock source is 
            used then the I2S CKIN pin should be also configured in Alternate 
            function Push-pull pull-up mode. 
          
   (#) Program the Polarity, Phase, First Data, Baud Rate Prescaler, Slave 
       Management, Peripheral Mode and CRC Polynomial values using the SPI_Init()
       function.
       In I2S mode, program the Mode, Standard, Data Format, MCLK Output, Audio 
       frequency and Polarity using I2S_Init() function. For I2S mode, make sure 
       that either:
       (++) I2S PLL is configured using the functions 
            RCC_I2SCLKConfig(RCC_I2S2CLKSource_PLLI2S), RCC_PLLI2SCmd(ENABLE) and 
            RCC_GetFlagStatus(RCC_FLAG_PLLI2SRDY); or 
       (++) External clock source is configured using the function 
            RCC_I2SCLKConfig(RCC_I2S2CLKSource_Ext) and after setting correctly 
            the define constant I2S_EXTERNAL_CLOCK_VAL in the stm32f4xx_conf.h file. 
  
   (#) Enable the NVIC and the corresponding interrupt using the function 
       SPI_ITConfig() if you need to use interrupt mode. 
  
   (#) When using the DMA mode 
       (++) Configure the DMA using DMA_Init() function
       (++) Active the needed channel Request using SPI_I2S_DMACmd() function
   
   (#) Enable the SPI using the SPI_Cmd() function or enable the I2S using
       I2S_Cmd().
   
   (#) Enable the DMA using the DMA_Cmd() function when using DMA mode. 
  
   (#) Optionally, you can enable/configure the following parameters without
       re-initialization (i.e there is no need to call again SPI_Init() function):
       (++) When bidirectional mode (SPI_Direction_1Line_Rx or SPI_Direction_1Line_Tx)
            is programmed as Data direction parameter using the SPI_Init() function
            it can be possible to switch between SPI_Direction_Tx or SPI_Direction_Rx
            using the SPI_BiDirectionalLineConfig() function.
       (++) When SPI_NSS_Soft is selected as Slave Select Management parameter 
            using the SPI_Init() function it can be possible to manage the 
            NSS internal signal using the SPI_NSSInternalSoftwareConfig() function.
       (++) Reconfigure the data size using the SPI_DataSizeConfig() function  
       (++) Enable or disable the SS output using the SPI_SSOutputCmd() function  
            
    (#) To use the CRC Hardware calculation feature refer to the Peripheral 
        CRC hardware Calculation subsection.
     
  
 [..] It is possible to use SPI in I2S full duplex mode, in this case, each SPI 
      peripheral is able to manage sending and receiving data simultaneously
      using two data lines. Each SPI peripheral has an extended block called I2Sxext
      (ie. I2S2ext for SPI2 and I2S3ext for SPI3).
      The extension block is not a full SPI IP, it is used only as I2S slave to
      implement full duplex mode. The extension block uses the same clock sources
      as its master.          
      To configure I2S full duplex you have to:
              
      (#) Configure SPIx in I2S mode (I2S_Init() function) as described above. 
             
      (#) Call the I2S_FullDuplexConfig() function using the same strucutre passed to  
          I2S_Init() function.
              
      (#) Call I2S_Cmd() for SPIx then for its extended block.
            
      (#) To configure interrupts or DMA requests and to get/clear flag status, 
          use I2Sxext instance for the extension block.
               
 [..] Functions that can be called with I2Sxext instances are: I2S_Cmd(), 
      I2S_FullDuplexConfig(), SPI_I2S_ReceiveData(), SPI_I2S_SendData(), 
      SPI_I2S_DMACmd(), SPI_I2S_ITConfig(), SPI_I2S_GetFlagStatus(), 
      SPI_I2S_ClearFlag(), SPI_I2S_GetITStatus() and SPI_I2S_ClearITPendingBit().
                   
      Example: To use SPI3 in Full duplex mode (SPI3 is Master Tx, I2S3ext is Slave Rx):
              
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);   
      I2S_StructInit(&I2SInitStruct);
      I2SInitStruct.Mode = I2S_Mode_MasterTx;     
      I2S_Init(SPI3, &I2SInitStruct);
      I2S_FullDuplexConfig(SPI3ext, &I2SInitStruct)
      I2S_Cmd(SPI3, ENABLE);
      I2S_Cmd(SPI3ext, ENABLE);
      ...
      while (SPI_I2S_GetFlagStatus(SPI2, SPI_FLAG_TXE) == RESET)
      {}
      SPI_I2S_SendData(SPI3, txdata[i]);
      ...  
      while (SPI_I2S_GetFlagStatus(I2S3ext, SPI_FLAG_RXNE) == RESET)
      {}
      rxdata[i] = SPI_I2S_ReceiveData(I2S3ext);
      ...          
                
 [..]       
   (@) In I2S mode: if an external clock is used as source clock for the I2S,  
       then the define I2S_EXTERNAL_CLOCK_VAL in file stm32f4xx_conf.h should 
       be enabled and set to the value of the source clock frequency (in Hz).
   
   (@) In SPI mode: To use the SPI TI mode, call the function SPI_TIModeCmd() 
       just after calling the function SPI_Init().
  
@endverbatim  
  *                                  
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

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_spi.h"
#include "stm32f4xx_rcc.h"

/** @addtogroup STM32F4xx_StdPeriph_Driver
  * @{
  */

/** @defgroup SPI 
  * @brief SPI driver modules
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup SPI_Private_Functions
  * @{
  */

/** @defgroup SPI_Group1 Initialization and Configuration functions
 *  @brief   Initialization and Configuration functions 
 *
@verbatim   
 ===============================================================================
             ##### Initialization and Configuration functions ##### 
 ===============================================================================  
 [..] This section provides a set of functions allowing to initialize the SPI 
      Direction, SPI Mode, SPI Data Size, SPI Polarity, SPI Phase, SPI NSS 
      Management, SPI Baud Rate Prescaler, SPI First Bit and SPI CRC Polynomial.
  
 [..] The SPI_Init() function follows the SPI configuration procedures for Master 
      mode and Slave mode (details for these procedures are available in reference 
      manual (RM0090)).
  
@endverbatim
  * @{
  */

/**
  * @brief  De-initialize the SPIx peripheral registers to their default reset values.
  * @param  SPIx: To select the SPIx/I2Sx peripheral, where x can be: 1, 2, 3, 4, 5 or 6 
  *         in SPI mode or 2 or 3 in I2S mode.   
  *         
  * @note   The extended I2S blocks (ie. I2S2ext and I2S3ext blocks) are de-initialized
  *         when the relative I2S peripheral is de-initialized (the extended block's clock
  *         is managed by the I2S peripheral clock).
  *             
  * @retval None
  */
void SPI_I2S_DeInit(SPI_TypeDef* SPIx)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));

  if (SPIx == SPI1)
  {
    /* Enable SPI1 reset state */
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1, ENABLE);
    /* Release SPI1 from reset state */
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1, DISABLE);
  }
  else if (SPIx == SPI2)
  {
    /* Enable SPI2 reset state */
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2, ENABLE);
    /* Release SPI2 from reset state */
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2, DISABLE);
  }
  else if (SPIx == SPI3)
  {
    /* Enable SPI3 reset state */
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3, ENABLE);
    /* Release SPI3 from reset state */
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3, DISABLE);
  }
  else if (SPIx == SPI4)
  {
    /* Enable SPI4 reset state */
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI4, ENABLE);
    /* Release SPI4 from reset state */
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI4, DISABLE);
  }
  else if (SPIx == SPI5)
  {
    /* Enable SPI5 reset state */
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI5, ENABLE);
    /* Release SPI5 from reset state */
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI5, DISABLE);
  }
  else 
  {
    if (SPIx == SPI6)
    {
      /* Enable SPI6 reset state */
      RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI6, ENABLE);
      /* Release SPI6 from reset state */
      RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI6, DISABLE);
    }
  }
}

/**
  * @brief  Initializes the SPIx peripheral according to the specified 
  *         parameters in the SPI_InitStruct.
  * @param  SPIx: where x can be 1, 2, 3, 4, 5 or 6 to select the SPI peripheral.
  * @param  SPI_InitStruct: pointer to a SPI_InitTypeDef structure that
  *         contains the configuration information for the specified SPI peripheral.
  * @retval None
  */
void SPI_Init(SPI_TypeDef* SPIx, SPI_InitTypeDef* SPI_InitStruct)
{
  uint16_t tmpreg = 0;
  
  /* check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  
  /* Check the SPI parameters */
  assert_param(IS_SPI_DIRECTION_MODE(SPI_InitStruct->SPI_Direction));
  assert_param(IS_SPI_MODE(SPI_InitStruct->SPI_Mode));
  assert_param(IS_SPI_DATASIZE(SPI_InitStruct->SPI_DataSize));
  assert_param(IS_SPI_CPOL(SPI_InitStruct->SPI_CPOL));
  assert_param(IS_SPI_CPHA(SPI_InitStruct->SPI_CPHA));
  assert_param(IS_SPI_NSS(SPI_InitStruct->SPI_NSS));
  assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_InitStruct->SPI_BaudRatePrescaler));
  assert_param(IS_SPI_FIRST_BIT(SPI_InitStruct->SPI_FirstBit));
  assert_param(IS_SPI_CRC_POLYNOMIAL(SPI_InitStruct->SPI_CRCPolynomial));

/*---------------------------- SPIx CR1 Configuration ------------------------*/
  /* Get the SPIx CR1 value */
  tmpreg = SPIx->CR1;
  /* Clear BIDIMode, BIDIOE, RxONLY, SSM, SSI, LSBFirst, BR, MSTR, CPOL and CPHA bits */
  tmpreg &= CR1_CLEAR_MASK;
  /* Configure SPIx: direction, NSS management, first transmitted bit, BaudRate prescaler
     master/salve mode, CPOL and CPHA */
  /* Set BIDImode, BIDIOE and RxONLY bits according to SPI_Direction value */
  /* Set SSM, SSI and MSTR bits according to SPI_Mode and SPI_NSS values */
  /* Set LSBFirst bit according to SPI_FirstBit value */
  /* Set BR bits according to SPI_BaudRatePrescaler value */
  /* Set CPOL bit according to SPI_CPOL value */
  /* Set CPHA bit according to SPI_CPHA value */
  tmpreg |= (uint16_t)((uint32_t)SPI_InitStruct->SPI_Direction | SPI_InitStruct->SPI_Mode |
                  SPI_InitStruct->SPI_DataSize | SPI_InitStruct->SPI_CPOL |  
                  SPI_InitStruct->SPI_CPHA | SPI_InitStruct->SPI_NSS |  
                  SPI_InitStruct->SPI_BaudRatePrescaler | SPI_InitStruct->SPI_FirstBit);
  /* Write to SPIx CR1 */
  SPIx->CR1 = tmpreg;

  /* Activate the SPI mode (Reset I2SMOD bit in I2SCFGR register) */
  SPIx->I2SCFGR &= (uint16_t)~((uint16_t)SPI_I2SCFGR_I2SMOD);
/*---------------------------- SPIx CRCPOLY Configuration --------------------*/
  /* Write to SPIx CRCPOLY */
  SPIx->CRCPR = SPI_InitStruct->SPI_CRCPolynomial;
}

/**
  * @brief  Initializes the SPIx peripheral according to the specified 
  *         parameters in the I2S_InitStruct.
  * @param  SPIx: where x can be  2 or 3 to select the SPI peripheral (configured in I2S mode).
  * @param  I2S_InitStruct: pointer to an I2S_InitTypeDef structure that
  *         contains the configuration information for the specified SPI peripheral
  *         configured in I2S mode.
  *           
  * @note   The function calculates the optimal prescaler needed to obtain the most 
  *         accurate audio frequency (depending on the I2S clock source, the PLL values 
  *         and the product configuration). But in case the prescaler value is greater 
  *         than 511, the default value (0x02) will be configured instead.    
  * 
  * @note   if an external clock is used as source clock for the I2S, then the define
  *         I2S_EXTERNAL_CLOCK_VAL in file stm32f4xx_conf.h should be enabled and set
  *         to the value of the the source clock frequency (in Hz).
  *  
  * @retval None
  */
void I2S_Init(SPI_TypeDef* SPIx, I2S_InitTypeDef* I2S_InitStruct)
{
  uint16_t tmpreg = 0, i2sdiv = 2, i2sodd = 0, packetlength = 1;
  uint32_t tmp = 0, i2sclk = 0;
#ifndef I2S_EXTERNAL_CLOCK_VAL
  uint32_t pllm = 0, plln = 0, pllr = 0;
#endif /* I2S_EXTERNAL_CLOCK_VAL */
  
  /* Check the I2S parameters */
  assert_param(IS_SPI_23_PERIPH(SPIx));
  assert_param(IS_I2S_MODE(I2S_InitStruct->I2S_Mode));
  assert_param(IS_I2S_STANDARD(I2S_InitStruct->I2S_Standard));
  assert_param(IS_I2S_DATA_FORMAT(I2S_InitStruct->I2S_DataFormat));
  assert_param(IS_I2S_MCLK_OUTPUT(I2S_InitStruct->I2S_MCLKOutput));
  assert_param(IS_I2S_AUDIO_FREQ(I2S_InitStruct->I2S_AudioFreq));
  assert_param(IS_I2S_CPOL(I2S_InitStruct->I2S_CPOL));  

/*----------------------- SPIx I2SCFGR & I2SPR Configuration -----------------*/
  /* Clear I2SMOD, I2SE, I2SCFG, PCMSYNC, I2SSTD, CKPOL, DATLEN and CHLEN bits */
  SPIx->I2SCFGR &= I2SCFGR_CLEAR_MASK; 
  SPIx->I2SPR = 0x0002;
  
  /* Get the I2SCFGR register value */
  tmpreg = SPIx->I2SCFGR;
  
  /* If the default value has to be written, reinitialize i2sdiv and i2sodd*/
  if(I2S_InitStruct->I2S_AudioFreq == I2S_AudioFreq_Default)
  {
    i2sodd = (uint16_t)0;
    i2sdiv = (uint16_t)2;   
  }
  /* If the requested audio frequency is not the default, compute the prescaler */
  else
  {
    /* Check the frame length (For the Prescaler computing) *******************/
    if(I2S_InitStruct->I2S_DataFormat == I2S_DataFormat_16b)
    {
      /* Packet length is 16 bits */
      packetlength = 1;
    }
    else
    {
      /* Packet length is 32 bits */
      packetlength = 2;
    }

    /* Get I2S source Clock frequency  ****************************************/
      
    /* If an external I2S clock has to be used, this define should be set  
       in the project configuration or in the stm32f4xx_conf.h file */
  #ifdef I2S_EXTERNAL_CLOCK_VAL     
    /* Set external clock as I2S clock source */
    if ((RCC->CFGR & RCC_CFGR_I2SSRC) == 0)
    {
      RCC->CFGR |= (uint32_t)RCC_CFGR_I2SSRC;
    }
    
    /* Set the I2S clock to the external clock  value */
    i2sclk = I2S_EXTERNAL_CLOCK_VAL;

  #else /* There is no define for External I2S clock source */
    /* Set PLLI2S as I2S clock source */
    if ((RCC->CFGR & RCC_CFGR_I2SSRC) != 0)
    {
      RCC->CFGR &= ~(uint32_t)RCC_CFGR_I2SSRC;
    }    
    
    /* Get the PLLI2SN value */
    plln = (uint32_t)(((RCC->PLLI2SCFGR & RCC_PLLI2SCFGR_PLLI2SN) >> 6) & \
                      (RCC_PLLI2SCFGR_PLLI2SN >> 6));
    
    /* Get the PLLI2SR value */
    pllr = (uint32_t)(((RCC->PLLI2SCFGR & RCC_PLLI2SCFGR_PLLI2SR) >> 28) & \
                      (RCC_PLLI2SCFGR_PLLI2SR >> 28));
    
    /* Get the PLLM value */
    pllm = (uint32_t)(RCC->PLLCFGR & RCC_PLLCFGR_PLLM);      

    /* Get the I2S source clock value */
    i2sclk = (uint32_t)(((HSE_VALUE / pllm) * plln) / pllr);
  #endif /* I2S_EXTERNAL_CLOCK_VAL */
    
    /* Compute the Real divider depending on the MCLK output state, with a floating point */
    if(I2S_InitStruct->I2S_MCLKOutput == I2S_MCLKOutput_Enable)
    {
      /* MCLK output is enabled */
      tmp = (uint16_t)(((((i2sclk / 256) * 10) / I2S_InitStruct->I2S_AudioFreq)) + 5);
    }
    else
    {
      /* MCLK output is disabled */
      tmp = (uint16_t)(((((i2sclk / (32 * packetlength)) *10 ) / I2S_InitStruct->I2S_AudioFreq)) + 5);
    }
    
    /* Remove the flatting point */
    tmp = tmp / 10;  
      
    /* Check the parity of the divider */
    i2sodd = (uint16_t)(tmp & (uint16_t)0x0001);
   
    /* Compute the i2sdiv prescaler */
    i2sdiv = (uint16_t)((tmp - i2sodd) / 2);
   
    /* Get the Mask for the Odd bit (SPI_I2SPR[8]) register */
    i2sodd = (uint16_t) (i2sodd << 8);
  }

  /* Test if the divider is 1 or 0 or greater than 0xFF */
  if ((i2sdiv < 2) || (i2sdiv > 0xFF))
  {
    /* Set the default values */
    i2sdiv = 2;
    i2sodd = 0;
  }

  /* Write to SPIx I2SPR register the computed value */
  SPIx->I2SPR = (uint16_t)((uint16_t)i2sdiv | (uint16_t)(i2sodd | (uint16_t)I2S_InitStruct->I2S_MCLKOutput));
 
  /* Configure the I2S with the SPI_InitStruct values */
  tmpreg |= (uint16_t)((uint16_t)SPI_I2SCFGR_I2SMOD | (uint16_t)(I2S_InitStruct->I2S_Mode | \
                  (uint16_t)(I2S_InitStruct->I2S_Standard | (uint16_t)(I2S_InitStruct->I2S_DataFormat | \
                  (uint16_t)I2S_InitStruct->I2S_CPOL))));
 
  /* Write to SPIx I2SCFGR */  
  SPIx->I2SCFGR = tmpreg;
}

/**
  * @brief  Fills each SPI_InitStruct member with its default value.
  * @param  SPI_InitStruct: pointer to a SPI_InitTypeDef structure which will be initialized.
  * @retval None
  */
void SPI_StructInit(SPI_InitTypeDef* SPI_InitStruct)
{
/*--------------- Reset SPI init structure parameters values -----------------*/
  /* Initialize the SPI_Direction member */
  SPI_InitStruct->SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  /* initialize the SPI_Mode member */
  SPI_InitStruct->SPI_Mode = SPI_Mode_Slave;
  /* initialize the SPI_DataSize member */
  SPI_InitStruct->SPI_DataSize = SPI_DataSize_8b;
  /* Initialize the SPI_CPOL member */
  SPI_InitStruct->SPI_CPOL = SPI_CPOL_Low;
  /* Initialize the SPI_CPHA member */
  SPI_InitStruct->SPI_CPHA = SPI_CPHA_1Edge;
  /* Initialize the SPI_NSS member */
  SPI_InitStruct->SPI_NSS = SPI_NSS_Hard;
  /* Initialize the SPI_BaudRatePrescaler member */
  SPI_InitStruct->SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
  /* Initialize the SPI_FirstBit member */
  SPI_InitStruct->SPI_FirstBit = SPI_FirstBit_MSB;
  /* Initialize the SPI_CRCPolynomial member */
  SPI_InitStruct->SPI_CRCPolynomial = 7;
}

/**
  * @brief  Fills each I2S_InitStruct member with its default value.
  * @param  I2S_InitStruct: pointer to a I2S_InitTypeDef structure which will be initialized.
  * @retval None
  */
void I2S_StructInit(I2S_InitTypeDef* I2S_InitStruct)
{
/*--------------- Reset I2S init structure parameters values -----------------*/
  /* Initialize the I2S_Mode member */
  I2S_InitStruct->I2S_Mode = I2S_Mode_SlaveTx;
  
  /* Initialize the I2S_Standard member */
  I2S_InitStruct->I2S_Standard = I2S_Standard_Phillips;
  
  /* Initialize the I2S_DataFormat member */
  I2S_InitStruct->I2S_DataFormat = I2S_DataFormat_16b;
  
  /* Initialize the I2S_MCLKOutput member */
  I2S_InitStruct->I2S_MCLKOutput = I2S_MCLKOutput_Disable;
  
  /* Initialize the I2S_AudioFreq member */
  I2S_InitStruct->I2S_AudioFreq = I2S_AudioFreq_Default;
  
  /* Initialize the I2S_CPOL member */
  I2S_InitStruct->I2S_CPOL = I2S_CPOL_Low;
}

/**
  * @brief  Configures the full duplex mode for the I2Sx peripheral using its
  *         extension I2Sxext according to the specified parameters in the 
  *         I2S_InitStruct.
  * @param  I2Sxext: where x can be  2 or 3 to select the I2S peripheral extension block.
  * @param  I2S_InitStruct: pointer to an I2S_InitTypeDef structure that
  *         contains the configuration information for the specified I2S peripheral
  *         extension.
  * 
  * @note   The structure pointed by I2S_InitStruct parameter should be the same
  *         used for the master I2S peripheral. In this case, if the master is 
  *         configured as transmitter, the slave will be receiver and vice versa.
  *         Or you can force a different mode by modifying the field I2S_Mode to the
  *         value I2S_SlaveRx or I2S_SlaveTx indepedently of the master configuration.    
  *         
  * @note   The I2S full duplex extension can be configured in slave mode only.    
  *  
  * @retval None
  */
void I2S_FullDuplexConfig(SPI_TypeDef* I2Sxext, I2S_InitTypeDef* I2S_InitStruct)
{
  uint16_t tmpreg = 0, tmp = 0;
  
  /* Check the I2S parameters */
  assert_param(IS_I2S_EXT_PERIPH(I2Sxext));
  assert_param(IS_I2S_MODE(I2S_InitStruct->I2S_Mode));
  assert_param(IS_I2S_STANDARD(I2S_InitStruct->I2S_Standard));
  assert_param(IS_I2S_DATA_FORMAT(I2S_InitStruct->I2S_DataFormat));
  assert_param(IS_I2S_CPOL(I2S_InitStruct->I2S_CPOL));  

/*----------------------- SPIx I2SCFGR & I2SPR Configuration -----------------*/
  /* Clear I2SMOD, I2SE, I2SCFG, PCMSYNC, I2SSTD, CKPOL, DATLEN and CHLEN bits */
  I2Sxext->I2SCFGR &= I2SCFGR_CLEAR_MASK; 
  I2Sxext->I2SPR = 0x0002;
  
  /* Get the I2SCFGR register value */
  tmpreg = I2Sxext->I2SCFGR;
  
  /* Get the mode to be configured for the extended I2S */
  if ((I2S_InitStruct->I2S_Mode == I2S_Mode_MasterTx) || (I2S_InitStruct->I2S_Mode == I2S_Mode_SlaveTx))
  {
    tmp = I2S_Mode_SlaveRx;
  }
  else
  {
    if ((I2S_InitStruct->I2S_Mode == I2S_Mode_MasterRx) || (I2S_InitStruct->I2S_Mode == I2S_Mode_SlaveRx))
    {
      tmp = I2S_Mode_SlaveTx;
    }
  }

 
  /* Configure the I2S with the SPI_InitStruct values */
  tmpreg |= (uint16_t)((uint16_t)SPI_I2SCFGR_I2SMOD | (uint16_t)(tmp | \
                  (uint16_t)(I2S_InitStruct->I2S_Standard | (uint16_t)(I2S_InitStruct->I2S_DataFormat | \
                  (uint16_t)I2S_InitStruct->I2S_CPOL))));
 
  /* Write to SPIx I2SCFGR */  
  I2Sxext->I2SCFGR = tmpreg;
}

/**
  * @}
  */

/** @defgroup SPI_Group2 Data transfers functions
 *  @brief   Data transfers functions
 *
@verbatim   
 ===============================================================================
                      ##### Data transfers functions #####
 ===============================================================================  

 [..] This section provides a set of functions allowing to manage the SPI data 
      transfers. In reception, data are received and then stored into an internal 
      Rx buffer while. In transmission, data are first stored into an internal Tx 
      buffer before being transmitted.

 [..] The read access of the SPI_DR register can be done using the SPI_I2S_ReceiveData()
      function and returns the Rx buffered value. Whereas a write access to the SPI_DR 
      can be done using SPI_I2S_SendData() function and stores the written data into 
      Tx buffer.

@endverbatim
  * @{
  */

/**
  * @}
  */

/** @defgroup SPI_Group3 Hardware CRC Calculation functions
 *  @brief   Hardware CRC Calculation functions
 *
@verbatim   
 ===============================================================================
                 ##### Hardware CRC Calculation functions #####
 ===============================================================================  

 [..] This section provides a set of functions allowing to manage the SPI CRC hardware 
      calculation

 [..] SPI communication using CRC is possible through the following procedure:
   (#) Program the Data direction, Polarity, Phase, First Data, Baud Rate Prescaler, 
       Slave Management, Peripheral Mode and CRC Polynomial values using the SPI_Init()
       function.
   (#) Enable the CRC calculation using the SPI_CalculateCRC() function.
   (#) Enable the SPI using the SPI_Cmd() function
   (#) Before writing the last data to the TX buffer, set the CRCNext bit using the 
       SPI_TransmitCRC() function to indicate that after transmission of the last 
       data, the CRC should be transmitted.
   (#) After transmitting the last data, the SPI transmits the CRC. The SPI_CR1_CRCNEXT
        bit is reset. The CRC is also received and compared against the SPI_RXCRCR 
        value. 
        If the value does not match, the SPI_FLAG_CRCERR flag is set and an interrupt
        can be generated when the SPI_I2S_IT_ERR interrupt is enabled.

 [..]
   (@) It is advised not to read the calculated CRC values during the communication.

   (@) When the SPI is in slave mode, be careful to enable CRC calculation only 
       when the clock is stable, that is, when the clock is in the steady state. 
       If not, a wrong CRC calculation may be done. In fact, the CRC is sensitive 
       to the SCK slave input clock as soon as CRCEN is set, and this, whatever 
       the value of the SPE bit.

   (@) With high bitrate frequencies, be careful when transmitting the CRC.
       As the number of used CPU cycles has to be as low as possible in the CRC 
       transfer phase, it is forbidden to call software functions in the CRC 
       transmission sequence to avoid errors in the last data and CRC reception. 
       In fact, CRCNEXT bit has to be written before the end of the transmission/reception 
       of the last data.

   (@) For high bit rate frequencies, it is advised to use the DMA mode to avoid the
       degradation of the SPI speed performance due to CPU accesses impacting the 
       SPI bandwidth.

   (@) When the STM32F4xx is configured as slave and the NSS hardware mode is 
       used, the NSS pin needs to be kept low between the data phase and the CRC 
       phase.

   (@) When the SPI is configured in slave mode with the CRC feature enabled, CRC
       calculation takes place even if a high level is applied on the NSS pin. 
       This may happen for example in case of a multi-slave environment where the 
       communication master addresses slaves alternately.

   (@) Between a slave de-selection (high level on NSS) and a new slave selection 
       (low level on NSS), the CRC value should be cleared on both master and slave
       sides in order to resynchronize the master and slave for their respective 
       CRC calculation.

   (@) To clear the CRC, follow the procedure below:
       (#@) Disable SPI using the SPI_Cmd() function
       (#@) Disable the CRC calculation using the SPI_CalculateCRC() function.
       (#@) Enable the CRC calculation using the SPI_CalculateCRC() function.
       (#@) Enable SPI using the SPI_Cmd() function.

@endverbatim
  * @{
  */

/**
  * @brief  Enables or disables the CRC value calculation of the transferred bytes.
  * @param  SPIx: where x can be 1, 2, 3, 4, 5 or 6 to select the SPI peripheral.
  * @param  NewState: new state of the SPIx CRC value calculation.
  *          This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SPI_CalculateCRC(SPI_TypeDef* SPIx, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {
    /* Enable the selected SPI CRC calculation */
    SPIx->CR1 |= SPI_CR1_CRCEN;
  }
  else
  {
    /* Disable the selected SPI CRC calculation */
    SPIx->CR1 &= (uint16_t)~((uint16_t)SPI_CR1_CRCEN);
  }
}

/**
  * @brief  Transmit the SPIx CRC value.
  * @param  SPIx: where x can be 1, 2, 3, 4, 5 or 6 to select the SPI peripheral.
  * @retval None
  */
void SPI_TransmitCRC(SPI_TypeDef* SPIx)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  
  /* Enable the selected SPI CRC transmission */
  SPIx->CR1 |= SPI_CR1_CRCNEXT;
}

/**
  * @brief  Returns the transmit or the receive CRC register value for the specified SPI.
  * @param  SPIx: where x can be 1, 2, 3, 4, 5 or 6 to select the SPI peripheral.
  * @param  SPI_CRC: specifies the CRC register to be read.
  *          This parameter can be one of the following values:
  *            @arg SPI_CRC_Tx: Selects Tx CRC register
  *            @arg SPI_CRC_Rx: Selects Rx CRC register
  * @retval The selected CRC register value..
  */
uint16_t SPI_GetCRC(SPI_TypeDef* SPIx, uint8_t SPI_CRC)
{
  uint16_t crcreg = 0;
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_SPI_CRC(SPI_CRC));
  if (SPI_CRC != SPI_CRC_Rx)
  {
    /* Get the Tx CRC register */
    crcreg = SPIx->TXCRCR;
  }
  else
  {
    /* Get the Rx CRC register */
    crcreg = SPIx->RXCRCR;
  }
  /* Return the selected CRC register */
  return crcreg;
}

/**
  * @brief  Returns the CRC Polynomial register value for the specified SPI.
  * @param  SPIx: where x can be 1, 2, 3, 4, 5 or 6 to select the SPI peripheral.
  * @retval The CRC Polynomial register value.
  */
uint16_t SPI_GetCRCPolynomial(SPI_TypeDef* SPIx)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  
  /* Return the CRC polynomial register */
  return SPIx->CRCPR;
}

/**
  * @}
  */

/** @defgroup SPI_Group4 DMA transfers management functions
 *  @brief   DMA transfers management functions
  *
@verbatim   
 ===============================================================================
                   ##### DMA transfers management functions #####
 ===============================================================================  

@endverbatim
  * @{
  */

/**
  * @}
  */

/** @defgroup SPI_Group5 Interrupts and flags management functions
 *  @brief   Interrupts and flags management functions
  *
@verbatim   
 ===============================================================================
            ##### Interrupts and flags management functions #####
 ===============================================================================  
 
 [..] This section provides a set of functions allowing to configure the SPI Interrupts 
      sources and check or clear the flags or pending bits status.
      The user should identify which mode will be used in his application to manage 
      the communication: Polling mode, Interrupt mode or DMA mode. 
    
 *** Polling Mode ***
 ====================
[..] In Polling Mode, the SPI/I2S communication can be managed by 9 flags:
  (#) SPI_I2S_FLAG_TXE : to indicate the status of the transmit buffer register
  (#) SPI_I2S_FLAG_RXNE : to indicate the status of the receive buffer register
  (#) SPI_I2S_FLAG_BSY : to indicate the state of the communication layer of the SPI.
  (#) SPI_FLAG_CRCERR : to indicate if a CRC Calculation error occur              
  (#) SPI_FLAG_MODF : to indicate if a Mode Fault error occur
  (#) SPI_I2S_FLAG_OVR : to indicate if an Overrun error occur
  (#) I2S_FLAG_TIFRFE: to indicate a Frame Format error occurs.
  (#) I2S_FLAG_UDR: to indicate an Underrun error occurs.
  (#) I2S_FLAG_CHSIDE: to indicate Channel Side.

  (@) Do not use the BSY flag to handle each data transmission or reception. It is
      better to use the TXE and RXNE flags instead.

 [..] In this Mode it is advised to use the following functions:
   (+) FlagStatus SPI_I2S_GetFlagStatus(SPI_TypeDef* SPIx, uint16_t SPI_I2S_FLAG);
   (+) void SPI_I2S_ClearFlag(SPI_TypeDef* SPIx, uint16_t SPI_I2S_FLAG);

 *** Interrupt Mode ***
 ======================
 [..] In Interrupt Mode, the SPI communication can be managed by 3 interrupt sources
      and 7 pending bits: 
   (+) Pending Bits:
       (##) SPI_I2S_IT_TXE : to indicate the status of the transmit buffer register
       (##) SPI_I2S_IT_RXNE : to indicate the status of the receive buffer register
       (##) SPI_IT_CRCERR : to indicate if a CRC Calculation error occur (available in SPI mode only)            
       (##) SPI_IT_MODF : to indicate if a Mode Fault error occur (available in SPI mode only)
       (##) SPI_I2S_IT_OVR : to indicate if an Overrun error occur
       (##) I2S_IT_UDR : to indicate an Underrun Error occurs (available in I2S mode only).
       (##) I2S_FLAG_TIFRFE : to indicate a Frame Format error occurs (available in TI mode only).

   (+) Interrupt Source:
       (##) SPI_I2S_IT_TXE: specifies the interrupt source for the Tx buffer empty 
            interrupt.  
       (##) SPI_I2S_IT_RXNE : specifies the interrupt source for the Rx buffer not 
            empty interrupt.
       (##) SPI_I2S_IT_ERR : specifies the interrupt source for the errors interrupt.

 [..] In this Mode it is advised to use the following functions:
   (+) void SPI_I2S_ITConfig(SPI_TypeDef* SPIx, uint8_t SPI_I2S_IT, FunctionalState NewState);
   (+) ITStatus SPI_I2S_GetITStatus(SPI_TypeDef* SPIx, uint8_t SPI_I2S_IT);
   (+) void SPI_I2S_ClearITPendingBit(SPI_TypeDef* SPIx, uint8_t SPI_I2S_IT);

 *** DMA Mode ***
 ================
 [..] In DMA Mode, the SPI communication can be managed by 2 DMA Channel requests:
   (#) SPI_I2S_DMAReq_Tx: specifies the Tx buffer DMA transfer request
   (#) SPI_I2S_DMAReq_Rx: specifies the Rx buffer DMA transfer request

 [..] In this Mode it is advised to use the following function:
   (+) void SPI_I2S_DMACmd(SPI_TypeDef* SPIx, uint16_t SPI_I2S_DMAReq, FunctionalState 
       NewState);

@endverbatim
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

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
