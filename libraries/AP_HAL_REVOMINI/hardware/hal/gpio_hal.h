#ifndef _GPIO_H
#define _GPIO_H

#include <stm32f4xx.h>
#include <hal.h>
#include "hal.h"



/**
 * @brief GPIO Pin modes.
 *
 * These only allow for 50MHZ max output speeds; if you want slower,
 * use direct register access.
 */
typedef enum gpio_pin_mode {
    GPIO_OUTPUT_PP, 		/**< Output push-pull. */
    GPIO_OUTPUT_OD, 		/**< Output open-drain. */
    GPIO_AF_OUTPUT_PP, 		/**< Alternate function output push-pull. */
    GPIO_AF_OUTPUT_OD, 		/**< Alternate function output open drain. */
    GPIO_INPUT_ANALOG, 		/**< Analog input. */
    GPIO_INPUT_FLOATING, 	/**< Input floating. */
    GPIO_INPUT_PD, 			/**< Input pull-down. */
    GPIO_INPUT_PU, 			/**< Input pull-up. */
    GPIO_OUTPUT_OD_PU, 		/**< Output open-drain with pullUp */
    GPIO_AF_OUTPUT_OD_PU, 	/**< Alternate function output open drain with pullup */
    /* GPIO_INPUT_PU treated as a special case, for ODR twiddling */
} gpio_pin_mode;


typedef uint8_t afio_remap_peripheral;

/**
 * @brief Debug port configuration
 *
 * Used to configure the behavior of JTAG and Serial Wire (SW) debug
 * ports and their associated GPIO pins.
 *
 * @see afio_cfg_debug_ports()
 */
typedef enum afio_debug_cfg {
    AFIO_DEBUG_FULL_SWJ,	 		/**< Full Serial Wire and JTAG debug */
    AFIO_DEBUG_FULL_SWJ_NO_NJRST, 	/**< Full Serial Wire and JTAG, but no NJTRST. */
    AFIO_DEBUG_SW_ONLY, 			/**< Serial Wire debug only (JTAG-DP disabled, SW-DP enabled) */
    AFIO_DEBUG_NONE					/**< No debug; all JTAG and SW pins are free for use as GPIOs. */
} afio_debug_cfg;


/** External interrupt trigger mode */
typedef enum exti_trigger_mode {
    EXTI_RISING,         /**< Trigger on the rising edge */
    EXTI_FALLING,        /**< Trigger on the falling edge */
    EXTI_RISING_FALLING  /**< Trigger on both the rising and falling edges */
} exti_trigger_mode;

/**
 * @brief External interrupt line port selector.
 *
 * Used to determine which GPIO port to map an external interrupt line
 * onto. */
/* (See AFIO sections, below) */
typedef enum afio_exti_port {
    AFIO_EXTI_PA,               /**< Use port A (PAx) pin. */
    AFIO_EXTI_PB,               /**< Use port B (PBx) pin. */
    AFIO_EXTI_PC,               /**< Use port C (PCx) pin. */
    AFIO_EXTI_PD,               /**< Use port D (PDx) pin. */
    AFIO_EXTI_PE,               /**< Use port E (PEx) pin. */
    AFIO_EXTI_PF,               /**< Use port E (PEx) pin. */
    AFIO_EXTI_PG,               /**< Use port E (PEx) pin. */    
} afio_exti_port;

/**
 * External interrupt line numbers.
 */
typedef enum afio_exti_num {
    AFIO_EXTI_0,                /**< External interrupt line 0. */
    AFIO_EXTI_1,                /**< External interrupt line 1. */
    AFIO_EXTI_2,                /**< External interrupt line 2. */
    AFIO_EXTI_3,                /**< External interrupt line 3. */
    AFIO_EXTI_4,                /**< External interrupt line 4. */
    AFIO_EXTI_5,                /**< External interrupt line 5. */
    AFIO_EXTI_6,                /**< External interrupt line 6. */
    AFIO_EXTI_7,                /**< External interrupt line 7. */
    AFIO_EXTI_8,                /**< External interrupt line 8. */
    AFIO_EXTI_9,                /**< External interrupt line 9. */
    AFIO_EXTI_10,               /**< External interrupt line 10. */
    AFIO_EXTI_11,               /**< External interrupt line 11. */
    AFIO_EXTI_12,               /**< External interrupt line 12. */
    AFIO_EXTI_13,               /**< External interrupt line 13. */
    AFIO_EXTI_14,               /**< External interrupt line 14. */
    AFIO_EXTI_15,               /**< External interrupt line 15. */
} afio_exti_num;

/** GPIO device type */
typedef struct gpio_dev {
    GPIO_TypeDef *GPIOx;      /**< Register map */
    uint32_t clk; 	      /**< RCC clock information */
    afio_exti_port exti_port; /**< AFIO external interrupt port value */
} gpio_dev;

#ifdef __cplusplus
  extern "C" {
#endif

extern const gpio_dev gpioa;
extern const gpio_dev* const _GPIOA;
extern const gpio_dev gpiob;
extern const gpio_dev* const _GPIOB;
extern const gpio_dev gpioc;
extern const gpio_dev* const _GPIOC;
extern const gpio_dev gpiod;
extern const gpio_dev* const _GPIOD;
extern const gpio_dev gpioe;
extern const gpio_dev* const _GPIOE;
extern const gpio_dev gpiof;
extern const gpio_dev* const _GPIOF;
extern const gpio_dev gpiog;
extern const gpio_dev* const _GPIOG;

/**
 * Initialize a GPIO device. 
 */
extern void gpio_init(const gpio_dev* const dev);

/**
 * Initialize and reset all available GPIO devices. 
 */
extern void gpio_init_all(void);

/**
 * Set the mode of a GPIO pin. 
 */
extern void gpio_set_mode(const gpio_dev* const dev, uint8_t pin, gpio_pin_mode mode);

/**
 * Initialize the AFIO clock, and reset the AFIO registers. 
 */
static inline void afio_init(void) 
{
}

/**
 * Set the alternate function mode of a GPIO pin.
 *
 * @param dev GPIO device.
 * @param pin Pin on the device whose mode to set, 0--15.
 * @param mode alternate function mode to set the pin to.
 * @see gpio_pin_mode
 */
void gpio_set_af_mode(const gpio_dev* const dev, uint8_t pin, uint8_t mode);

/**
 * Get the gpio device from port number
 */
extern const gpio_dev * gpio_get_gpio_dev(uint8_t port);

/**
 * Perform an alternate function remap. 
 */
void afio_remap(const gpio_dev* const dev, uint8_t pin, afio_remap_peripheral remapping);

/**
 * Enable or disable the JTAG and SW debug ports. 
 */
void afio_cfg_debug_ports(afio_debug_cfg config);					

static INLINE void gpio_write_bit(const gpio_dev* const dev, uint8_t pin, uint8_t val)
{
	/* Check the parameters */
    assert_param(IS_GPIO_ALL_PERIPH(dev->GPIOx));
    assert_param(IS_GPIO_PIN_SOURCE(pin));

    uint16_t bv = BIT(pin);
    
    if (val) {
	dev->GPIOx->BSRRL = bv;
    } else {
	dev->GPIOx->BSRRH = bv;
    }    
}

static INLINE uint8_t gpio_read_bit(const gpio_dev* const dev, uint8_t pin)
{
    /* Check the parameters */
    assert_param(IS_GPIO_ALL_PERIPH(dev->GPIOx));
    assert_param(IS_GPIO_PIN_SOURCE(pin));
 
    if ((dev->GPIOx->IDR & BIT(pin)) != Bit_RESET){
	return  (uint8_t)Bit_SET;
    } 

    return (uint8_t)Bit_RESET;

	
}

static inline void gpio_toggle_bit(const gpio_dev* const dev, uint8_t pin)
{
	/* Check the parameters */
    assert_param(IS_GPIO_ALL_PERIPH(dev->GPIOx));
    assert_param(IS_GPIO_PIN_SOURCE(pin));
    dev->GPIOx->ODR ^= BIT(pin);	
}

static inline afio_exti_port gpio_exti_port(const gpio_dev* const dev)
{
	/* Check the parameters */
    assert_param(IS_GPIO_ALL_PERIPH(dev->GPIOx));
    return dev->exti_port;
}


static inline void gpio_set_speed(const gpio_dev* const dev, uint8_t pin, GPIOSpeed_TypeDef gpio_speed){
/* Speed mode configuration */
    dev->GPIOx->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR0 << (pin * 2));
    dev->GPIOx->OSPEEDR |=  ((uint32_t)(gpio_speed) << (pin * 2));
}

static inline void afio_exti_select(afio_exti_num exti, afio_exti_port gpio_port)
{
	/* Check the parameters */
	assert_param(IS_EXTI_PIN_SOURCE(exti));
	assert_param(IS_EXTI_PORT_SOURCE(gpio_port));
		
	SYSCFG_EXTILineConfig(gpio_port, exti);
}



#ifdef __cplusplus
  }
#endif
 
#endif

