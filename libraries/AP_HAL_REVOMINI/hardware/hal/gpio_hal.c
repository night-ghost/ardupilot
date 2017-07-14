#include <gpio_hal.h>


/*
 * GPIO devices
 */

const gpio_dev gpioa = {
    .GPIOx     = GPIOA,
    .clk       = RCC_AHB1Periph_GPIOA,
    .exti_port = AFIO_EXTI_PA,
};
/** GPIO port A device. */
const gpio_dev* const _GPIOA = &gpioa;

const gpio_dev gpiob = {
    .GPIOx      = GPIOB,
    .clk       = RCC_AHB1Periph_GPIOB,
    .exti_port = AFIO_EXTI_PB,
};
/** GPIO port B device. */
const gpio_dev* const _GPIOB = &gpiob;

const gpio_dev gpioc = {
    .GPIOx      = GPIOC,
    .clk       = RCC_AHB1Periph_GPIOC,
    .exti_port = AFIO_EXTI_PC,
};
/** GPIO port C device. */
const gpio_dev* const _GPIOC = &gpioc;

const gpio_dev gpiod = {
    .GPIOx      = GPIOD,
    .clk       = RCC_AHB1Periph_GPIOD,
    .exti_port = AFIO_EXTI_PD,
};
/** GPIO port D device. */
const gpio_dev* const _GPIOD = &gpiod;

const gpio_dev gpioe = {
    .GPIOx      = GPIOE,
    .clk       = RCC_AHB1Periph_GPIOE,
    .exti_port = AFIO_EXTI_PE,
};
/** GPIO port E device. */
const gpio_dev* const _GPIOE = &gpioe;

const gpio_dev gpiof = {
    .GPIOx      = GPIOF,
    .clk       = RCC_AHB1Periph_GPIOF,
    .exti_port = AFIO_EXTI_PF,
};
/** GPIO port F device. */
const gpio_dev* const _GPIOF = &gpiof;

const gpio_dev gpiog = {
    .GPIOx      = GPIOG,
    .clk       = RCC_AHB1Periph_GPIOG,
    .exti_port = AFIO_EXTI_PG,
};
/** GPIO port G device. */
const gpio_dev* const _GPIOG = &gpiog;



static const gpio_dev* _gpios[] =  { &gpioa, &gpiob, &gpioc, &gpiod, &gpioe, &gpiof, &gpiog };



void gpio_init(const gpio_dev* const dev) 
{
	/* Check the parameters */
	assert_param(IS_GPIO_ALL_PERIPH(dev->GPIOx));
	GPIO_DeInit(dev->GPIOx);
	/* Enable the GPIO Clock  */
	RCC_AHB1PeriphClockCmd(dev->clk, ENABLE);
}

void gpio_init_all(void)
{
	GPIO_DeInit(GPIOA);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	GPIO_DeInit(GPIOB);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	GPIO_DeInit(GPIOC);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	GPIO_DeInit(GPIOD);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	GPIO_DeInit(GPIOE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
}

const gpio_dev * gpio_get_gpio_dev(uint8_t port)
{
    // Check the parameters 
    if(port <= sizeof(_gpios) / sizeof(gpio_dev*) ) {

/*
	gpio_dev *dev;
	 
	switch(port) {
		case 0: dev = _GPIOA;
				break;
		case 1: dev = _GPIOB;
				break;
		case 2: dev = _GPIOC;
				break;
		case 3: dev = _GPIOD;
				break;
		case 4: dev = _GPIOE;
				break;
		default:
			assert_param(0);
			//errno_r = EINVAL;
			dev = NULL;
	}		
	return dev;
*/
	return _gpios[port];
    } else
	return NULL;
    
}


void gpio_set_mode(const gpio_dev* const dev, uint8_t pin, gpio_pin_mode mode)
{
    /* Check the parameters */
    assert_param(IS_GPIO_ALL_PERIPH(dev->GPIOx));
    assert_param(IS_GPIO_PIN_SOURCE(pin));

    GPIO_InitTypeDef config;

    /* Enable the GPIO Clock  */
    RCC_AHB1PeriphClockCmd(dev->clk, ENABLE);
  
    /* Configure the pin */
    GPIO_StructInit(&config);
    config.GPIO_Speed = GPIO_Speed_50MHz;
	
    switch(mode) {
	case GPIO_OUTPUT_PP:
		config.GPIO_Mode = GPIO_Mode_OUT;
		config.GPIO_PuPd = GPIO_PuPd_NOPULL;
		config.GPIO_OType = GPIO_OType_PP;
		break;
	case GPIO_OUTPUT_OD:
		config.GPIO_Mode = GPIO_Mode_OUT;
		config.GPIO_PuPd = GPIO_PuPd_NOPULL;
		config.GPIO_OType = GPIO_OType_OD;
		break;
	case GPIO_OUTPUT_OD_PU:
		config.GPIO_Mode = GPIO_Mode_OUT;
		config.GPIO_PuPd = GPIO_PuPd_UP;
		config.GPIO_OType = GPIO_OType_OD;
		break;
	case GPIO_INPUT_FLOATING:
		config.GPIO_Mode = GPIO_Mode_IN;
		config.GPIO_PuPd = GPIO_PuPd_NOPULL;
		config.GPIO_OType = GPIO_OType_PP;
		break;
	case GPIO_INPUT_ANALOG:
		config.GPIO_Mode = GPIO_Mode_AN;
		config.GPIO_PuPd = GPIO_PuPd_NOPULL;
		config.GPIO_OType = GPIO_OType_PP;
		break;
	case GPIO_INPUT_PU:
		config.GPIO_Mode = GPIO_Mode_IN;
		config.GPIO_PuPd = GPIO_PuPd_UP;
		config.GPIO_OType = GPIO_OType_PP;
		break;
	case GPIO_INPUT_PD:
		config.GPIO_Mode = GPIO_Mode_IN;
		config.GPIO_PuPd = GPIO_PuPd_DOWN;
		config.GPIO_OType = GPIO_OType_PP;
		break;
	case GPIO_AF_OUTPUT_PP:
		config.GPIO_Mode = GPIO_Mode_AF;
		config.GPIO_PuPd = GPIO_PuPd_NOPULL;
		config.GPIO_OType = GPIO_OType_PP;
		break;
	case GPIO_AF_OUTPUT_OD:
		config.GPIO_Mode = GPIO_Mode_AF;
		config.GPIO_PuPd = GPIO_PuPd_NOPULL;
		config.GPIO_OType = GPIO_OType_OD;
		break;
	case GPIO_AF_OUTPUT_OD_PU:
		config.GPIO_Mode = GPIO_Mode_AF;
		config.GPIO_PuPd = GPIO_PuPd_UP;
		config.GPIO_OType = GPIO_OType_OD;
		break;
	default:
		//errno_r = EINVAL;
		return;
    }

    config.GPIO_Pin = BIT(pin);
    GPIO_Init(dev->GPIOx, &config);
}




void gpio_set_af_mode(const gpio_dev* const dev, uint8_t pin, uint8_t mode)
{
	/* Check the parameters */
    assert_param(IS_GPIO_ALL_PERIPH(dev->GPIOx));
    assert_param(IS_GPIO_PIN_SOURCE(pin));
    assert_param(IS_GPIO_AF(mode));
    
    /* Enable the GPIO Clock  */
    RCC_AHB1PeriphClockCmd(dev->clk, ENABLE);    
    GPIO_PinAFConfig(dev->GPIOx, pin, mode);
}


// not used
void afio_remap(const gpio_dev* const dev, uint8_t pin, afio_remap_peripheral remapping)
{
	/* Check the parameters */
    assert_param(IS_GPIO_ALL_PERIPH(dev->GPIOx));
    assert_param(IS_GPIO_PIN_SOURCE(pin));
    assert_param(IS_GPIO_AF(remapping));
    	
    /* Enable the GPIO Clock  */
    RCC_AHB1PeriphClockCmd(dev->clk, ENABLE);    	
    GPIO_PinAFConfig(dev->GPIOx, pin, remapping);
}

void afio_cfg_debug_ports(afio_debug_cfg config)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	switch(config) {
	case AFIO_DEBUG_NONE:
		/* Enable GPIOA and GPIOB clocks */
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB , ENABLE);
		
		/* Configure PA.13 (JTMS/SWDIO), PA.14 (JTCK/SWCLK) and PA.15 (JTDI) as output push-pull */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOA, &GPIO_InitStructure);

		/* Configure PB.03 (JTDO) and PB.04 (JTRST) as output push-pull */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
		GPIO_Init(GPIOB, &GPIO_InitStructure);
		break;

	case AFIO_DEBUG_SW_ONLY:
		/* Enable GPIOA clocks */
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA , ENABLE);
		
		/* Configure PA.13 (JTMS/SWDIO), PA.14 (JTCK/SWCLK) as output push-pull */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
		break;
		
	case AFIO_DEBUG_FULL_SWJ_NO_NJRST:
		/* Enable GPIOB clocks */
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB , ENABLE);
			
		/* Configure PB.04 (JTRST) as output push-pull */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOB, &GPIO_InitStructure);
		break;
	case AFIO_DEBUG_FULL_SWJ:
		break;
	default:
		//errno_r = EINVAL;
		return;
	}
}
