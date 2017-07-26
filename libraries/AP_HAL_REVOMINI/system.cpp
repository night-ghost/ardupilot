#include <stdarg.h>
#include <stdio.h>

#include <AP_HAL/AP_HAL.h>
#include <AP_HAL/system.h>

#include "Scheduler.h"

extern const AP_HAL::HAL& hal;

using namespace REVOMINI;

//extern "C" void printf(const char *msg, ...);

namespace AP_HAL {

void init()
{
}

void panic(const char *errormsg, ...)
{
    /* Suspend timer processes. We still want the timer event to go off to run the _failsafe code, however. */
    va_list ap;

    hal.scheduler->suspend_timer_procs();

    if(boardEmergencyHandler) boardEmergencyHandler(); // call emergency handler before

    va_start(ap, errormsg);
    hal.console->vprintf(errormsg, ap);
    va_end(ap);
    hal.console->printf("\n");


    if(is_bare_metal())  // bare metal build without bootloader should reboot to DFU after any fault
        board_set_rtc_register(DFU_RTC_SIGNATURE, RTC_SIGNATURE_REG);

    error_throb(0);
}

uint32_t millis()
{
    REVOMINIScheduler::yield(50);
    return REVOMINIScheduler::_millis();
}

uint64_t millis64(){
    return REVOMINIScheduler::_millis64();
}

uint32_t micros() {
    return REVOMINIScheduler::_micros();
}

uint64_t micros64(){
    return REVOMINIScheduler::_micros64();
}

// revo internals

void delay(uint32_t ms){
    REVOMINIScheduler::_delay(ms); 
}

void     delay_microseconds(uint16_t us) { 
    REVOMINIScheduler::_delay_microseconds(us); 
}

void yield(uint32_t us){
    REVOMINIScheduler::yield(us); 
}


} // namespace AP_HAL

int printf(const char *msg, ...)
{
    /* Suspend timer processes. We still want the timer event to go off to run the _failsafe code, however. */
    va_list ap;

    va_start(ap, msg);
    hal.console->vprintf(msg, ap);
    va_end(ap);
    return 1;
}


