/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
/*
 * Copyright (C) 2015  Intel Corporation. All rights reserved.
 *
 * This file is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once


#include <AP_HAL/HAL.h>
#include "Semaphores.h"
#include "Scheduler.h"
#include <dma.h>
#include <spi.h>

namespace REVOMINI {

#define MAX_BUS_NUM 3

typedef enum SPIFrequency {
    SPI_18MHZ       = 0, /**< 18 MHz */
    SPI_9MHZ        = 1, /**< 9 MHz */
    SPI_4_5MHZ      = 2, /**< 4.5 MHz */
    SPI_2_25MHZ     = 3, /**< 2.25 MHz */
    SPI_1_125MHZ    = 4, /**< 1.125 MHz */
    SPI_562_500KHZ  = 5, /**< 562.500 KHz */
    SPI_281_250KHZ  = 6, /**< 281.250 KHz */
    SPI_140_625KHZ  = 7, /**< 140.625 KHz */
    SPI_36MHZ       = 8, /**< 36 MHz */
} SPIFrequency;

typedef uint8_t (*spi_WaitFunc)(uint8_t b);

struct spi_pins {
    uint8_t sck;
    uint8_t miso;
    uint8_t mosi;
};

struct SPIDesc {
    const char * const name;
    const spi_dev * const dev;
    uint8_t bus;
    spi_mode sm;
    uint16_t cs_pin;
    SPIFrequency lowspeed;
    SPIFrequency highspeed;
    uint8_t mode;  // mode of operations: 0 - polling, 1&2 DMA, 3 interrupts
    uint32_t prio; // DMA priority
};

enum SPI_ISR_MODE {
    SPI_ISR_NONE,
    SPI_ISR_STROBE,
    SPI_ISR_COMPARE,
    SPI_ISR_FINISH,
};

//#define  DEBUG_SPI    

#ifdef  DEBUG_SPI    
// for debug
struct spi_trans {
    const spi_dev * dev;
    uint32_t send_len;
    uint8_t  sent;
    uint8_t  sent1;
    uint32_t recv_len;
    uint8_t  recv0;
    uint8_t  recv1;
};

#define SPI_LOG_SIZE 200
#endif

class SPIDevice : public AP_HAL::SPIDevice {
public:
    SPIDevice(const SPIDesc &device_desc);

    ~SPIDevice() {}

    /* AP_HAL::SPIDevice implementation */

    bool set_speed(AP_HAL::Device::Speed speed) override;

    bool transfer(const uint8_t *send, uint32_t send_len,
                        uint8_t *recv, uint32_t recv_len) override;

    // one byte
    uint8_t transfer(uint8_t out);
    void send(uint8_t out); // without wait for answer

    /* See AP_HAL::SPIDevice::transfer_fullduplex() */
    bool transfer_fullduplex(const uint8_t *send, uint8_t *recv, uint32_t len) override;


    uint16_t send_strobe(const uint8_t *buffer, uint16_t len); // send in ISR and strobe each byte by CS
    void wait_busy() { spi_wait_busy(_desc.dev);  }
    uint8_t wait_for(uint8_t out, spi_WaitFunc cb, uint16_t dly); // wait for needed byte in ISR

    /* See AP_HAL::Device::get_semaphore() */
    inline REVOMINI::Semaphore *get_semaphore() { uint8_t n = _desc.bus - 1; if(n<MAX_BUS_NUM) { return &_semaphores[n];} else return NULL; } // numbers from 1

    /* See AP_HAL::Device::register_periodic_callback() */
    inline AP_HAL::Device::PeriodicHandle register_periodic_callback(uint32_t period_usec, AP_HAL::Device::PeriodicCb proc) override
    {
        return REVOMINIScheduler::register_timer_task(period_usec, proc, get_semaphore() );
    }

    inline bool adjust_periodic_callback(AP_HAL::Device::PeriodicHandle h, uint32_t period_usec) override
    {
        return REVOMINIScheduler::adjust_timer_task(h, period_usec);
    }

    inline bool unregister_callback(PeriodicHandle h) { return REVOMINIScheduler::unregister_timer_task(h); }

    void register_completion_callback(Handler h);

    inline void register_completion_callback(AP_HAL::MemberProc proc){
        Revo_handler r = { .mp=proc };
        register_completion_callback(r.h);
    }

    inline void register_completion_callback(AP_HAL::Proc proc){
        Revo_handler r = { .hp=proc };
        register_completion_callback(r.h);
    }

    void  dma_isr();
    void  spi_isr();

#define SPI_BUFFER_SIZE 512
    
protected:
    const SPIDesc &_desc;

    REVOMINIDigitalSource *_cs;
    SPIFrequency _speed;

    static REVOMINI::Semaphore _semaphores[MAX_BUS_NUM]; // per bus 
    static void * owner[MAX_BUS_NUM];
    static uint8_t buffer[MAX_BUS_NUM][SPI_BUFFER_SIZE];

    bool _initialized;
    uint8_t  byte_time; // in 0.25uS

    void init(void);

    inline void _cs_assert(){                   if(_cs){_cs->write(0); delay_ns100(1);} } // Select device and wait a little
    inline void _cs_release(){ spi_wait_busy(_desc.dev); if(_cs){      delay_ns100(5); _cs->write(1); } } // Deselect device, time from http://datasheetspdf.com/mobile/735133/MPU-6000.html page 19

    const spi_pins* dev_to_spi_pins(const spi_dev *dev);

    spi_baud_rate determine_baud_rate(SPIFrequency freq);

    uint8_t _transfer_s(uint8_t bt);
    uint8_t _transfer(uint8_t data);

    uint8_t dma_transfer(const uint8_t *send, const uint8_t *recv, uint32_t btr );
    
    Handler _completion_cb;
    void   *_task;

    
    // vars for send_strobe() and wait_for()
    const uint8_t *_send_address;
    uint16_t       _send_len;
    uint8_t *_recv_address;
    uint16_t       _recv_len;

    SPI_ISR_MODE   _isr_mode;
    spi_WaitFunc   _compare_cb;
    uint8_t        _recv_data;

    void isr_transfer_finish();
    void disable_dma();

#ifdef BOARD_SOFTWARE_SPI
    volatile GPIO_TypeDef *sck_port;
             uint16_t      sck_pin;

    volatile GPIO_TypeDef *mosi_port;
             uint16_t      mosi_pin;

    volatile GPIO_TypeDef *miso_port;
             uint16_t      miso_pin;

    uint16_t dly_time;
#endif

#ifdef DEBUG_SPI    
    static struct spi_trans spi_trans_array[SPI_LOG_SIZE];
    static uint8_t spi_trans_ptr;
#endif

};

class SPIDeviceManager : public AP_HAL::SPIDeviceManager {
public:
    friend class SPIDevice;

    SPIDeviceManager(){ }

    AP_HAL::OwnPtr<AP_HAL::SPIDevice> get_device(const char *name) { return _get_device(name); }
    static AP_HAL::OwnPtr<REVOMINI::SPIDevice> _get_device(const char *name);

protected:

};

}
