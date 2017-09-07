#include "i2c.h"
#include "dma.h"
#include "systick.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_dma.h"



#define I2C_Yield(x) hal_yield(x)

static void i2c1_isr_handler();
static void i2c2_isr_handler();


static const i2c_dev i2c_dev1 = {
    .I2Cx         = I2C1,
    .gpio_port    = &gpiob,
    .sda_pin      = 9,
    .scl_pin      = 8,
    .clk       	  = RCC_APB1Periph_I2C1,
    .gpio_af	  = GPIO_AF_I2C1,
    .ev_nvic_line = I2C1_EV_IRQn,
    .er_nvic_line = I2C1_ER_IRQn,
    .dma          = { DMA_CR_CH1, DMA1_STREAM0, DMA1_STREAM6 }, // I2C1
    .dma_isr      = i2c1_isr_handler,
};
/** I2C1 device */
const i2c_dev* const _I2C1 = &i2c_dev1;


static const i2c_dev i2c_dev2 = {
    .I2Cx         = I2C2,
    .gpio_port    = &gpiob,
    .sda_pin      = 11,
    .scl_pin      = 10,
    .clk       	  = RCC_APB1Periph_I2C2,
    .gpio_af	  = GPIO_AF_I2C2,
    .ev_nvic_line = I2C2_EV_IRQn,
    .er_nvic_line = I2C2_ER_IRQn,
    .dma          = { DMA_CR_CH7, DMA1_STREAM3 /* intersects with spi2_tx */ , DMA1_STREAM7 }, // I2C2
    .dma_isr      = i2c2_isr_handler,
};

/** I2C2 device */
const i2c_dev* const _I2C2 = &i2c_dev2;


typedef enum {TX = 0, RX = 1, TXREG = 2} I2C_Dir;

static void delay_10us(){
    hal_delay_microseconds(10);
}

static uint16_t i2c_bit_time=4; // on 250kHz 1 bit is 4uS

#ifdef I2C_DEBUG
static uint32_t op_time;
static uint32_t op_sr1;

inline uint32_t i2c_get_operation_time(uint8_t *psr1){ 
    if(psr1) *psr1 = op_sr1;
    return op_time; 
}
#endif

/**
 * @brief  DeInitializes peripherals used by the I2C driver.
 * @param  None
 * @retval None
 */
void i2c_lowLevel_deinit(const i2c_dev *dev){
    GPIO_InitTypeDef GPIO_InitStructure;

    /* I2C Peripheral Disable */
    I2C_Cmd(dev->I2Cx, DISABLE);

    /* I2C DeInit */
    I2C_DeInit(dev->I2Cx);

    /*!< GPIO configuration */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    /*!< Configure I2C pins: SCL */
    GPIO_InitStructure.GPIO_Pin = BIT(dev->scl_pin);
    GPIO_Init(dev->gpio_port->GPIOx, &GPIO_InitStructure);

    /*!< Configure I2C pins: SDA */
    GPIO_InitStructure.GPIO_Pin = BIT(dev->sda_pin);
    GPIO_Init(dev->gpio_port->GPIOx, &GPIO_InitStructure);
}

/**
 * @brief  Initializes peripherals used by the I2C EEPROM driver.
 * @param  None
 * @retval None
 */
static inline void i2c_lowLevel_init(const i2c_dev *dev)  {
    GPIO_InitTypeDef GPIO_InitStructure;
//    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable the i2c */
    RCC_APB1PeriphClockCmd(dev->clk, ENABLE);

    /* Reset the Peripheral */
    RCC_APB1PeriphResetCmd(dev->clk, ENABLE);
    RCC_APB1PeriphResetCmd(dev->clk, DISABLE);

    /* Enable the GPIOs for the SCL/SDA Pins */
    RCC_AHB1PeriphClockCmd(dev->gpio_port->clk, ENABLE);


// common configuration
    /* common GPIO configuration */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz; // GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    /* Configure SCL */
    GPIO_InitStructure.GPIO_Pin = BIT(dev->scl_pin);
    GPIO_Init(dev->gpio_port->GPIOx, &GPIO_InitStructure);

    /* Configure SDA */
    GPIO_InitStructure.GPIO_Pin = BIT(dev->sda_pin);
    GPIO_Init(dev->gpio_port->GPIOx, &GPIO_InitStructure);

    /* Connect GPIO pins to peripheral */
    GPIO_PinAFConfig(dev->gpio_port->GPIOx, dev->scl_pin, dev->gpio_af);
    GPIO_PinAFConfig(dev->gpio_port->GPIOx, dev->sda_pin, dev->gpio_af);
}

void i2c_init(const i2c_dev *dev, uint16_t address, uint32_t speed)
{
    I2C_InitTypeDef I2C_InitStructure;

    i2c_lowLevel_init(dev);


    i2c_bit_time = 1000000l / speed;

    /* I2C configuration */
    I2C_StructInit(&I2C_InitStructure);

    I2C_InitStructure.I2C_Mode                = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle           = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1         = address;
    I2C_InitStructure.I2C_Ack                 = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed          = speed;

    I2C_ITConfig(dev->I2Cx, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);

    /* I2C Peripheral Enable */
    I2C_Cmd(dev->I2Cx, ENABLE);
    /* Apply I2C configuration after enabling it */
    I2C_Init(dev->I2Cx, &I2C_InitStructure);

    dma_init(dev->dma.stream_rx);
    dma_disable(dev->dma.stream_rx);
}

/**
 * @brief  DeInitializes peripherals used by the I2C EEPROM driver.
 * @param  None
 * @retval None
 */
void i2c_deinit(const i2c_dev *dev)
{
    i2c_lowLevel_deinit(dev);
}



#define DMA_BUFSIZE 16 // we read just 6 bytes from compass

static uint8_t dma_buffer[DMA_BUFSIZE];

/* Send a buffer to the i2c port */
uint32_t i2c_write(const i2c_dev *dev, uint8_t addr, const uint8_t *tx_buff, uint8_t len)
{

//    uint16_t sent = 0;
    const uint8_t *buffer = tx_buff;

    uint32_t state = I2C_ERROR;
    uint16_t sr1;
    
    /*!< While the bus is busy */
    uint32_t t = hal_micros();
    while ((dev->I2Cx->SR2 & (I2C_FLAG_BUSY>>16) & FLAG_MASK) != 0) {
	if (hal_micros() - t > I2C_TIMEOUT)
	    return state; // 2 - bus busy
	
	I2C_Yield(0); 
    }

    state++;



    // Bus got!  enable Acknowledge for our operation
    dev->I2Cx->CR1 |= I2C_CR1_ACK; 
    dev->I2Cx->CR1 &= ~I2C_NACKPosition_Next; 

    // Send START condition
    dev->I2Cx->CR1 |= I2C_CR1_START;

    // Test on EV5 and clear it (cleared by reading SR1 then writing to DR)
    t = hal_micros();

    while( ((sr1=dev->I2Cx->SR1) & I2C_FLAG_SB & FLAG_MASK) == 0) { // wait for start bit generated
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6
        
        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        
        
        
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout - time of timeout much large than we use so it is useless, but...
            dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }
	if (hal_micros() - t > I2C_TIMEOUT)
	    return state; // 3 - failed to start
    }


    state++;
    
    // Send address for write
    I2C_Send7bitAddress(dev->I2Cx, addr<<1, I2C_Direction_Transmitter );

    dev->I2Cx->CR1 &= (uint16_t)(~I2C_CR1_STOP);    /* clear STOP condition - just to touch CR1*/

    t = hal_micros();
    // Test on EV6 and clear it
    while ( ((sr1 = dev->I2Cx->SR1) & I2C_FLAG_ADDR & FLAG_MASK) == 0)  {
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(sr1 & I2C_FLAG_AF & FLAG_MASK) {
            dev->I2Cx->SR1 = ~I2C_SR1_AF; // reset it
            
            state = I2C_NO_DEVICE; // Acknolege Failed
            goto err_exit;
        }

        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }

	if (hal_micros() - t > I2C_TIMEOUT)
	    goto err_exit; // 4 failed to send address
    }

    /* Clear ADDR register by reading SR1 then SR2 register (SR1 has already been read) */
    (void) dev->I2Cx->SR2;

    state++;

    while( ((sr1=dev->I2Cx->SR1) & I2C_FLAG_TXE & FLAG_MASK)  == 0) { // wait for TX empty
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }

        if (hal_micros() - t > I2C_TIMEOUT) goto err_exit; // 5 - TXe not set on 1st byte
    }

    state++;

    dev->I2Cx->DR = *buffer++; // 1st byte


    if (len < 2) { // only 1 byte
        /* Test on EV8 and clear it */
	t = hal_micros();
//	while(I2C_GetFlagStatus(dev->I2Cx, I2C_FLAG_BTF ) == RESET) { // wait for end of transmission
        while( ((sr1=dev->I2Cx->SR1) & I2C_FLAG_TXE & FLAG_MASK)  == 0) { // wait for TX empty
            if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

            if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                state = I2C_BUS_ERR;
                goto err_exit;
            }
            if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
                dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
                return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
            }

    	    if (hal_micros() - t > I2C_TIMEOUT) goto err_exit; // 6 1-st byte transmit failed
        }

        state++;

	dev->I2Cx->CR1 |= I2C_CR1_STOP;         	/* Send STOP condition */

    } else {
	do {
	    t = hal_micros();
            while( ((sr1=dev->I2Cx->SR1) & I2C_FLAG_TXE & FLAG_MASK)  == 0) { // wait for TX empty
                if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

                if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                    dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                    state = I2C_BUS_ERR;
                    goto err_exit;
                }
                if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
                    dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
                    return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
                }

                if (hal_micros() - t > I2C_TIMEOUT) {
                            // byte   1 2 3 4 5...
		    goto err_exit; // 6 7 8 9 10 byte transmit failed
		}		    
//                I2C_Yield(i2c_bit_time*8);   // 250kHz so 1 bit is 4uS, 8 bits
            }
            state++;

            if(--len == 0) { // last is sent, no more bytes
            
	        t = hal_micros();
                while( ((sr1=dev->I2Cx->SR1) & I2C_FLAG_TXE & FLAG_MASK)  == 0) { // wait for TX empty
                    if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

                    if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                        dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                        state = I2C_BUS_ERR;
                        goto err_exit;
                    }
                    if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
                        dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
                        return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
                    }

	            if (hal_micros() - t > I2C_TIMEOUT) goto err_exit; // 7 2nd byte transmit failed
                }
                                
		dev->I2Cx->CR1 |= I2C_CR1_STOP; /* Send STOP condition */
            } else 
	        dev->I2Cx->DR = *buffer++; // next byte
	} while(len);

    }

    // Wait to make sure that STOP control bit has been cleared - bus released
    t = hal_micros();
    while (dev->I2Cx->CR1 & I2C_CR1_STOP ){
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(dev->I2Cx->SR1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost or bus error
            dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            return I2C_STOP_BERR; // bus error on STOP
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }

        if (hal_micros() - t > I2C_TIMEOUT) return I2C_ERR_STOP; // stop error

	I2C_Yield(0); 
    }

    /* wait while the bus is busy */
    t = hal_micros();
    while ((dev->I2Cx->SR2 & (I2C_FLAG_BUSY>>16) & FLAG_MASK) != 0) {
	if (hal_micros() - t > I2C_TIMEOUT) return I2C_STOP_BUSY; // bus busy after STOP
	
	I2C_Yield(0); 
    }

#ifdef I2C_DEBUG
    op_time = t;
#endif

    return I2C_OK;

err_exit:// after any error make STOP to release bus
    dev->I2Cx->CR1 |= I2C_CR1_STOP;                    /* Send STOP condition */
#ifdef I2C_DEBUG
    op_time = t; // time of failed operation start
    op_sr1 = sr1;
#endif
    return state;
}



static void i2c_isr_handler(const i2c_dev *dev){

    if(dma_get_isr_bits(dev->dma.stream_rx) & DMA_FLAG_TCIF) { // was receive
        dma_disable(dev->dma.stream_rx);

        dev->I2Cx->CR2 &= ~(I2C_CR2_LAST | I2C_CR2_DMAEN);        /* Disable I2C DMA request */

        dma_clear_isr_bits(dev->dma.stream_rx); 

        dev->I2Cx->CR1 |= I2C_CR1_STOP;     /* Send STOP condition */
        // transfer done!
    }
    
/*
    if(dma_get_isr_bits(dev->dma.stream_tx) & DMA_FLAG_TCIF) { // was transmit - not used
    }

*/
}

static void i2c1_isr_handler(){
    i2c_isr_handler(_I2C1);
}

static void i2c2_isr_handler(){
    i2c_isr_handler(_I2C2);
}


uint32_t i2c_read(const i2c_dev *dev, uint8_t addr, const uint8_t *tx_buff, uint8_t txlen, uint8_t *rx_buff, uint8_t rxlen)
{

    uint8_t *buffer8 = rx_buff;
    
    uint32_t state=I2C_ERROR; 
    bool dma_mode = /* (addr != HAL_BARO_MS5611_I2C_ADDR) && */ (rxlen < DMA_BUFSIZE) ; // MS5611 refuses to work in DMA mode

    uint16_t sr1;
    uint32_t t;
    
    // in case of DMA transfer
    dma_stream rx_stream = dev->dma.stream_rx;

    if(dma_mode) {

        //  проверить, не занят ли поток DMA перед использованием
        t = hal_micros();
        while(dma_is_stream_enabled(rx_stream) /* || dma_is_stream_enabled(dp.stream_tx) we don't use TX */  ) {
            // wait for transfer termination
            if (hal_micros() - t > I2C_TIMEOUT) {
                dma_disable(rx_stream);  // something went wrong so let it get stopped
                return 103; // DMA stream busy
            }
            I2C_Yield(0); 
        }

        // init DMA beforehand
        DMA_InitTypeDef DMA_InitStructure;
        DMA_StructInit(&DMA_InitStructure);
    
        dma_init(rx_stream); 
    
        dma_clear_isr_bits(rx_stream); 
    
        DMA_InitStructure.DMA_Channel               = dev->dma.channel;
        DMA_InitStructure.DMA_Memory0BaseAddr       = (uint32_t)dma_buffer;
        DMA_InitStructure.DMA_BufferSize            = rxlen;
        DMA_InitStructure.DMA_PeripheralBaseAddr    = (uint32_t)(&(dev->I2Cx->DR));
        DMA_InitStructure.DMA_PeripheralDataSize    = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize        = DMA_MemoryDataSize_Byte;
        DMA_InitStructure.DMA_PeripheralInc         = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc             = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_Mode                  = DMA_Mode_Normal;
        DMA_InitStructure.DMA_Priority              = DMA_Priority_High;
        DMA_InitStructure.DMA_FIFOMode              = DMA_FIFOMode_Disable;
        DMA_InitStructure.DMA_FIFOThreshold         = DMA_FIFOThreshold_Full;
        DMA_InitStructure.DMA_MemoryBurst           = DMA_MemoryBurst_Single;
        DMA_InitStructure.DMA_PeripheralBurst       = DMA_PeripheralBurst_Single;
        DMA_InitStructure.DMA_DIR                   = DMA_DIR_PeripheralToMemory;
        
        dma_init_transfer(rx_stream, &DMA_InitStructure);

        dma_attach_interrupt(rx_stream, (Handler)dev->dma_isr, DMA_CR_TCIE);

        dma_enable(rx_stream);
    } // DMA mode

    // While the bus is busy
    t = hal_micros();
    while ((dev->I2Cx->SR2 & (I2C_FLAG_BUSY>>16) & FLAG_MASK) != 0 ){ 
        if (hal_micros() - t > I2C_TIMEOUT)
	    return state; // 2 - bus busy

        I2C_Yield(0); 
    }

    state++; 

    dev->I2Cx->CR1 &= ~I2C_NACKPosition_Next; // I2C_NACKPosition_Current
    dev->I2Cx->CR1 |= I2C_CR1_ACK;      // Bus got!  enable Acknowledge for our operation
    
    dev->I2Cx->CR1 |= I2C_CR1_START;    // Send START condition

    // Test on EV5 and clear it (cleared by reading SR1 then writing to DR)
    t = hal_micros();
    while( ((sr1=dev->I2Cx->SR1) & I2C_FLAG_SB & FLAG_MASK) == 0) { // wait for start bit generated
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }
        if (hal_micros() - t > I2C_TIMEOUT)  return state; // 3 error Master can't be selected (bus has owner)
    }

    state++; 

    // Send address for write
    I2C_Send7bitAddress(dev->I2Cx, addr<<1, I2C_Direction_Transmitter );
    
    dev->I2Cx->CR1 &= (uint16_t)(~I2C_CR1_STOP);    /* clear STOP condition */

    t = hal_micros();
    // Test on EV6 and clear it
    while ( ((sr1 = dev->I2Cx->SR1) & I2C_FLAG_ADDR & FLAG_MASK) == 0)  {
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(sr1 & I2C_FLAG_AF & FLAG_MASK) {
            dev->I2Cx->SR1 = ~I2C_SR1_AF; // reset it
            state = I2C_NO_DEVICE; // Acknolege Failed
            goto err_exit;

        }
        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }

        if (hal_micros() - t > I2C_TIMEOUT) {
            goto err_exit;  // 4 TX mode not acknoleged
        }
    }

    state++; 

    /* Clear ADDR flag by reading SR1 then SR2 register (SR1 has already been read) */
    (void) dev->I2Cx->SR2;

    while( ((sr1=dev->I2Cx->SR1) & I2C_FLAG_TXE & FLAG_MASK)  == 0) { // wait for TX empty
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }

        if (hal_micros() - t > I2C_TIMEOUT) goto err_exit; // 5 - TXe not set on 1st byte
    }

    state++;

    while(tx_buff && txlen--) {
        dev->I2Cx->DR = *tx_buff++; // send next byte

        // Test on EV8 and clear it
        t = hal_micros();
        while( ((sr1=dev->I2Cx->SR1) & I2C_FLAG_TXE & FLAG_MASK) == 0) { // wait for TX empty
            if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

            if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                state = I2C_BUS_ERR;
                goto err_exit;
            }
            if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
                dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
                return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
            }

            if (hal_micros() - t > I2C_TIMEOUT)  goto err_exit; // 6 write error
        }
        
        state++; 
    }

    // Send START condition a second time
    dev->I2Cx->CR1 |= I2C_CR1_START;

    // Test on EV5 and clear it (cleared by reading SR1 then writing to DR)
    t = hal_micros();
    while( ((sr1=dev->I2Cx->SR1) & I2C_FLAG_SB & FLAG_MASK) == 0) { // wait for Restart generated - bit was Cleared by reading the SR1 register followed by writing the DR register
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }
        if (hal_micros() - t > I2C_TIMEOUT)   goto err_exit; // 7 restart error
    }

    state++; 

    // Send device address for read
    I2C_Send7bitAddress(dev->I2Cx, addr<<1, I2C_Direction_Receiver );

    dev->I2Cx->CR1 &= (uint16_t)(~I2C_CR1_STOP);    /* clear STOP condition - just to touch CR1 */

//[ wait for end of address sending
    t = hal_micros();
    while ( ((sr1 = dev->I2Cx->SR1) & I2C_FLAG_ADDR & FLAG_MASK) == 0)  {
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(sr1 & I2C_FLAG_AF & FLAG_MASK) {
            dev->I2Cx->SR1 = ~I2C_SR1_AF;
            goto err_exit; // 8 Acknolege Failed - not "no device"! this happens when we try to read non-existent register from chip
        }

        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }
	    
        if (hal_micros() - t > I2C_TIMEOUT) {
            state+=1; // 9 send read address error
            goto err_exit;
        }
    }
//]
 
    state+=2; // 10+ read data error

    if(dma_mode) {
//------------
        // let setup DMA mode now
        if(rxlen == 1) {                 // Disable Acknowledge 
            dev->I2Cx->CR1 &= ~I2C_CR1_ACK;
            dev->I2Cx->CR2 &= ~I2C_CR2_LAST; // disable DMA generated NACK
        } else if(rxlen == 2) {              // Disable Acknowledge and change NACK position
            dev->I2Cx->CR1 |= I2C_NACKPosition_Next; // move NACK to next byte
            dev->I2Cx->CR1 &= ~I2C_CR1_ACK;
            dev->I2Cx->CR2 &= ~I2C_CR2_LAST; // disable DMA generated NACK
        } else {                             // Enable ACK and Last DMA bit
            dev->I2Cx->CR1 |= I2C_CR1_ACK;
            dev->I2Cx->CR2 |= I2C_CR2_LAST;  
        }
        dev->I2Cx->CR2 |= I2C_CR2_DMAEN;    // Enable I2C RX request - all reads will be in DMA mode


        /* Clear ADDR bit by reading SR1 then SR2 register (SR1 has already been read) */
        (void) dev->I2Cx->SR2;
                
        t = hal_micros();
        // need to wait until DMA transfer complete */
        while ( dma_is_stream_enabled(rx_stream)) {
            if (hal_micros() - t > I2C_TIMEOUT) {
                dev->I2Cx->CR2 &= ~(I2C_CR2_LAST | I2C_CR2_DMAEN); // Disable I2C DMA request 
                dma_disable(rx_stream);
                dma_clear_isr_bits(rx_stream); 
                state = 100; // 100 DMA error
                goto err_exit;
            }
            I2C_Yield(i2c_bit_time * 8 * rxlen); // пока ждем пусть другие работают
        }

        //** DMA disabled and stop generated in ISR
            
        dma_detach_interrupt(rx_stream);

        memmove(rx_buff, dma_buffer, rxlen); // move to destination

    } else { // not DMA

//------------
        if (rxlen == 1) { // 1 byte reads - by hands

        // Disable Acknowledgement - send NACK for single byte BEFORE resetting ADDR
	    dev->I2Cx->CR1 &= ~I2C_CR1_ACK;

	    /* Clear ADDR flag by reading SR1 then SR2 register (SR1 has already been read) */
	    (void) dev->I2Cx->SR2;

	    dev->I2Cx->CR1 |= I2C_CR1_STOP;         // Send STOP condition after this byte

	    /* Wait for the byte to be received */
	    t = hal_micros();
	    while ( ((sr1=dev->I2Cx->SR1) & I2C_FLAG_RXNE & FLAG_MASK) == 0)  {
	        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

                if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                    dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                    state = I2C_BUS_ERR;
                    goto err_exit;  // 10 1st data byte read error
                }
	        if (hal_micros() - t > I2C_TIMEOUT) return state; // 10 read data error - STOP already generated
	    }

            state++;

	    /*!< Read the byte received  */
	    *buffer8 = (uint8_t)(dev->I2Cx->DR);

        } else if (rxlen==2) { // 2 byte reads - by hands, special case

/*
For 2-byte reception:
 Wait until ADDR = 1 (SCL stretched low until the ADDR flag is cleared)
 Set ACK low, set POS high
 Clear ADDR flag
 Wait until BTF = 1 (Data 1 in DR, Data2 in shift register, SCL stretched low until a data 1 is read)
 Set STOP high
 Read data 1 and 2

*/
	    // Disable Acknowledgement - send NACK for 2 bytes BEFORE resetting ADDR
	    dev->I2Cx->CR1 &= ~I2C_CR1_ACK; //      disable ACK
	    dev->I2Cx->CR1 |= I2C_NACKPosition_Next; // move NACK to next byte 

	    /* Clear ADDR register by reading SR1 then SR2 register (SR1 has already been read) */
	    (void) dev->I2Cx->SR2;

	    /* Wait for the byte to be received */
	    t = hal_micros();
	    while ( ((sr1=dev->I2Cx->SR1) & I2C_FLAG_RXNE & FLAG_MASK) == 0)  {
	        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

                if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                    dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                    state = I2C_BUS_ERR;
                    goto err_exit;
                }

	        if (hal_micros() - t > I2C_TIMEOUT) goto err_exit; // 10 read data error
	    }

            state++;

	    *buffer8++ = (uint8_t)(dev->I2Cx->DR);	/*!< Read the 1st byte received  */

	    dev->I2Cx->CR1 |= I2C_CR1_STOP;                 /* Send STOP condition */

	    /* Wait for the byte to be received */
	    t = hal_micros();
	    while ( ((sr1=dev->I2Cx->SR1) & I2C_FLAG_RXNE & FLAG_MASK) == 0)  {
	        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

                if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                    dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                    state = I2C_BUS_ERR;
                    goto err_exit;
                }
	        if (hal_micros() - t > I2C_TIMEOUT) return state; // 10 read data 2 error - stop already generated
	    }

            state++;
	
            *buffer8 = (uint8_t)(dev->I2Cx->DR);    /*!< Read the 2nd byte received  */

        } else { // More than 2 Byte Master Reception procedure 
    
            /* Clear ADDR bit by reading SR1 then SR2 register (SR1 has already been read) */
            (void) dev->I2Cx->SR2;

            do {
                /* Wait for the byte to be received */
	        t = hal_micros();
        	while ( ((sr1=dev->I2Cx->SR1) & I2C_FLAG_RXNE & FLAG_MASK) == 0)  {
        	    if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

                    if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                        dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                        state = I2C_BUS_ERR;
                        goto err_exit;
                    }

	            if (hal_micros() - t > I2C_TIMEOUT) {
	            //                byte 0  1  2  3  4  5  6
	                goto err_exit; // 10 11 12 13 14 15 16...
	            }
	        }
	     
	        state++; 

	        /*!< Read the byte received  */
	        *buffer8++ = (uint8_t)(dev->I2Cx->DR);
	        rxlen -= 1; // 1 byte done
	    
	    
	        if(rxlen == 1) { // last second byte
	            dev->I2Cx->CR1 &= ~I2C_CR1_ACK;     // Disable Acknowledgement - send NACK for last byte 

	            dev->I2Cx->CR1 |= I2C_CR1_STOP;     /* Send STOP condition after last byte */
	        }
            } while(rxlen);
        }      
    }

    // all transfers finished

    // Wait to make sure that STOP control bit has been cleared - bus released
    t = hal_micros();
    while (dev->I2Cx->CR1 & I2C_CR1_STOP ){
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(dev->I2Cx->SR1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost or bus error
            dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            return I2C_STOP_BERR; // bus error on STOP
        }
        if (hal_micros() - t > I2C_TIMEOUT) return I2C_ERR_STOP; // stop error

        I2C_Yield(0); 
    }

    /* wait while the bus is busy */
    t = hal_micros();
    while ((dev->I2Cx->SR2 & (I2C_FLAG_BUSY>>16) & FLAG_MASK) != 0) {
	if (hal_micros() - t > I2C_TIMEOUT) return I2C_STOP_BUSY; // bus busy after STOP
	
	I2C_Yield(0); 
    }
#ifdef I2C_DEBUG
    op_time = t;
#endif

    return I2C_OK;

err_exit:// after any error make STOP to release bus
    dev->I2Cx->CR1 |= I2C_CR1_STOP;                 /* Send STOP condition */
#ifdef I2C_DEBUG
    op_time = t; // time of failed operation start
    op_sr1 = sr1;
#endif
    return state;

}

/*
    errata 2.4.6
Spurious Bus Error detection in Master mode
Description
In Master mode, a bus error can be detected by mistake, so the BERR flag can be wrongly
raised in the status register. This will generate a spurious Bus Error interrupt if the interrupt
is enabled. A bus error detection has no effect on the transfer in Master mode, therefore the
I2C transfer can continue normally.
Workaround
If a bus error interrupt is generated in Master mode, the BERR flag must be cleared by
software. No other action is required and the on-going transfer can be handled normally

*/

void i2c_master_release_bus(const i2c_dev *dev) {
    gpio_write_bit(dev->gpio_port, dev->scl_pin, 1);
    gpio_write_bit(dev->gpio_port, dev->sda_pin, 1);
    gpio_set_mode(dev->gpio_port, dev->scl_pin, GPIO_OUTPUT_OD_PU);
    gpio_set_mode(dev->gpio_port, dev->sda_pin, GPIO_OUTPUT_OD_PU);
}


/**
 * @brief Reset an I2C bus.
 *
 * Reset is accomplished by clocking out pulses until any hung slaves
 * release SDA and SCL, then generating a START condition, then a STOP
 * condition.
 *
 * @param dev I2C device
 */
 
#define MAX_I2C_TIME 300 // 300ms before device turn off

bool i2c_bus_reset(const i2c_dev *dev) {

    /* Release both lines */
    i2c_master_release_bus(dev);

    uint32_t t=systick_uptime();

    /*
     * Make sure the bus is free by clocking it until any slaves release the
     * bus.
     */

again:
    /* Wait for any clock stretching to finish */
    while (!gpio_read_bit(dev->gpio_port, dev->scl_pin)) {// device can output 1 so check clock first
        if(systick_uptime()-t > MAX_I2C_TIME) return false;
        I2C_Yield(10);
    }
    delay_10us();	// 50kHz

    while (!gpio_read_bit(dev->gpio_port, dev->sda_pin)) {
        /* Wait for any clock stretching to finish */
        while (!gpio_read_bit(dev->gpio_port, dev->scl_pin)){
            if(systick_uptime()-t > MAX_I2C_TIME) return false;
            I2C_Yield(10);
        }
        delay_10us();	// 50kHz

        /* Pull low */
        gpio_write_bit(dev->gpio_port, dev->scl_pin, 0);
        delay_10us();

        /* Release high again */
        gpio_write_bit(dev->gpio_port, dev->scl_pin, 1);
        delay_10us();
    }

    /* Generate start then stop condition */
    gpio_write_bit(dev->gpio_port, dev->sda_pin, 0);
    delay_10us();
    gpio_write_bit(dev->gpio_port, dev->scl_pin, 0);
    delay_10us();
    gpio_write_bit(dev->gpio_port, dev->scl_pin, 1);
    delay_10us();
    gpio_write_bit(dev->gpio_port, dev->sda_pin, 1);
    
    uint32_t rtime = stopwatch_getticks();
    uint32_t dt    = us_ticks * 50; // 50uS

    while ((stopwatch_getticks() - rtime) < dt) {
        if (!gpio_read_bit(dev->gpio_port, dev->scl_pin))  goto again; // any SCL activity after STOP
    }

// we was generating signals on I2C bus, but BUSY flag senses it even when hardware is off
// datasheet: It indicates a communication in progress on the bus. This information is still updated when the interface is disabled (PE=0).

    dev->I2Cx->CR1 |= I2C_CR1_SWRST;           // set SoftReset for some time 
    I2C_Yield(10);
    dev->I2Cx->CR1 &= (uint16_t)(~I2C_CR1_SWRST); // clear SoftReset flag 
    return true;
}



