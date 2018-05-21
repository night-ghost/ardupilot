/*

(c) 2017 night_ghost@ykoctpa.ru
 

based on: LeafLabs

*/

#pragma GCC optimize ("O2")

#include <hal.h>
#include "adc.h"
#include <stdbool.h>

const adc_dev _adc1 = {
    .adcx   = ADC1,
};
/** ADC1 device. */
const adc_dev* const _ADC1 = &_adc1;

const adc_dev _adc2 = {
    .adcx   = ADC2,
};
/** ADC2 device. */
const adc_dev* const _ADC2 = &_adc2;

const adc_dev const _adc3 = {
    .adcx   = ADC3,
};
/** ADC3 device. */
const adc_dev* const _ADC3 = &_adc3;

__IO uint16_t 	ADC_ConvertedValue;
__IO bool adc_data_ready;

/**
 * @brief Call a function on all ADC devices.
 * @param fn Function to call on each ADC device.
 */
void adc_foreach(void (*fn)(const adc_dev*)) 
{
    fn(_ADC1);
    fn(_ADC2);
    fn(_ADC3);
}

#define CR_CLEAR_MASK             ((uint32_t)0xFFFC30E0)  
#define CR1_CLEAR_MASK            ((uint32_t)0xFCFFFEFF)
#define CR2_CLEAR_MASK            ((uint32_t)0xC0FFF7FD)


/**
 * @brief Initialize an ADC peripheral.
 *
 * Initializes the RCC clock line for the given peripheral.  Resets
 * ADC device registers.
 *
 * @param dev ADC peripheral to initialize
 */
void adc_init(const adc_dev *dev) {
 
    if (dev->adcx == ADC1)
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    else if (dev->adcx == ADC2)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
    else
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE); 

    
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC, ENABLE); // Enable all ADCs reset state 
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC, DISABLE);// Release all ADCs from reset state
      
    /* Get the ADC CCR value */
    uint32_t tmp = ADC->CCR  & CR_CLEAR_MASK; // Clear MULTI, DELAY, DMA and ADCPRE bits

    /* Configure ADCx: Multi mode, Delay between two sampling time, ADC prescaler,
       and DMA access mode for multimode */
    /* Set MULTI bits according to ADC_Mode value */
    /* Set ADCPRE bits according to ADC_Prescaler value */
    /* Set DMA bits according to ADC_DMAAccessMode value */
    /* Set DELAY bits according to ADC_TwoSamplingDelay value */
    ADC->CCR = tmp | (uint32_t)(ADC_Mode_Independent |
                                    ADC_Prescaler_Div4 |
                                    ADC_DMAAccessMode_Disabled |
                                    ADC_TwoSamplingDelay_5Cycles);

    /* ADCx Init ****************************************************************/
    
   /* Get the ADCx CR1 value */
    tmp = dev->adcx->CR1 & CR1_CLEAR_MASK;  // Clear RES and SCAN bits 

  /* Configure ADCx: scan conversion mode and resolution */
  /* Set SCAN bit according to ADC_ScanConvMode value */
  /* Set RES bit according to ADC_Resolution value */
  dev->adcx->CR1 = tmp | (uint32_t)(((uint32_t)DISABLE << 8) | ADC_Resolution_12b);

  /*---------------------------- ADCx CR2 Configuration -----------------*/
  /* Get the ADCx CR2 value */
  tmp = dev->adcx->CR2 & CR2_CLEAR_MASK; // Clear CONT, ALIGN, EXTEN and EXTSEL bits 


  /* Configure ADCx: external trigger event and edge, data alignment and 
     continuous conversion mode */
  /* Set ALIGN bit according to ADC_DataAlign value */
  /* Set EXTEN bits according to ADC_ExternalTrigConvEdge value */
  /* Set EXTSEL bits according to ADC_ExternalTrigConv value */
  /* Set CONT bit according to ADC_ContinuousConvMode value */
  dev->adcx->CR2 = tmp | (uint32_t)(ADC_DataAlign_Right | \
                        ADC_ExternalTrigConv_T1_CC1 |
                        ADC_ExternalTrigConvEdge_None | \
                        ((uint32_t)DISABLE << 1));

  /*---------------------------- ADCx SQR1 Configuration -----------------*/
  /* Get the ADCx SQR1 value */
  tmp = dev->adcx->SQR1;

  /* Clear L bits */
  tmp &= SQR1_L_RESET;

  /* Configure ADCx: regular channel sequence length */
  /* Set L bits according to ADC_NbrOfConversion value */
  uint32_t nc = (uint8_t)(1 /* number of conversions */ - (uint8_t)1);
  dev->adcx->SQR1 =tmp | ((uint32_t)nc << 20);
}

#if 0 // unused

/**
 * @brief Perform a single synchronous software triggered conversion on a
 * channel.
 * @param dev ADC device to use for reading.
 * @param channel channel to convert
 * @return conversion result
 */
uint16_t adc_read(const adc_dev *dev, uint8_t channel)
{
  adc_data_ready = false;
  
  adc_disable(dev);
 
  /* ADC regular channel14 configuration */
  adc_channel_config(dev, channel, 1, ADC_SampleTime_56Cycles);
  adc_enable(dev);
      
  /* Start ADC Software Conversion */
  ADC_SoftwareStartConv(dev->adcx);
 
  /* Wait until ADC Channel end of conversion */  
  while (ADC_GetFlagStatus(dev->adcx, ADC_FLAG_EOC) == RESET);
  
  /* Read ADC conversion result */
  return ADC_GetConversionValue(dev->adcx);
}

uint16_t temp_read(void)
{
  uint8_t i;
  uint16_t res, T_StartupTimeDelay;
 
  ADC_TempSensorVrefintCmd(ENABLE);
  /* Wait until ADC + Temp sensor start */
  T_StartupTimeDelay = 1024;
  while (T_StartupTimeDelay--);
  
  /* Enable TempSensor and Vrefint channels: channel16 and Channel17 */
  adc_channel_config(_adc1, ADC_Channel_16, 1, ADC_SampleTime_84Cycles);
                               
  /* initialize result */
  res = 0;
  for(i=4; i>0; i--)  {
  /* start ADC convertion by software */
    ADC_SoftwareStartConv(ADC1);

    /* wait until end-of-covertion */
    while( ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == 0 );
  /* read ADC convertion result */
    res += ADC_GetConversionValue(ADC1);
  }
	
  /* de-initialize ADC */
  ADC_TempSensorVrefintCmd(DISABLE);
  
  return (res>>2);
}

uint16_t vref_read(void)
{
  uint8_t i;
  uint16_t res, T_StartupTimeDelay;

  adc_set_reg_seqlen(_ADC1, 1);
  ADC_TempSensorVrefintCmd(ENABLE);

  /* Wait until ADC + Temp sensor start */
  T_StartupTimeDelay = 1024;
  while (T_StartupTimeDelay--);
  
  /* Enable TempSensor and Vrefint channels: channel16 and Channel17 */
//  adc_channel_config(_adc1, ADC_Channel_17, 2, ADC_SampleTime_56Cycles);
  adc_channel_config(_adc1, ADC_Channel_17, 1, ADC_SampleTime_84Cycles);

  /* initialize result */
  res = 0;
  for(i=4; i>0; i--)  {
  /* start ADC convertion by software */
    ADC_SoftwareStartConv(ADC1);

    /* wait until end-of-covertion */
    while( ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == 0 );
  /* read ADC convertion result */
    res += ADC_GetConversionValue(ADC1);
  }

  /* de-initialize ADC */
  ADC_TempSensorVrefintCmd(DISABLE);

  return (res>>2);
}
#endif
