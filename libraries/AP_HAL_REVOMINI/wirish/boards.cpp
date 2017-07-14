/******************************************************************************
 * The MIT License
 *
based on:
 
 * Copyright (c) 2010 Perry Hung.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/**
 * @brief Generic board initialization routines.
 *
 */

#include "boards.h"
#include "systick.h"
#include "gpio_hal.h"
#include "exti.h"
#include "timer.h"
#include "adc.h"
#include <usb.h>

static void setupFlash(void);
static void setupClocks(void);
static void setupNVIC(void);
static void enableFPU(void);
static void setupCCM(void);

void setupADC(void);
void setupTimers(void);
void usb_init(void);


void usb_init(void){


    usb_attr_t usb_attr;
    usb_open();

    usb_default_attr(&usb_attr);
    usb_attr.preempt_prio = 1;
    usb_attr.sub_prio = 3;
    usb_attr.use_present_pin = 1;
    usb_attr.present_port = PIN_MAP[BOARD_USB_SENSE].gpio_device;
    usb_attr.present_pin =  PIN_MAP[BOARD_USB_SENSE].gpio_bit;

    usb_configure(&usb_attr);

}

inline void enableFPU(void){
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
	SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));	// set CP10 and CP11 Full Access
#endif
}


inline static void setupFlash(void) { // all done in SetSysClock()
}

/*
 * Clock setup.  Note that some of this only takes effect if we're
 * running bare metal and the bootloader hasn't done it for us
 * already.
 *
 */
inline static void setupClocks() {
}


inline static void setupCCM(){
    extern unsigned _sccm,_eccm; // defined by link script

    RCC->AHB1ENR |= RCC_AHB1ENR_CCMDATARAMEN;
    asm volatile("dsb \n");

//    volatile unsigned *src = &_siccm; // CCM initializers in flash
    volatile unsigned *dest = &_sccm; // start of CCM

#if 0

    while (dest < &_eccm) {
        *dest = *src;
    }
//        for (src = &_data_loadaddr, dest = &_data;                 dest < &_edata;                 src++, dest++) {
#endif
    while (dest < &_eccm) {
        *dest++ = 0;
    }
}

inline static void setupNVIC() {
    /* 4 bit preemption,  0 bit subpriority */
    NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4);
    
    exti_init();
}


void board_set_rtc_register(uint32_t sig, uint16_t reg)
{

        RCC->APB1ENR |= RCC_APB1ENR_PWREN;
        // enable the backup registers.
        PWR->CR   |= PWR_CR_DBP;
        RCC->BDCR |= RCC_BDCR_RTCEN;
        RCC->AHB1ENR |= RCC_AHB1ENR_BKPSRAMEN;
        PWR_BackupAccessCmd(ENABLE);

//        RTC_WriteProtectionCmd(DISABLE);
        for(volatile int i=0; i<50; i++); // small delay
        
        RTC_WriteBackupRegister(reg, sig);

        PWR_BackupAccessCmd(DISABLE);

        // disable the backup registers
//        RCC->BDCR &= RCC_BDCR_RTCEN;
        PWR->CR   &= ~PWR_CR_DBP;
}


uint32_t board_get_rtc_register(uint16_t reg)
{
        // enable the backup registers.
        PWR->CR   |= PWR_CR_DBP;
        RCC->BDCR |= RCC_BDCR_RTCEN;

        uint32_t ret = RTC_ReadBackupRegister(reg);

        // disable the backup registers
//        RCC->BDCR &= RCC_BDCR_RTCEN;
        PWR->CR   &= ~PWR_CR_DBP;
        
        return ret;
}


// 1st executing function

void INLINE init(void) {
    setupCCM(); // needs because stack in CCM
    

    if(board_get_rtc_register(RTC_SIGNATURE_REG) == DFU_RTC_SIGNATURE) {
        board_set_rtc_register(0, RTC_SIGNATURE_REG);
        goDFU();        // just after reset - so all hardware is in boot state
    }


    setupFlash();  // empty
    setupClocks(); // empty

    SystemInit();
    SystemCoreClockUpdate();

    enableFPU();
    exti_init();
    setupNVIC();
    systick_init(SYSTICK_RELOAD_VAL);

    stopwatch_init(); // will use stopwatch_delay_us

    boardInit();
/*
     only CPU init here, all another moved to modules .init() functions
*/
}

void pre_init(){ // before any stack usage @NG

    init();
}

// частота неправильная и штатными функциями задержки мы не можем пользоваться
void emerg_delay(uint32_t n){
    volatile uint32_t i;

    while(n){
        for (i=4000; i!=0; i--) { // 16MHz, command each tick - ~4MHz or 0.25uS * 4000 = 1ms
            asm volatile("nop \n");
        }
    }
}


void NMI_Handler() {

    //Очищаем флаг прерывания CSS
    RCC->CIR |= RCC_CIR_CSSC;
    //Ждем некоторое время после сбоя, если он кратковременный
    //Возможно удастся перезапустить
    emerg_delay(100);  // clock is wrong so all micros() etc lies!


    //Пытаемся запустить HSE
    RCC_HSEConfig(RCC_HSE_ON);
    emerg_delay(1);     //Задержка на запуск кварца


    if (RCC_WaitForHSEStartUp() == SUCCESS){
        //Если запустился - проводим установку заново
        SetSysClock(); 

    } else {

// кварц не запустился, переключаемся на HSI и выставляем полную частоту

/* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N */
#define PLL_M      8
#define PLL_N      168

/* SYSCLK = PLL_VCO / PLL_P */
#define PLL_P      2

/* USB OTG FS, SDIO and RNG Clock =  PLL_VCO / PLLQ */
#define PLL_Q      7

        /* Enable high performance mode, System frequency up to 168 MHz */
        RCC->APB1ENR |= RCC_APB1ENR_PWREN;
        PWR->CR |= PWR_CR_PMODE;  

        /* HCLK = SYSCLK / 1*/
        RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
      
        /* PCLK2 = HCLK / 2*/
        RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;
    
        /* PCLK1 = HCLK / 4*/
        RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;

        /* Configure the main PLL */
        RCC->PLLCFGR = PLL_M | (PLL_N << 6) | (((PLL_P >> 1) -1) << 16) |
                   (RCC_PLLCFGR_PLLSRC_HSI) | (PLL_Q << 24);

        /* Enable the main PLL */
        RCC->CR |= RCC_CR_PLLON;

        /* Wait till the main PLL is ready */
        while((RCC->CR & RCC_CR_PLLRDY) == 0)   {   }
   
        /* Select the main PLL as system clock source */
        RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
        RCC->CFGR |= RCC_CFGR_SW_PLL;    
    }
}

