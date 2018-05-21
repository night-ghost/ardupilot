/**
 *  @file adc.h
 *
 *  @brief Analog-to-Digital Conversion (ADC) header.
 */

#ifndef _ADC_H_
#define _ADC_H_

#include "stm32.h"
#include "hal_types.h"

/** ADC device type. */
typedef struct adc_dev {
   ADC_TypeDef *adcx;
} adc_dev;

#ifdef __cplusplus
extern "C"{
#endif

extern const adc_dev* const _ADC1;
extern const adc_dev* const _ADC2;
extern const adc_dev* const _ADC3;

extern const adc_dev _adc1;
extern const adc_dev _adc2;
extern const adc_dev _adc3;

#define ADC_Channel_0                               ((uint8_t)0x00)
#define ADC_Channel_1                               ((uint8_t)0x01)
#define ADC_Channel_2                               ((uint8_t)0x02)
#define ADC_Channel_3                               ((uint8_t)0x03)
#define ADC_Channel_4                               ((uint8_t)0x04)
#define ADC_Channel_5                               ((uint8_t)0x05)
#define ADC_Channel_6                               ((uint8_t)0x06)
#define ADC_Channel_7                               ((uint8_t)0x07)
#define ADC_Channel_8                               ((uint8_t)0x08)
#define ADC_Channel_9                               ((uint8_t)0x09)
#define ADC_Channel_10                              ((uint8_t)0x0A)
#define ADC_Channel_11                              ((uint8_t)0x0B)
#define ADC_Channel_12                              ((uint8_t)0x0C)
#define ADC_Channel_13                              ((uint8_t)0x0D)
#define ADC_Channel_14                              ((uint8_t)0x0E)
#define ADC_Channel_15                              ((uint8_t)0x0F)
#define ADC_Channel_16                              ((uint8_t)0x10)
#define ADC_Channel_17                              ((uint8_t)0x11)
#define ADC_Channel_18                              ((uint8_t)0x12)

#define ADC_Channel_TempSensor                      ((uint8_t)ADC_Channel_16)
#define ADC_Channel_Vrefint                         ((uint8_t)ADC_Channel_17)
#define ADC_Channel_Vbat                            ((uint8_t)ADC_Channel_18)

#define ADC_SampleTime_3Cycles                    ((uint8_t)0x00)
#define ADC_SampleTime_15Cycles                   ((uint8_t)0x01)
#define ADC_SampleTime_28Cycles                   ((uint8_t)0x02)
#define ADC_SampleTime_56Cycles                   ((uint8_t)0x03)
#define ADC_SampleTime_84Cycles                   ((uint8_t)0x04)
#define ADC_SampleTime_112Cycles                  ((uint8_t)0x05)
#define ADC_SampleTime_144Cycles                  ((uint8_t)0x06)
#define ADC_SampleTime_480Cycles                  ((uint8_t)0x07)

#define SMPR_SMP_SET             ((uint32_t)0x00000007)  
#define SQR_SQ_SET               ((uint32_t)0x0000001F)
#define SQR1_L_RESET             ((uint32_t)0xFE0FFFFF)


#define ADC_Mode_Independent                       ((uint32_t)0x00000000)       
#define ADC_DualMode_RegSimult_InjecSimult         ((uint32_t)0x00000001)
#define ADC_DualMode_RegSimult_AlterTrig           ((uint32_t)0x00000002)
#define ADC_DualMode_InjecSimult                   ((uint32_t)0x00000005)
#define ADC_DualMode_RegSimult                     ((uint32_t)0x00000006)
#define ADC_DualMode_Interl                        ((uint32_t)0x00000007)
#define ADC_DualMode_AlterTrig                     ((uint32_t)0x00000009)
#define ADC_TripleMode_RegSimult_InjecSimult       ((uint32_t)0x00000011)
#define ADC_TripleMode_RegSimult_AlterTrig         ((uint32_t)0x00000012)
#define ADC_TripleMode_InjecSimult                 ((uint32_t)0x00000015)
#define ADC_TripleMode_RegSimult                   ((uint32_t)0x00000016)
#define ADC_TripleMode_Interl                      ((uint32_t)0x00000017)
#define ADC_TripleMode_AlterTrig                   ((uint32_t)0x00000019)

#define ADC_Prescaler_Div2                         ((uint32_t)0x00000000)
#define ADC_Prescaler_Div4                         ((uint32_t)0x00010000)
#define ADC_Prescaler_Div6                         ((uint32_t)0x00020000)
#define ADC_Prescaler_Div8                         ((uint32_t)0x00030000)

#define ADC_DMAAccessMode_Disabled      ((uint32_t)0x00000000)     /* DMA mode disabled */
#define ADC_DMAAccessMode_1             ((uint32_t)0x00004000)     /* DMA mode 1 enabled (2 / 3 half-words one by one - 1 then 2 then 3)*/
#define ADC_DMAAccessMode_2             ((uint32_t)0x00008000)     /* DMA mode 2 enabled (2 / 3 half-words by pairs - 2&1 then 1&3 then 3&2)*/
#define ADC_DMAAccessMode_3             ((uint32_t)0x0000C000)     /* DMA mode 3 enabled (2 / 3 bytes by pairs - 2&1 then 1&3 then 3&2) */

#define ADC_TwoSamplingDelay_5Cycles               ((uint32_t)0x00000000)
#define ADC_TwoSamplingDelay_6Cycles               ((uint32_t)0x00000100)
#define ADC_TwoSamplingDelay_7Cycles               ((uint32_t)0x00000200)
#define ADC_TwoSamplingDelay_8Cycles               ((uint32_t)0x00000300)
#define ADC_TwoSamplingDelay_9Cycles               ((uint32_t)0x00000400)
#define ADC_TwoSamplingDelay_10Cycles              ((uint32_t)0x00000500)
#define ADC_TwoSamplingDelay_11Cycles              ((uint32_t)0x00000600)
#define ADC_TwoSamplingDelay_12Cycles              ((uint32_t)0x00000700)
#define ADC_TwoSamplingDelay_13Cycles              ((uint32_t)0x00000800)
#define ADC_TwoSamplingDelay_14Cycles              ((uint32_t)0x00000900)
#define ADC_TwoSamplingDelay_15Cycles              ((uint32_t)0x00000A00)
#define ADC_TwoSamplingDelay_16Cycles              ((uint32_t)0x00000B00)
#define ADC_TwoSamplingDelay_17Cycles              ((uint32_t)0x00000C00)
#define ADC_TwoSamplingDelay_18Cycles              ((uint32_t)0x00000D00)
#define ADC_TwoSamplingDelay_19Cycles              ((uint32_t)0x00000E00)
#define ADC_TwoSamplingDelay_20Cycles              ((uint32_t)0x00000F00)

#define ADC_Resolution_12b                         ((uint32_t)0x00000000)
#define ADC_Resolution_10b                         ((uint32_t)0x01000000)
#define ADC_Resolution_8b                          ((uint32_t)0x02000000)
#define ADC_Resolution_6b                          ((uint32_t)0x03000000)

#define ADC_ExternalTrigConvEdge_None          ((uint32_t)0x00000000)
#define ADC_ExternalTrigConvEdge_Rising        ((uint32_t)0x10000000)
#define ADC_ExternalTrigConvEdge_Falling       ((uint32_t)0x20000000)
#define ADC_ExternalTrigConvEdge_RisingFalling ((uint32_t)0x30000000)


#define ADC_ExternalTrigConv_T1_CC1                ((uint32_t)0x00000000)
#define ADC_ExternalTrigConv_T1_CC2                ((uint32_t)0x01000000)
#define ADC_ExternalTrigConv_T1_CC3                ((uint32_t)0x02000000)
#define ADC_ExternalTrigConv_T2_CC2                ((uint32_t)0x03000000)
#define ADC_ExternalTrigConv_T2_CC3                ((uint32_t)0x04000000)
#define ADC_ExternalTrigConv_T2_CC4                ((uint32_t)0x05000000)
#define ADC_ExternalTrigConv_T2_TRGO               ((uint32_t)0x06000000)
#define ADC_ExternalTrigConv_T3_CC1                ((uint32_t)0x07000000)
#define ADC_ExternalTrigConv_T3_TRGO               ((uint32_t)0x08000000)
#define ADC_ExternalTrigConv_T4_CC4                ((uint32_t)0x09000000)
#define ADC_ExternalTrigConv_T5_CC1                ((uint32_t)0x0A000000)
#define ADC_ExternalTrigConv_T5_CC2                ((uint32_t)0x0B000000)
#define ADC_ExternalTrigConv_T5_CC3                ((uint32_t)0x0C000000)
#define ADC_ExternalTrigConv_T8_CC1                ((uint32_t)0x0D000000)
#define ADC_ExternalTrigConv_T8_TRGO               ((uint32_t)0x0E000000)
#define ADC_ExternalTrigConv_Ext_IT11              ((uint32_t)0x0F000000)


#define ADC_DataAlign_Right                        ((uint32_t)0x00000000)
#define ADC_DataAlign_Left                         ((uint32_t)0x00000800)



void adc_init(const adc_dev *dev);
//void adc_set_extsel(const adc_dev *dev, adc_extsel_event event);
void adc_foreach(void (*fn)(const adc_dev*));
//void adc_set_sample_rate(const adc_dev *dev, adc_smp_rate smp_rate);

uint16_t adc_read(const adc_dev *dev, uint8_t channel);
uint16_t vref_read(void);
uint16_t temp_read(void);

/**
 * @brief Set the regular channel sequence length.
 *
 * Defines the total number of conversions in the regular channel
 * conversion sequence.
 *
 * @param dev ADC device.
 * @param length Regular channel sequence length, from 1 to 16.
 */
static inline void adc_set_reg_seqlen(const adc_dev *dev, uint8_t length) {
	/* ADC L Mask */

	uint32_t tmpreg1 = 0;
	uint8_t tmpreg2 = 0;

	/* Get the ADCx SQR1 value */
	tmpreg1 = dev->adcx->SQR1;
	/* Clear L bits */
	tmpreg1 &= SQR1_L_RESET;
	/* Configure ADCx: regular channel sequence length */
	/* Set L bits according to ADC_NbrOfConversion value */ 
	tmpreg2 |= (uint8_t)(length - (uint8_t)1);
	tmpreg1 |= ((uint32_t)tmpreg2 << 20);
	/* Write to ADCx SQR1 */
	dev->adcx->SQR1 = tmpreg1;
}

static inline void adc_channel_config(const adc_dev *dev, uint8_t channel, uint8_t rank, uint8_t sampleTime)
{
  /* if ADC_Channel_10 ... ADC_Channel_18 is selected */
  if (channel > ADC_Channel_9)  {
    uint32_t tmpreg1 = dev->adcx->SMPR1 & ~(SMPR_SMP_SET << (3 * (channel - 10)));
    dev->adcx->SMPR1 = tmpreg1 |    (uint32_t)sampleTime << (3 * (channel - 10));
  }  else  {/* channel include in ADC_Channel_[0..9] */
    uint32_t tmpreg1 = dev->adcx->SMPR2 & ~(SMPR_SMP_SET << (3 * channel));
    dev->adcx->SMPR2 = tmpreg1 |    (uint32_t)sampleTime << (3 * channel);
  }

  if (rank < 7) {
    uint32_t tmpreg1 = dev->adcx->SQR3 & ~(SQR_SQ_SET << (5 * (rank - 1)));
    dev->adcx->SQR3 = tmpreg1 |     (uint32_t)channel << (5 * (rank - 1));
  } else if (rank < 13)  { /* For Rank 7 to 12 */
    uint32_t tmpreg1 = dev->adcx->SQR2 & ~(SQR_SQ_SET << (5 * (rank - 7)));
    dev->adcx->SQR2 = tmpreg1 |     (uint32_t)channel << (5 * (rank - 7));
  }  else  { /* For Rank 13 to 16 */
    uint32_t tmpreg1 = dev->adcx->SQR1 & ~(SQR_SQ_SET << (5 * (rank - 13)));
    dev->adcx->SQR1 = tmpreg1 |     (uint32_t)channel << (5 * (rank - 13));
  }
}
  
/**
 * @brief Enable an adc peripheral
 * @param dev ADC device to enable
 */
static inline void adc_enable(const adc_dev *dev) {
    /* Enable ADCx */
    /* Set the ADON bit to wake up the ADC from power down mode */
    dev->adcx->CR2 |= (uint32_t)ADC_CR2_ADON;
}

/**
 * @brief Disable an ADC peripheral
 * @param dev ADC device to disable
 */
static inline void adc_disable(const adc_dev *dev) {
    dev->adcx->CR2 &= (uint32_t)(~ADC_CR2_ADON);
}

/**
 * @brief Disable all ADC peripherals.
 */
static inline void adc_disable_all(void) {
    adc_foreach(adc_disable);
}

static inline void adc_start_conv(const adc_dev *dev)
{
  /* Enable the selected ADC conversion for regular group */
  dev->adcx->CR2 |= (uint32_t)ADC_CR2_SWSTART;
}

static inline void adc_vref_enable(){
    /* Enable the temperature sensor and Vrefint channel*/
    ADC->CCR |= (uint32_t)ADC_CCR_TSVREFE;
}

static inline void adc_vref_disable(){
    /* Disable the temperature sensor and Vrefint channel*/
    ADC->CCR &= (uint32_t)(~ADC_CCR_TSVREFE);
}


#ifdef __cplusplus
} // extern "C"
#endif

#endif
