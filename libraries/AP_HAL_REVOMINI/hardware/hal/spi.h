#ifndef _SPI_H_
#define _SPI_H_

#include <gpio_hal.h>
#include <stm32f4xx.h>


/*
 * Devices
 */

/** SPI device type */
typedef struct spi_dev {
    SPI_TypeDef* SPIx;          
    uint8_t afio;
    IRQn_Type irq;
    uint16_t clock;
} spi_dev;

extern const spi_dev * const _SPI1;
extern const spi_dev * const _SPI2;
extern const spi_dev * const _SPI3;

typedef enum spi_firstbit {
	LSBFIRST=0,
	MSBFIRST
} spi_firstbit;

/**
 * @brief SPI mode configuration.
 *
 * Determines a combination of clock polarity (CPOL), which determines
 * idle state of the clock line, and clock phase (CPHA), which
 * determines which clock edge triggers data capture.
 */
typedef enum spi_mode {
    SPI_MODE_0=0,  /**< Clock line idles low (0), data capture on first
                    clock transition. */
    SPI_MODE_1,  /**< Clock line idles low (0), data capture on second
                    clock transition */
    SPI_MODE_2,  /**< Clock line idles high (1), data capture on first
                    clock transition. */
    SPI_MODE_3   /**< Clock line idles high (1), data capture on
                    second clock transition. */
} spi_mode;

/**
 * @brief SPI baud rate configuration, as a divisor of f_PCLK, the
 *        PCLK clock frequency.
 */
typedef enum spi_baud_rate {
    SPI_BAUD_PCLK_DIV_2   = SPI_BaudRatePrescaler_2,   /**< f_PCLK/2 */
    SPI_BAUD_PCLK_DIV_4   = SPI_BaudRatePrescaler_4,   /**< f_PCLK/4 */
    SPI_BAUD_PCLK_DIV_8   = SPI_BaudRatePrescaler_8,   /**< f_PCLK/8 */
    SPI_BAUD_PCLK_DIV_16  = SPI_BaudRatePrescaler_16,  /**< f_PCLK/16 */
    SPI_BAUD_PCLK_DIV_32  = SPI_BaudRatePrescaler_32,  /**< f_PCLK/32 */
    SPI_BAUD_PCLK_DIV_64  = SPI_BaudRatePrescaler_64,  /**< f_PCLK/64 */
    SPI_BAUD_PCLK_DIV_128 = SPI_BaudRatePrescaler_128, /**< f_PCLK/128 */
    SPI_BAUD_PCLK_DIV_256 = SPI_BaudRatePrescaler_256, /**< f_PCLK/256 */
} spi_baud_rate;

/** Available SPI interrupts */
typedef enum spi_interrupt {
    SPI_TXE_INTERRUPT  = SPI_I2S_IT_TXE,  /**< TX buffer empty interrupt */
    SPI_RXNE_INTERRUPT = SPI_I2S_IT_RXNE, /**< RX buffer not empty interrupt */
    SPI_ERR_INTERRUPT  = SPI_I2S_IT_ERR   /**<
                                          * Error interrupt (CRC, overrun,
                                          * and mode fault errors for SPI;
                                          * underrun, overrun errors for I2S)
                                          */
} spi_interrupt;

/**
 * @brief Mask for all spi_interrupt values
 * @see spi_interrupt
 */
#define SPI_INTERRUPTS_ALL              (SPI_TXE_INTERRUPT  |           \
                                         SPI_RXNE_INTERRUPT |           \
                                         SPI_ERR_INTERRUPT)
                                         

#ifdef __cplusplus
  extern "C" {
#endif

/**
 * @brief Enable a SPI peripheral
 * @param dev Device to enable
 */
 

static inline void spi_peripheral_enable(const spi_dev *dev) {
        SPI_Cmd(dev->SPIx, ENABLE);
}

/**
 * @brief Disable a SPI peripheral
 * @param dev Device to disable
 */

static inline void spi_peripheral_disable(const spi_dev *dev) {
        SPI_Cmd(dev->SPIx, DISABLE);
}

void spi_reconfigure(const spi_dev *dev, uint8_t ismaster, uint16_t baudPrescaler, uint16_t bitorder, uint8_t mode);

/**
 * @brief Initialize and reset a SPI device.
 * @param dev Device to initialize and reset.
 */
void spi_init(const spi_dev *dev);

/**
 * @brief Configure GPIO bit modes for use as a SPI port's pins.
 * @param as_master If true, configure bits for use as a bus master.
 *                  Otherwise, configure bits for use as slave.
 * @param nss_dev NSS pin's GPIO device
 * @param comm_dev SCK, MISO, MOSI pins' GPIO device
 * @param nss_bit NSS pin's GPIO bit on nss_dev
 * @param sck_bit SCK pin's GPIO bit on comm_dev
 * @param miso_bit MISO pin's GPIO bit on comm_dev
 * @param mosi_bit MOSI pin's GPIO bit on comm_dev
 */

static inline void spi_master_enable(const spi_dev *dev,
                       spi_baud_rate baudPrescaler,
                       spi_mode mode,
                       uint16_t bitorder)
{
    spi_reconfigure(dev, 1, baudPrescaler, bitorder, mode);
}

void spi_gpio_slave_cfg(const spi_dev *dev,
                  const gpio_dev *comm_dev, uint8_t sck_bit,
                  uint8_t miso_bit,
                  uint8_t mosi_bit);
                  
/**
 * @brief Configure and enable a SPI device as bus master.
 *
 * The device's peripheral will be disabled before being reconfigured.
 *
 * @param dev Device to configure as bus master
 * @param baud Bus baud rate
 * @param mode SPI mode
 * @param flags Logical OR of spi_cfg_flag values.
 * @see spi_cfg_flag
 */
 
void spi_gpio_master_cfg(const spi_dev *dev,
                  const gpio_dev *comm_dev,
                  uint8_t sck_bit,
                  uint8_t miso_bit,
                  uint8_t mosi_bit);
 


                       
/**
 * @brief Configure and enable a SPI device as a bus slave.
 *
 * The device's peripheral will be disabled before being reconfigured.
 *
 * @param dev Device to configure as a bus slave
 * @param mode SPI mode
 * @param flags Logical OR of spi_cfg_flag values.
 * @see spi_cfg_flag
 */
void spi_slave_enable(const spi_dev *dev,
                      spi_mode mode,
                      uint16_t bitorder);


void spi_set_speed(const spi_dev *dev, uint16_t baudPrescaler);                   

/*
uint32_t spi_tx(spi_dev *dev, const void *buf, uint32_t len);
void spi_tx_byte(spi_dev *dev, uint8_t data);
uint8_t spi_rx_byte(spi_dev *dev);
void spi_write(spi_dev *dev, uint8_t reg, uint8_t data);
void spi_tx_buf(spi_dev *dev, uint8_t *txbuf, uint32_t txcount);
*/

void spi_foreach(void (*fn)(const spi_dev*));

uint32_t spi_tx(const spi_dev *dev, const void *buf, uint32_t len);

int spimaster_transfer(const spi_dev *dev,
                       const uint8_t *txbuf,
                       uint32_t txcount,
                       uint8_t *rxbuf,
                       uint32_t rxcount);
                       
                       

static inline uint8_t spi_is_enabled(const spi_dev *dev) {
    return dev->SPIx->CR1 & SPI_CR1_SPE;
}

static inline void spi_peripheral_disable_all(void) {
    spi_foreach(spi_peripheral_disable);
}

static inline void spi_irq_enable(const spi_dev *dev, uint32_t interrupt_flags) {
	SPI_I2S_ITConfig(dev->SPIx, interrupt_flags, ENABLE);
}

static inline void spi_irq_disable(const spi_dev *dev, uint32_t interrupt_flags) {
	SPI_I2S_ITConfig(dev->SPIx, interrupt_flags, DISABLE);
}

static inline uint16_t spi_dff(const spi_dev *dev) {
    return ((dev->SPIx->CR1 & SPI_DataSize_16b) == SPI_DataSize_8b ? SPI_DataSize_8b : SPI_DataSize_16b);
}

static inline uint8_t spi_is_rx_nonempty(const spi_dev *dev) {
	return (dev->SPIx->SR & SPI_I2S_FLAG_RXNE);
}

static inline uint16_t spi_rx_reg(const spi_dev *dev) {
    return (uint16_t)dev->SPIx->DR;
}

static inline uint8_t spi_is_tx_empty(const spi_dev *dev) {
	return (dev->SPIx->SR & SPI_I2S_FLAG_TXE);
}

static inline void spi_tx_reg(const spi_dev *dev, uint16_t val) {
    dev->SPIx->DR = val;
}

static inline uint8_t spi_is_busy(const spi_dev *dev) {
	return (dev->SPIx->SR & SPI_I2S_FLAG_BSY);
}

                                       
#ifdef __cplusplus
  }
#endif

#endif
