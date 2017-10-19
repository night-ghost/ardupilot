/*
 * USBDriver.cpp --- AP_HAL_REVOMINI USB-UART driver.
 *
 */

#include <AP_HAL/AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_REVOMINI
#include "USBDriver.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <usb.h>
#include <gpio_hal.h>
#include "Scheduler.h"

using namespace REVOMINI;

extern const AP_HAL::HAL& hal;

extern void delay(uint32_t ms);


USBDriver::USBDriver(bool usb):
    _usb_present(usb),
    _initialized(false),
    _blocking(false)
{
}

void USBDriver::begin(uint32_t baud) {

    _usb_present = gpio_read_bit(PIN_MAP[BOARD_USB_SENSE].gpio_device,PIN_MAP[BOARD_USB_SENSE].gpio_bit);

    _initialized = true;
}


uint32_t USBDriver::available() { 
    uint32_t v = usb_data_available();
    return v; 
}

int16_t USBDriver::read() {
    if(_usb_present && is_usb_opened() ){
	if (available() == 0)
	    return -1;
	return usb_getc();
    }
    return 0;
}

size_t USBDriver::write(uint8_t c) {
    return write(&c,1);
}


size_t USBDriver::write(const uint8_t *buffer, size_t size)
{
    size_t n = 0;
    uint32_t t = REVOMINIScheduler::_millis();

    if(_usb_present && is_usb_opened()){
        while (size) {
            uint8_t k=usb_write((uint8_t *)buffer, size);
            size-=k;
            n+=k;
            buffer+=k;
            
            if(!_blocking && REVOMINIScheduler::_millis() - t > 300 ){        // время ожидания превысило 300мс - что-то пошло не так...
                reset_usb_opened();
                return n;
            } 
        }
        return n;
    }
    return size;
}

#endif // CONFIG_HAL_BOARD
