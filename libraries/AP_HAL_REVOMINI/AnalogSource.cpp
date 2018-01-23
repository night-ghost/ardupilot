/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
    (c) 2017 night_ghost@ykoctpa.ru
 
based on: Flymaple port by Mike McCauley
 */
#pragma GCC optimize ("O2")

#include <AP_HAL/AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_REVOMINI


#include "AP_HAL_REVOMINI.h"

#include "AP_HAL_REVOMINI_Namespace.h"

#include "AnalogIn.h"
#include <adc.h>
#include <boards.h>
#include <gpio_hal.h>
#include "GPIO.h"
#include <stm32f4xx.h>
#include "Scheduler.h"

#pragma GCC optimize ("O2")

extern const AP_HAL::HAL& hal;

using namespace REVOMINI;

REVOMINIAnalogSource::REVOMINIAnalogSource(uint8_t pin) :
    _sum_count(0),
    _sum(0),
    _latest(0),
    _last_average(0),
    _pin(ANALOG_INPUT_NONE),
    _stop_pin(ANALOG_INPUT_NONE),
    _settle_time_ms(0),
    _read_start_time_ms(0),
    _init_done(false)
{
    if(pin != ANALOG_INPUT_NONE) {
	set_pin(pin);
    }
}

/*
  return voltage from 0.0 to 3.3V, scaled to Vcc
 */
float REVOMINIAnalogSource::voltage_average(void)
{
    return voltage_average_ratiometric();
}

float REVOMINIAnalogSource::voltage_latest(void)
{
    return read_latest() * (3.3f / 4096.0f);
}

/*
  return voltage from 0.0 to 3.3V, assuming a ratiometric sensor. This
  means the result is really a pseudo-voltage, that assumes the supply
  voltage is exactly 3.3V.
 */
float REVOMINIAnalogSource::voltage_average_ratiometric(void)
{
    float v = read_average();
    return v * (3.3f / 4096.0f);
}

void REVOMINIAnalogSource::set_pin(uint8_t pin) {
    if (pin != _pin) {
	noInterrupts();
	_sum = 0;
	_sum_count = 0;
	_last_average = 0;
	_latest = 0;
	_pin = pin;
	interrupts();

	// ensure the pin is marked as an INPUT pin
	if (pin != ANALOG_INPUT_NONE && pin != ANALOG_INPUT_REVOMINI_VCC && pin < BOARD_NR_GPIO_PINS) {
            REVOMINIGPIO::_pinMode(pin, INPUT_ANALOG);
	}
	
	if (pin == ANALOG_INPUT_REVOMINI_VCC || (pin != ANALOG_INPUT_NONE &&  pin < BOARD_NR_GPIO_PINS)) {
            const adc_dev *dev = _find_device();

            if(dev) {
                adc_init(dev);
                adc_enable(dev);
            }	
        }        
        _init_done=true;
    }
}


/* read_average is called from the normal thread (not an interrupt). */
float REVOMINIAnalogSource::_read_average()
{

    if (_sum_count == 0) {
        // avoid blocking waiting for new samples
        return _last_average;
    }

    /* Read and clear in a critical section */
    EnterCriticalSection;
      _last_average = _sum / _sum_count;
    LeaveCriticalSection;
    
    return _last_average;
}


void REVOMINIAnalogSource::setup_read() {
    if (_stop_pin != ANALOG_INPUT_NONE && _stop_pin < BOARD_NR_GPIO_PINS) {

        const stm32_pin_info &p = PIN_MAP[_stop_pin];
        gpio_set_mode( p.gpio_device, p.gpio_bit, GPIO_OUTPUT_PP);
        gpio_write_bit(p.gpio_device, p.gpio_bit, 1);

    }
    if (_settle_time_ms != 0) {
        _read_start_time_ms = AP_HAL::millis();
    }
    const adc_dev *dev = _find_device();

    if (_pin == ANALOG_INPUT_REVOMINI_VCC){
	adc_set_reg_seqlen(dev, 1);

	  /* Enable Vrefint on Channel17 */
	ADC_RegularChannelConfig(dev->adcx, ADC_Channel_17, 1, ADC_SampleTime_84Cycles);
	ADC_TempSensorVrefintCmd(ENABLE);

	  /* Wait until ADC + Temp sensor start */
        REVOMINIScheduler::_delay_microseconds(10);

    } else if (_pin == ANALOG_INPUT_NONE) {
        // nothing to do
    } else if(dev != NULL && _pin < BOARD_NR_GPIO_PINS) {
	adc_set_reg_seqlen(dev, 1);
	uint8_t channel = PIN_MAP[_pin].adc_channel;
        ADC_RegularChannelConfig(dev->adcx, channel, 1, ADC_SampleTime_84Cycles);
	adc_enable(dev);
    }
}

void REVOMINIAnalogSource::stop_read() {
    if(_pin == ANALOG_INPUT_REVOMINI_VCC) {
	ADC_TempSensorVrefintCmd(DISABLE);
    }
    if (_stop_pin != ANALOG_INPUT_NONE && _stop_pin < BOARD_NR_GPIO_PINS) {
        const adc_dev *dev = _find_device();
	adc_disable(dev);
        const stm32_pin_info &p = PIN_MAP[_stop_pin];
        gpio_set_mode( p.gpio_device, p.gpio_bit, GPIO_OUTPUT_PP);
        gpio_write_bit(p.gpio_device, p.gpio_bit, 0);
    }
}

bool REVOMINIAnalogSource::reading_settled()
{
    if (_settle_time_ms != 0 && (AP_HAL::millis() - _read_start_time_ms) < _settle_time_ms) {
        return false;
    }
    return true;
}

/* new_sample is called from another process */
void REVOMINIAnalogSource::new_sample(uint16_t sample) {
    _latest = sample;

    EnterCriticalSection;
    _sum += sample;

//#define MAX_SUM_COUNT 16 // a legacy of the painfull 8-bit past
#define MAX_SUM_COUNT 64

    if (_sum_count >= MAX_SUM_COUNT) { // REVOMINI has a 12 bit ADC, so can only sum 16 in a uint16_t - and a 16*65536 in uint32_t
        _sum /= 2;
        _sum_count = MAX_SUM_COUNT/2;
    } else {
        _sum_count++;
    }
    LeaveCriticalSection;
}

#endif 
