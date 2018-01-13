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
  Copied from: Flymaple port by Mike McCauley
 */

#pragma GCC optimize ("O2")



#include <AP_HAL/AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_REVOMINI

#include "AP_HAL_REVOMINI_Namespace.h"
#include "AnalogIn.h"
#include <adc.h>
#include <boards.h>
#include <gpio_hal.h>
#include "GPIO.h"
#include "Scheduler.h"
#include "c++.h"

extern const AP_HAL::HAL& hal;

using namespace REVOMINI;

/* CHANNEL_READ_REPEAT: how many reads on a channel before using the value.
 * This seems to be determined empirically */
#define CHANNEL_READ_REPEAT 1




REVOMINIAnalogSource* IN_CCM REVOMINIAnalogIn::_channels[REVOMINI_INPUT_MAX_CHANNELS];
REVOMINIAnalogSource  IN_CCM REVOMINIAnalogIn::_vcc(ANALOG_INPUT_REVOMINI_VCC);


REVOMINIAnalogIn::REVOMINIAnalogIn(){}


void REVOMINIAnalogIn::init() {

    // Register _timer_event in the scheduler. 
    void *_task = REVOMINIScheduler::start_task(FUNCTOR_BIND_MEMBER(&REVOMINIAnalogIn::_timer_event, void), 256); // small stack
    if(_task){
        REVOMINIScheduler::set_task_priority(_task, DRIVER_PRIORITY+1);  // slightly less
        REVOMINIScheduler::set_task_period(_task, 2000); // setting of period allows task to run
    }

    _register_channel(&_vcc);     // Register each private channel with REVOMINIAnalogIn. 
    
    cnv_started=false;
}


REVOMINIAnalogSource* REVOMINIAnalogIn::_create_channel(uint8_t chnum) {

#if 0
    REVOMINIAnalogSource *ch = new REVOMINIAnalogSource(chnum); 
#else
    caddr_t ptr = sbrk_ccm(sizeof(REVOMINIAnalogSource)); // allocate memory in CCM
    REVOMINIAnalogSource *ch = new(ptr) REVOMINIAnalogSource(chnum);
#endif

    _register_channel(ch);
    return ch;
}

void REVOMINIAnalogIn::_register_channel(REVOMINIAnalogSource* ch) {
    if (_num_channels >= REVOMINI_INPUT_MAX_CHANNELS) {
        AP_HAL::panic("Error: AP_HAL_REVOMINI::REVOMINIAnalogIn out of channels\r\n");
    }
    _channels[_num_channels] = ch;

    // *NO* need to lock to increment _num_channels INSPITE OF it is used by the interrupt to access _channels 
    // because we first fill _channels[]
    _num_channels++;
}


void REVOMINIAnalogIn::_timer_event(void)
{


    if (_num_channels == 0)      return;        /* No channels are registered - nothing to be done. */

    const adc_dev *dev = _channels[_active_channel]->_find_device();

    if (_channels[_active_channel]->_pin == ANALOG_INPUT_NONE || !_channels[_active_channel]->initialized()) {
        _channels[_active_channel]->new_sample(0);
        goto next_channel;
    }

    if (cnv_started && !(dev->adcx->SR & ADC_SR_EOC))	{
	    // ADC Conversion is still running - this should not happens, as we are called at 1khz.
	    // SO - more likely we forget to start conversion or some went wrong...
	    // let's fix it
	    adc_start_conv(dev);
	    return;
    }

    _channel_repeat_count++;
    if (_channel_repeat_count < CHANNEL_READ_REPEAT ||
        !_channels[_active_channel]->reading_settled())  {
        // Start a new conversion on the same channel, throw away the current conversion 
        adc_start_conv(dev);

        cnv_started=true;
        return;
    }

    _channel_repeat_count = 0;
    if (cnv_started)  { 
        uint16_t sample = (uint16_t)(dev->adcx->DR & ADC_DR_DATA);
        /* Give the active channel a new sample */
        _channels[_active_channel]->new_sample( sample );
    }
    
next_channel:
    
    _channels[_active_channel]->stop_read();            /* stop the previous channel, if a stop pin is defined */
    _active_channel = (_active_channel + 1) % _num_channels; /* Move to the next channel */
    _channels[_active_channel]->setup_read();                /* Setup the next channel's conversion */
    dev = _channels[_active_channel]->_find_device();

    if(dev != NULL) {
        /* Start conversion */
        adc_start_conv(dev);
        cnv_started=true;
    }

}


AP_HAL::AnalogSource* REVOMINIAnalogIn::channel(int16_t ch)
{
    if ((uint8_t)ch == ANALOG_INPUT_REVOMINI_VCC) {
        return &_vcc;
    } else {
        return _create_channel((uint8_t)ch);
    }
}

#endif
