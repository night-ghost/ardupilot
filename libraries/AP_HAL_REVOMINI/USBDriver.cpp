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



/* REVOMINI implementations of Stream virtual methods */

uint32_t USBDriver::available() { 
    uint32_t v = usb_data_available();
    if(!v) REVOMINIScheduler::yield(); // если нет данных то переключим задачу насильно, все равно делать нечего
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

/*
    size_t n=1;

    if(_usb_present && is_usb_opened()){
        uint16_t tr=3; // 3 попытки
        while(tr) {
            n = usb_putc(c);
            if(n==0) {
                REVOMINIScheduler::yield(); // пока ожидаем - пусть другие работают
                if(!_blocking || REVOMINIScheduler::_in_timerprocess()) tr--; // при неблокированном выводе уменьшим счетчик попыток
            } else break; // успешно отправили
        }        
        return n;
    } 
    return 1;
*/

    return write(&c,1);
}


size_t USBDriver::write(const uint8_t *buffer, size_t size)
{
    size_t n = 0;
    uint32_t t = REVOMINIScheduler::_millis();

    if(_usb_present && is_usb_opened()){
        while (size) {
//                n += write(*buffer++); we got 1-byte USB packets which are very slow
            uint8_t k=usb_write((uint8_t *)buffer, size);
            size-=k;
            n+=k;
            buffer+=k;
            
            if(!_blocking && REVOMINIScheduler::_millis() - t > 300 ){        // время ожидания превысило 300мс - что-то пошло не так...
                reset_usb_opened();
                return size;
            }
            REVOMINIScheduler::yield(); // пока ожидаем - пусть другие работают
        }
        return n;
    }
    return size;
}

#endif // CONFIG_HAL_BOARD
