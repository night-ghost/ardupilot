/*
 * UART_OSD.cpp --- AP_HAL_REVOMINI OSD implementation via fake UART
 *
 */

#include <AP_HAL/AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_REVOMINI && defined(BOARD_OSD_NAME) && defined(BOARD_OSD_CS_PIN)

#include "UART_OSD.h"

#include "SPIDevice.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include <gpio_hal.h>

#include "osd/osd.h"

using namespace REVOMINI;
using namespace OSDns;

UART_OSD::UART_OSD():
    _initialized(false),
    _blocking(true)
{
}

void UART_OSD::begin(uint32_t baud) {

    OSDns::osd_begin(REVOMINI::SPIDeviceManager::_get_device(BOARD_OSD_NAME));

    void * task = REVOMINIScheduler::start_task(OSDns::osd_loop, SMALL_TASK_STACK); // 
#ifndef PREEMPTIVE
    REVOMINIScheduler::set_task_ttw(task, 10000); // 100Hz
#else
    REVOMINIScheduler::set_task_period(task, 20000); // 50Hz
    REVOMINIScheduler::set_task_priority(task, 101); // 1 less than main task
#endif
    _initialized = true;
}


/* REVOMINI implementations of Stream virtual methods */

uint32_t UART_OSD::available() {
    uint16_t v=OSDns::osd_available(); 
#ifndef PREEMPTIVE
    if(!v) REVOMINIScheduler::yield(); // если нет данных то переключим задачу насильно, все равно делать нечего
#endif
    return v;
}

int16_t UART_OSD::read() {
    if (available() <= 0)
        return -1;
    return OSDns::osd_getc();
}

size_t UART_OSD::write(uint8_t c) {

    if (REVOMINIScheduler::_in_timerprocess() || !_initialized) {      // not allowed from timers
        return 0;
    }

    OSDns::osd_putc(c);
     
    return 1;
}

size_t UART_OSD::write(const uint8_t *buffer, size_t size)
{
    size_t n = 0;
    while (size--) {
        n += write(*buffer++);
    }
    return n;
}

#endif // CONFIG_HAL_BOARD

