/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdbool.h>
#include <stm32f4xx.h>
#include <hal.h>
#include "systick.h"
#include "Scheduler.h"

class Soft_I2C {
public:
    Soft_I2C( const gpio_dev *scl_dev, uint8_t scl_bit, const gpio_dev *sda_dev, uint8_t sda_bit);
    Soft_I2C();

    void init();

    uint32_t writeBuffer( uint8_t addr_, uint8_t reg_, uint8_t len_, const uint8_t *data);
    uint32_t write( uint8_t addr_, uint8_t reg, uint8_t data);
    uint32_t read( uint8_t addr_, uint8_t reg, uint8_t len, uint8_t* buf);
    uint32_t transfer(uint8_t  addr, uint8_t  send_len, const uint8_t *send, uint8_t len, uint8_t *buf);

    uint16_t getErrorCounter(void) { return i2cErrorCount; }

    bool bus_reset(void);
    
    void set_low_speed(bool s) { _dly_time = s?5:1; } 

private:
    const gpio_dev *_scl_dev;
    uint8_t         _scl_bit;
    const gpio_dev *_sda_dev;
    uint8_t         _sda_bit;

    volatile GPIO_TypeDef *scl_port; // for quick direct access to hardware
    uint16_t               scl_pin;
    volatile GPIO_TypeDef *sda_port;
    uint16_t               sda_pin;

    void _delay(void);
    uint16_t _dly_time;    
    bool _failed;

    volatile uint16_t i2cErrorCount = 0;

    bool _Start(void);
    bool  _Stop(void);
    bool  _Ack(void);
    bool  _NoAck(void);
    bool _WaitAck(void);
    bool  _SendByte(uint8_t bt);
    bool _ReceiveByte(uint8_t *bp);

    bool wait_scl();
};

