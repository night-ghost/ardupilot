/*
(c) 2017 night_ghost@ykoctpa.ru
 

    a low-level interface for SD card driver

*/
#pragma GCC optimize ("O2")

#include <stdio.h>
#include <util.h>
#include <utility>

#include <systick.h>
#include <hal.h>

#include <AP_HAL/AP_HAL.h>
#include <AP_RTC/AP_RTC.h>

#if defined(BOARD_SDCARD_CS_PIN) || defined(BOARD_DATAFLASH_FATFS)
#include <GCS_MAVLink/GCS.h>

#include <AP_HAL_F4Light/AP_HAL_F4Light.h>
#include <AP_HAL_F4Light/SPIDevice.h>
#include <AP_HAL_F4Light/Scheduler.h>

#include "Sd2Card.h"
#include "SdFatFs.h"


using namespace F4Light;

extern const AP_HAL::HAL& hal;

static AP_HAL::OwnPtr<F4Light::SPIDevice> _spi;
AP_HAL::Semaphore                *_spi_sem;
static AP_HAL::Device::Speed _speed;


uint8_t spi_waitFor(uint8_t out, spi_WaitFunc cb, uint32_t dly) {
    return _spi->wait_for(out, cb, dly);
}

void spi_spiTransfer(const uint8_t *send, uint32_t send_len,  uint8_t *recv, uint32_t recv_len) {
  _spi->transfer(send, send_len,  recv, recv_len);
}

//------------------------------------------------------------------------------


void spi_yield(){
    hal_yield(0); // while we wait - let others work
}


uint8_t spi_detect(){
#ifdef  BOARD_SDCARD_DET_PIN
    const stm32_pin_info &pp = PIN_MAP[BOARD_SDCARD_DET_PIN];
    return gpio_read_bit(pp.gpio_device, pp.gpio_bit) == LOW;
#else
    return 1;
#endif
}






uint32_t get_fattime()
{
    uint64_t now;
    AP::rtc().get_utc_usec(now);
    now /= 1000; // in ms
    

    uint16_t year      = 1970;
    uint8_t month;

    uint64_t seconds = now / 1000;
    uint32_t sys_days    = seconds / (24*60*60uL);
    uint16_t day_seconds = seconds % (24*60*60uL);

    uint32_t days = sys_days;
    uint16_t y_day;

    while(1) {
        bool     leapYear   = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        uint16_t daysInYear = leapYear ? 366 : 365;

        if (days >= daysInYear)  {
            days      -= daysInYear;
            ++year;
        } else {
            y_day = days;

        /* calculate the month and day */
          static const uint8_t daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
          for(month = 0; month < 12; ++month) {
            uint8_t dim = daysInMonth[month];

            /* add a day to feburary if this is a leap year */
            if (month == 1 && leapYear)      ++dim;

            if (days >= dim)
              days -= dim;
            else
              break;
          }
          break;
        }
    }

    uint16_t min = day_seconds / 60;
    uint16_t hour = min /60;

    uint16_t sec = day_seconds % 60;
    
 
    /* Pack date and time into a uint32_t variable */
    return    ((uint32_t)(year - 1980) << 25)
            | ((uint32_t)month << 21)
            | ((uint32_t)days << 16)
            | ((uint32_t)hour << 11)
            | ((uint32_t)min << 5)
            | ((uint32_t)sec >> 1);
}

#endif


#if defined(BOARD_SDCARD_CS_PIN)

void spi_chipSelectHigh(void) {
    _spi->wait_busy();
    const stm32_pin_info &pp = PIN_MAP[BOARD_SDCARD_CS_PIN];
    gpio_write_bit(pp.gpio_device, pp.gpio_bit, HIGH);
    _spi_sem->give();
}

bool spi_chipSelectLow(bool take_sem) {
    if(take_sem){
        if(!_spi_sem->take(HAL_SEMAPHORE_BLOCK_FOREVER)) return false;

        _spi->set_speed(_speed);

    }
//  const stm32_pin_info &pp = PIN_MAP[BOARD_SDCARD_CS_PIN];
//  gpio_write_bit(pp.gpio_device, pp.gpio_bit, LOW);
    _spi->need_cs(true);        // assert CS before transfer
    return true;
}


uint8_t Sd2Card::init(AP_HAL::OwnPtr<F4Light::SPIDevice> spi) {

    _spi = std::move(spi);


  // set pin modes
    {
        const stm32_pin_info &pp = PIN_MAP[BOARD_SDCARD_CS_PIN];
        gpio_set_mode(pp.gpio_device, pp.gpio_bit, GPIO_OUTPUT_PP);
        gpio_set_speed(pp.gpio_device, pp.gpio_bit, GPIO_speed_100MHz);
        gpio_write_bit(pp.gpio_device, pp.gpio_bit, HIGH);
    }

#ifdef BOARD_SDCARD_DET_PIN
    {
        const stm32_pin_info &pp = PIN_MAP[BOARD_SDCARD_DET_PIN];
        gpio_set_mode(pp.gpio_device, pp.gpio_bit, GPIO_INPUT_PU);
        gpio_write_bit(pp.gpio_device, pp.gpio_bit, HIGH);
    }
#endif

    _spi_sem = _spi->get_semaphore();

    if(!_spi_sem->take(HAL_SEMAPHORE_BLOCK_FOREVER)) return false;

    //_spi->register_periodic_callback(1000, FUNCTOR_BIND_MEMBER(&Sd2Card::_timer, void));
    Revo_handler h = { .mp = FUNCTOR_BIND_MEMBER(&Sd2Card::_timer, void) };
    systick_attach_callback(h.h);

    _speed = AP_HAL::Device::SPEED_LOW; // all initialization at low speed
    _spi->set_speed(_speed);

    // must supply min of 74 clock cycles with CS high.
    for (uint8_t i = 0; i < 10; i++) {
         uint8_t b = 0XFF;
         _spi->transfer(&b, 1,  NULL, 0);
    }

    _spi_sem->give();

    uint8_t n_try=7;
    
    DSTATUS ret;
    do {
        ret = disk_initialize(0);    
    } while(ret!=RES_OK  && n_try-- != 0  );

    printf("\nSD initialize: status %d size %ldMb\n", ret, sectorCount()/2048UL);
    gcs().send_text(MAV_SEVERITY_INFO, "\nSD initialize: status %d size %ldMb\n", ret, sectorCount()/2048UL);

    _speed = AP_HAL::Device::SPEED_HIGH;

    return ret == RES_OK;
}

#elif defined(BOARD_DATAFLASH_FATFS)

#define DF_RESET BOARD_DATAFLASH_CS_PIN

void spi_chipSelectHigh(void) {
    _spi->wait_busy();
    const stm32_pin_info &pp = PIN_MAP[BOARD_DATAFLASH_CS_PIN];
    gpio_write_bit(pp.gpio_device, pp.gpio_bit, HIGH);
    _spi_sem->give();
}

bool spi_chipSelectLow(bool take_sem) {
    if(take_sem){
        if(!_spi_sem->take(HAL_SEMAPHORE_BLOCK_FOREVER)) return false;

        _spi->set_speed(_speed);

    }
//    const stm32_pin_info &pp = PIN_MAP[BOARD_DATAFLASH_CS_PIN];
//    gpio_write_bit(pp.gpio_device, pp.gpio_bit, LOW);
    _spi->need_cs(true);        // assert CS before transfer
    return true;
}

uint8_t Sd2Card::init(AP_HAL::OwnPtr<F4Light::SPIDevice> spi) {

    _spi = std::move(spi);

    GPIO::_pinMode(DF_RESET,OUTPUT);
    GPIO::_setSpeed(DF_RESET, GPIO_speed_100MHz);
    // Reset the chip. We don't need a semaphore because no SPI activity
    GPIO::_write(DF_RESET,0);
    Scheduler::_delay(1);
    GPIO::_write(DF_RESET,1);

    if (!_spi) {
        printf("DataFlash SPIDeviceDriver not found\n");
        return false;
    }

    _spi_sem = _spi->get_semaphore();
    if (!_spi_sem) {
        printf("DataFlash SPIDeviceDriver semaphore is null\n");
        return false;
    }

    if(!_spi_sem->take(HAL_SEMAPHORE_BLOCK_FOREVER)) return false; // just for check

    _spi_sem->give();

    Revo_handler h = { .mp = FUNCTOR_BIND_MEMBER(&Sd2Card::_timer, void) };
    systick_attach_callback(h.h); // not at common interrupt level as tasks because it can be called at USB interrupt level. 
    //                                  Systick has own interrupt level above all IRQ

    _speed = AP_HAL::Device::SPEED_HIGH;

    DSTATUS ret;
    ret = disk_initialize(0);    

    printf("\nDataFlash initialize: status %d size %ldMb\n", ret, sectorCount()/2048UL);
    gcs().send_text(MAV_SEVERITY_INFO, "\nDataFlash initialize: status %d size %ldMb\n", ret, sectorCount()/2048UL);

    return ret == RES_OK;
}

#endif
