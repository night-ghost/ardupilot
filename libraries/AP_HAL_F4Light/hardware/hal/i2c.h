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


#define I2C_BIT_MASK               ((uint32_t)0x00FFFFFF) 

// SR1 bits
#define I2C_BIT_SMBALERT               ((uint32_t)1<<15)
#define I2C_BIT_TIMEOUT                ((uint32_t)1<<14)
// reserved
#define I2C_BIT_PECERR                 ((uint32_t)1<<12)
#define I2C_BIT_OVR                    ((uint32_t)1<<11)
#define I2C_BIT_AF                     ((uint32_t)1<<10)
#define I2C_BIT_ARLO                   ((uint32_t)1<<9)
#define I2C_BIT_BERR                   ((uint32_t)1<<8)
#define I2C_BIT_TXE                    ((uint32_t)1<<7)
#define I2C_BIT_RXNE                   ((uint32_t)1<<6)
//reserved
#define I2C_BIT_STOPF                  ((uint32_t)1<<4)
#define I2C_BIT_ADD10                  ((uint32_t)1<<3)
#define I2C_BIT_BTF                    ((uint32_t)1<<2)
#define I2C_BIT_ADDR                   ((uint32_t)1<<1)
#define I2C_BIT_SB                     ((uint32_t)1<<0)

//SR2 bits

#define I2C_IE_BUF                     ((uint16_t)1<<10) // interrupt enable bits
#define I2C_IE_EVT                     ((uint16_t)1<<9)
#define I2C_IE_ERR                     ((uint16_t)1<<8)
#define I2C_BIT_DUALF                  ((uint32_t)1<<7)
#define I2C_BIT_SMBHOST                ((uint32_t)1<<6)
#define I2C_BIT_SMBDEFAULT             ((uint32_t)1<<5)
#define I2C_BIT_GENCALL                ((uint32_t)1<<4)
//  reserved
#define I2C_BIT_TRA                    ((uint32_t)1<<2)
#define I2C_BIT_BUSY                   ((uint32_t)1<<1)
#define I2C_BIT_MSL                    ((uint32_t)1<<0)


#define I2C_NACK_POS_Next              ((uint16_t)1<<11)

#define I2C_Direction_Transmitter      ((uint8_t)0)
#define I2C_Direction_Receiver         ((uint8_t)1)

/* Maximum Timeout values for events waiting loops */
   
#undef  I2C_TIMEOUT
#define I2C_TIMEOUT         (300)// in uS - wait for byte transfer: 10us per bit (100kHz) * 9 bits
#define I2C_SMALL_TIMEOUT   (50)  // in uS - wait for bit


#define I2C_OK          0
#define I2C_NO_DEVICE   1
#define I2C_ERROR       2
#define I2C_BUS_BUSY    2
#define I2C_ERR_WRITE   6
#define I2C_NO_REGISTER 8 // 8 Acknolege Failed - not "no device"! this happens when we try to read non-existent register from chip
#define I2C_BUS_ERR     99
#define I2C_ERR_STOP    98
#define I2C_STOP_BERR   97
#define I2C_STOP_BUSY   96
#define I2C_ERR_TIMEOUT 95
#define I2C_ERR_REGISTER 94
#define I2C_ERR_OVERRUN  93
#define I2C_ERR_STOP_TIMEOUT 92
#define I2C_DMA_BUSY    103
#define I2C_PENDING     255
#define I2C_DMA_ERROR   100

#define I2C_CR1_CLEAR_MASK    ((uint16_t)0xFBF5)

#define DMA_BUFSIZE 8 // we read just 6 bytes from compass
    
typedef struct I2C_DMA {
    uint32_t channel;
    dma_stream stream_rx;
    dma_stream stream_tx;
} I2C_dma;


typedef struct I2c_state {
    Handler        handler;
    volatile bool  busy;
} i2c_state;


extern uint32_t i2c_bit_time;

/**
 * @brief I2C device type.
 */
typedef struct i2c_dev {
    I2C_TypeDef* regs;          
    uint8_t sda_pin;             
    uint8_t scl_pin;             
    uint32_t clk;          
    uint8_t gpio_af;     
    IRQn_Type ev_nvic_line;  /* Event IRQ number */
    IRQn_Type er_nvic_line;  /* Error IRQ number */        
//    I2C_dma dma;
    i2c_state *state;
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

static inline void i2c_set_isr_handler(const i2c_dev *dev, Handler h){
    IRQn_Type irq;
    dev->state->handler = h;

    irq=dev->er_nvic_line;
    enable_nvic_irq(irq, I2C_INT_PRIORITY); // 8 bits * 4uS = 32uS max reaction time

    irq=dev->ev_nvic_line;
    
    enable_nvic_irq(irq, I2C_INT_PRIORITY);
}

static inline void i2c_clear_isr_handler(const i2c_dev *dev){
    dev->state->handler=0;
}


static inline void i2c_send_address(const i2c_dev *dev, uint8_t address, uint8_t direction)
{
  /* Test on the direction to set/reset the read/write bit */
  if (direction != I2C_Direction_Transmitter) {    
    dev->regs->DR = address | I2C_OAR1_ADD0; /* Set the address bit0 for read */
  } else {    
    dev->regs->DR = address & (uint8_t)~(I2C_OAR1_ADD0); /* Reset the address bit0 for write */
  }
}

static inline void i2c_peripheral_disable(const i2c_dev *dev) {
    dev->regs->CR1 &= (uint16_t)~(I2C_CR1_PE);
}

static inline void i2c_peripheral_enable(const i2c_dev *dev) {
    dev->regs->CR1 |= (uint16_t)I2C_CR1_PE;
}


static inline void i2c_enable_irq(const i2c_dev *dev, uint16_t interrupt_flags) {
    dev->regs->CR2 |= interrupt_flags;
}

static inline void i2c_disable_irq(const i2c_dev *dev, uint16_t interrupt_flags) {
    dev->regs->CR2 &= ~interrupt_flags;
}

#ifdef I2C_DEBUG
uint32_t i2c_get_operation_time(uint8_t *psr1);
#endif

extern const i2c_dev* const _I2C1;
extern const i2c_dev* const _I2C2;
extern const i2c_dev* const _I2C3;

#ifdef __cplusplus
  }
#endif
 

#endif
