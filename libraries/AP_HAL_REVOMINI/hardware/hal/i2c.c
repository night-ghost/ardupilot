#include <i2c.h>
//#include "gpiopins.h"
#include "systick.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_dma.h"

DMA_InitTypeDef I2C1DMA_InitStructure;
DMA_InitTypeDef I2C2DMA_InitStructure;

#define TIMEOUT 500
//__IO uint32_t sTimeout = I2C_LONG_TIMEOUT;

//static i2c1_timeout = I2C_LONG_TIMEOUT;

static const i2c_dev i2c_dev1 = {
    .I2Cx         = I2C1,
    .gpio_port    = &gpiob,
    .sda_pin      = 9,
    .scl_pin      = 8,
    .clk       	  = RCC_APB1Periph_I2C1,
    .gpio_af	  = GPIO_AF_I2C1,
    .ev_nvic_line = I2C1_EV_IRQn,
    .er_nvic_line = I2C1_ER_IRQn,
};
/** I2C1 device */
const i2c_dev* const _I2C1 = &i2c_dev1;

//static i2c2_timeout = I2C_LONG_TIMEOUT;

static const i2c_dev i2c_dev2 = {
    .I2Cx         = I2C2,
    .gpio_port    = &gpiob,
    .sda_pin      = 11,
    .scl_pin      = 10,
    .clk       	  = RCC_APB1Periph_I2C2,
    .gpio_af	  = GPIO_AF_I2C2,
    .ev_nvic_line = I2C2_EV_IRQn,
    .er_nvic_line = I2C2_ER_IRQn,
};
/** I2C2 device */
const i2c_dev* const _I2C2 = &i2c_dev2;


typedef enum {TX = 0, RX = 1, TXREG = 2} I2C_Dir;

__IO uint8_t I2C_BLOCKED = 0;

/*
#define I2C_BUF_SIZE 16

__IO uint16_t sEEAddress = 0;
__IO uint32_t  sEETimeout = I2C_LONG_TIMEOUT;
__IO uint8_t   sEEDataNum;
*/

static void delay_10us(){
    stopwatch_delay_us(10);
}

/**
 * @brief  DeInitializes peripherals used by the I2C EEPROM driver.
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
    /*!< Configure I2C pins: SCL */
    GPIO_InitStructure.GPIO_Pin = BIT(dev->scl_pin);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
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
    /* GPIO configuration */
    /* Configure SCL */
    GPIO_InitStructure.GPIO_Pin = BIT(dev->scl_pin);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
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

    /* I2C configuration */
    I2C_StructInit(&I2C_InitStructure);

    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = address;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = speed;

    I2C_ITConfig(dev->I2Cx, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);

    /* I2C Peripheral Enable */
    I2C_Cmd(dev->I2Cx, ENABLE);
    /* Apply I2C configuration after enabling it */
    I2C_Init(dev->I2Cx, &I2C_InitStructure);

    I2C_BLOCKED = 0;
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


uint8_t i2c_is_busy() {
    return I2C_BLOCKED;
}


/* Send a buffer to the i2c port */
uint32_t i2c_write(const i2c_dev *dev, uint8_t addr, const uint8_t *tx_buff, uint8_t *len)
{

//    uint16_t sent = 0;
    const uint8_t *buffer = tx_buff;

    uint32_t state = I2C_ERROR;
    
    /*!< While the bus is busy */
    uint32_t timeout = I2C_LONG_TIMEOUT;
    while (I2C_GetFlagStatus(dev->I2Cx, I2C_FLAG_BUSY )) {
	if (timeout-- == 0)
	    return state;
	
	delay_10us();
    }

    state++;

    // Bus got!  enable Acknowledge for our operation
    I2C_AcknowledgeConfig(dev->I2Cx, ENABLE);

    // Send START condition
    I2C_GenerateSTART(dev->I2Cx, ENABLE);

    // Test on EV5 and clear it (cleared by reading SR1 then writing to DR)
    timeout = I2C_FLAG_TIMEOUT;
    while (!I2C_CheckEvent(dev->I2Cx, I2C_EVENT_MASTER_MODE_SELECT )) {
	if (timeout-- == 0)
	    return state;
    }


    state++;
    
    // Send address for write
    I2C_Send7bitAddress(dev->I2Cx, addr<<1, I2C_Direction_Transmitter );

    timeout = I2C_FLAG_TIMEOUT;
    // Test on EV6 and clear it
    while (!I2C_CheckEvent(dev->I2Cx,  I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED )){

        if(I2C_GetFlagStatus(dev->I2Cx, I2C_FLAG_AF ) != RESET) {
            dev->I2Cx->SR1 &= ~I2C_SR1_AF; // reset it
            
            I2C_GenerateSTOP(dev->I2Cx, ENABLE); // release bus
            return I2C_NO_DEVICE; // Acknolege Failed
        }

	if ((timeout--) == 0)
	    return state;
    }

    state++;

    I2C_SendData(dev->I2Cx, *buffer++); // 1st byte


    if ((*len) < 2) { // only 1 byte
	/* Send the current byte */
//	I2C_SendData(dev->I2Cx, *buffer); WTF??? @NG
	/* Point to the next byte to be written */
//	sent++;
	/* Test on EV8 and clear it */
	timeout = I2C_LONG_TIMEOUT;
//	while (!I2C_CheckEvent(dev->I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED )) {
	while(I2C_GetFlagStatus(dev->I2Cx, I2C_FLAG_BTF ) == RESET) { // wait for end of transmission
	    if ((timeout--) == 0)
		return state;
        }

        state++;

	/*!< STOP condition */
	I2C_GenerateSTOP(dev->I2Cx, DISABLE);
	I2C_ClearFlag(dev->I2Cx, I2C_FLAG_STOPF );
	/* Send STOP condition */
	I2C_GenerateSTOP(dev->I2Cx, ENABLE);
	(*len)=0; // done
    } else {
	do {
	    timeout = I2C_LONG_TIMEOUT;
	    while (!I2C_CheckEvent(dev->I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED )) {
	        if ((timeout--) == 0)
		    return state;
            }
            state++;

            if(--(*len) == 0) { // last is sent, no more bytes
            
	        timeout = I2C_LONG_TIMEOUT;
	        while(I2C_GetFlagStatus(dev->I2Cx, I2C_FLAG_BTF ) == RESET) { // wait for end of transmission
	            if ((timeout--) == 0)
		        return state;
                }
                                
		/*!< STOP condition */
		I2C_GenerateSTOP(dev->I2Cx, DISABLE);
		I2C_ClearFlag(dev->I2Cx, I2C_FLAG_STOPF );
		/* Send STOP condition */
		I2C_GenerateSTOP(dev->I2Cx, ENABLE);
            } else 
	        I2C_SendData(dev->I2Cx, *buffer++); // next byte
	} while((*len) );

    }

    return I2C_OK;
}


uint32_t i2c_read(const i2c_dev *dev, uint8_t addr, const uint8_t *tx_buff, uint8_t txlen, uint8_t *rx_buff, uint8_t *rxlen)
{

    uint8_t *buffer8 = rx_buff;
    
    uint32_t state=I2C_ERROR; 
    //uint32_t sr2;

    // While the bus is busy
    uint32_t timeout = I2C_FLAG_TIMEOUT;
    while (I2C_GetFlagStatus(dev->I2Cx, I2C_FLAG_BUSY )){ 
	if ((timeout--) == 0)
	    return state; // 2 - bus busy

	delay_10us();
    }

    state++; 

    // Bus got!  enable Acknowledge for our operation
    I2C_AcknowledgeConfig(dev->I2Cx, ENABLE);

    // Send START condition
    I2C_GenerateSTART(dev->I2Cx, ENABLE);

    // Test on EV5 and clear it (cleared by reading SR1 then writing to DR)
    timeout = I2C_LONG_TIMEOUT;
    while (!I2C_CheckEvent(dev->I2Cx, I2C_EVENT_MASTER_MODE_SELECT )){


	if ((timeout--) == 0)
	    return state; // 3 error Master can't be selected (bus has owner)
    }

    state++; 

    // Send address for write
    I2C_Send7bitAddress(dev->I2Cx, addr<<1, I2C_Direction_Transmitter );
    timeout = I2C_FLAG_TIMEOUT;
    // Test on EV6 and clear it
    while (!I2C_CheckEvent(dev->I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED )){
        if(I2C_GetFlagStatus(dev->I2Cx, I2C_FLAG_AF ) != RESET) {
            dev->I2Cx->SR1 &= ~I2C_SR1_AF; // reset it
            I2C_GenerateSTOP(dev->I2Cx, ENABLE); // release bus
            return I2C_NO_DEVICE; // Acknolege Failed
        }
	if ((timeout--) == 0)
	    return state;  // 4 TX mode not acknoleged
    }

    state++; 

    while(tx_buff && txlen--) {
        I2C_SendData(dev->I2Cx, *tx_buff++);

        // Test on EV8 and clear it
        timeout = I2C_LONG_TIMEOUT;
        while (I2C_GetFlagStatus(dev->I2Cx, I2C_FLAG_BTF ) == RESET){
            if ((timeout--) == 0)
                return state; // 5 write error
        }
        
        state++; 
    }

    // Send START condition a second time
    I2C_GenerateSTART(dev->I2Cx, ENABLE);

    // Test on EV5 and clear it (cleared by reading SR1 then writing to DR)
    timeout = I2C_FLAG_TIMEOUT;
    while (!I2C_CheckEvent(dev->I2Cx, I2C_EVENT_MASTER_MODE_SELECT )){
	if ((timeout--) == 0)
	    return state; // 6 restart error
    }

    state++; 

    // Send device address for read
    I2C_Send7bitAddress(dev->I2Cx, addr<<1, I2C_Direction_Receiver );


//[ wait for end of address sending
	timeout = I2C_LONG_TIMEOUT;
	while (I2C_GetFlagStatus(dev->I2Cx, I2C_FLAG_ADDR ) == RESET)  {
	
	    if(I2C_GetFlagStatus(dev->I2Cx, I2C_FLAG_AF ) != RESET) return state; // 7 Acknolege Failed
	    
	    if ((timeout--) == 0)
		return state+1; // 8 send read address error
	}
//]

 
    state+=2; // 9+ read data error

    if ((*rxlen) ==1) { // 1 byte reads - by hands

	// Disable Acknowledgement - send NACK for single byte BEFORE resetting ADDR
	I2C_AcknowledgeConfig(dev->I2Cx, DISABLE);

	/* Clear ADDR register by reading SR1 then SR2 register (SR1 has already been read) */
	(void) dev->I2Cx->SR2;


	/*!< STOP condition */
	I2C_GenerateSTOP(dev->I2Cx, DISABLE);
	I2C_ClearFlag(dev->I2Cx, I2C_FLAG_STOPF );
	/* Send STOP condition */
	I2C_GenerateSTOP(dev->I2Cx, ENABLE);

	/* Wait for the byte to be received */
	timeout = I2C_LONG_TIMEOUT;
	while (I2C_GetFlagStatus(dev->I2Cx, I2C_FLAG_RXNE ) == RESET)  {
	    if ((timeout--) == 0)
		return state; // 9 read data error
	}

        state++;

	/*!< Read the byte received  */
	*buffer8 = I2C_ReceiveData(dev->I2Cx);

	(*rxlen)--;
	/* Wait to make sure that STOP control bit has been cleared */
	timeout = I2C_FLAG_TIMEOUT;
	while (dev->I2Cx->CR1 & I2C_CR1_STOP ){
	    if ((timeout--) == 0)
		return state; // 10 stop error
	}

	(*rxlen)=0; // done

    } else if ((*rxlen) ==2) { // 2 byte reads - by hands, special case

/*
For 2-byte reception:
 Wait until ADDR = 1 (SCL stretched low until the ADDR flag is cleared)
 Set ACK low, set POS high
 Clear ADDR flag
 Wait until BTF = 1 (Data 1 in DR, Data2 in shift register, SCL stretched low until a data 1 is read)
 Set STOP high
 Read data 1 and 2

*/


	// Disable Acknowledgement - send NACK for single byte BEFORE resetting ADDR
	I2C_AcknowledgeConfig(dev->I2Cx, DISABLE);
	I2C_NACKPositionConfig(dev->I2Cx, I2C_NACKPosition_Next);

	/* Clear ADDR register by reading SR1 then SR2 register (SR1 has already been read) */
	(void) dev->I2Cx->SR2;

	/* Wait for the byte to be received */
	timeout = I2C_LONG_TIMEOUT;
	while (I2C_GetFlagStatus(dev->I2Cx, I2C_FLAG_RXNE ) == RESET)  {
	    if ((timeout--) == 0)
		return state; // 9 read data error
	}

        state++;

	/*!< Read the 1st byte received  */
	*buffer8++ = I2C_ReceiveData(dev->I2Cx);

	/*!< STOP condition */
	I2C_GenerateSTOP(dev->I2Cx, DISABLE);
	I2C_ClearFlag(dev->I2Cx, I2C_FLAG_STOPF );
	/* Send STOP condition */
	I2C_GenerateSTOP(dev->I2Cx, ENABLE);

	/* Wait for the byte to be received */
	timeout = I2C_LONG_TIMEOUT;
	while (I2C_GetFlagStatus(dev->I2Cx, I2C_FLAG_RXNE ) == RESET)  {
	    if ((timeout--) == 0)
		return state; // 10 read data 2 error
	}

        state++;


	/*!< Read the 2nd byte received  */
	*buffer8 = I2C_ReceiveData(dev->I2Cx);

	/* Wait to make sure that STOP control bit has been cleared */
	timeout = I2C_FLAG_TIMEOUT;
	while (dev->I2Cx->CR1 & I2C_CR1_STOP ){
	    if ((timeout--) == 0)
		return state; // 11 stop error
	}

	// Re-Enable Acknowledgement to be ready for another reception
//	I2C_AcknowledgeConfig(dev->I2Cx, ENABLE);
	I2C_NACKPositionConfig(dev->I2Cx, I2C_NACKPosition_Current);

	(*rxlen)=0; // done

    } else { // More than 2 Byte Master Reception procedure 
    
	/* Clear ADDR bit by reading SR1 then SR2 register (SR1 has already been read) */
	(void) dev->I2Cx->SR2;

        do {
    

	    /* Wait for the byte to be received */
	    timeout = I2C_LONG_TIMEOUT;
	    while (I2C_GetFlagStatus(dev->I2Cx, I2C_FLAG_RXNE ) == RESET)  { // wait for byte
	        if ((timeout--) == 0)
	        //               byte 0  1  2  3  4  5  6
	            return state; //  9 10 11 12 13 14 15...
	    }
	     
	    state++; 


	    /*!< Read the byte received  */
	    *buffer8++ = I2C_ReceiveData(dev->I2Cx);
	    *rxlen -= 1; // 1 byte done
	    
	    
	    if((*rxlen) == 1) { // last second byte
	        // Disable Acknowledgement - send NACK for last byte 
	        I2C_AcknowledgeConfig(dev->I2Cx, DISABLE);

	        /*!< STOP condition */
	        I2C_GenerateSTOP(dev->I2Cx, DISABLE);
	        I2C_ClearFlag(dev->I2Cx, I2C_FLAG_STOPF );
	        /* Send STOP condition */
	        I2C_GenerateSTOP(dev->I2Cx, ENABLE);	                
	    }

        } while((*rxlen));

    }
    return I2C_OK;
}


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
    }
    delay_10us();	// 50kHz

    while (!gpio_read_bit(dev->gpio_port, dev->sda_pin)) {
        /* Wait for any clock stretching to finish */
        while (!gpio_read_bit(dev->gpio_port, dev->scl_pin)){
            if(systick_uptime()-t > MAX_I2C_TIME) return false;
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
    
    return true;
}



