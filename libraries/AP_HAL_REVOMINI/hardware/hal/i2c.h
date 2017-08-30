#ifndef _I2C_H
#define _I2C_H

#include <hal_types.h>
#include <hal.h>
#include "dma.h"

#define I2C_50KHz_SPEED                          50000
#define I2C_75KHz_SPEED                          75000
#define I2C_100KHz_SPEED                        100000
#define I2C_250KHz_SPEED                        250000
#define I2C_400KHz_SPEED                        400000

// missed definition in .h
#define FLAG_MASK         ((uint32_t)0x00FFFFFF)  /*<! I2C FLAG mask */

/* Maximum Timeout values for events waiting loops */
   
#undef  I2C_TIMEOUT
#define I2C_TIMEOUT         (4000)// in uS

#define I2C_OK          0
#define I2C_NO_DEVICE   1
#define I2C_ERROR       2
#define I2C_BUS_BUSY    2
#define I2C_BUS_BERR    99
#define I2C_ERR_STOP    98
#define I2C_STOP_BERR   97
#define I2C_STOP_BUSY   96
#define I2C_ERR_TIMEOUT 95


typedef struct I2C_DMA {
    uint32_t channel;
    dma_stream stream_rx;
    dma_stream stream_tx;
} I2C_dma;

/*
typedef struct i2c_state {
    bool active;
} i2c_state;
*/


/**
 * @brief I2C device type.
 */
typedef struct i2c_dev {
    I2C_TypeDef* I2Cx;          
    const gpio_dev *gpio_port;        
    uint8_t sda_pin;             
    uint8_t scl_pin;             
    uint32_t clk;          
    uint8_t gpio_af;     
    IRQn_Type ev_nvic_line;  /* Event IRQ number */
    IRQn_Type er_nvic_line;  /* Error IRQ number */        
    I2C_dma dma;
    voidFuncPtr dma_isr;
//    i2c_state *state;
} i2c_dev;

#ifdef __cplusplus
  extern "C" {
#endif
 

void i2c_init(const i2c_dev *dev, uint16_t address, uint32_t speed);
void i2c_deinit(const i2c_dev *dev);

uint32_t i2c_write(const i2c_dev *dev, uint8_t addr, const uint8_t *tx_buff, uint8_t txlen);
uint32_t i2c_read (const i2c_dev *dev, uint8_t addr, const uint8_t *tx_buff, uint8_t txlen, uint8_t *rx_buff, uint8_t rxlen);

void i2c_lowLevel_deinit(const i2c_dev *dev);

void i2c_master_release_bus(const i2c_dev *dev);
bool i2c_bus_reset(const i2c_dev *dev);

#ifdef I2C_DEBUG
uint32_t i2c_get_operation_time(uint8_t *psr1);
#endif

extern const i2c_dev* const _I2C1;
extern const i2c_dev* const _I2C2;

#ifdef __cplusplus
  }
#endif
 

#endif
