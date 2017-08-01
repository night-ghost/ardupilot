/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-

#pragma GCC optimize ("O2")

#include <AP_HAL_REVOMINI/AP_HAL_REVOMINI.h>
#include "Semaphores.h"
#include "Scheduler.h"

using namespace REVOMINI;

extern const AP_HAL::HAL& hal;

bool Semaphore::_error = false;

#ifdef SEM_PROF 
uint64_t Semaphore::sem_time=0;    
#endif

// Constructor
Semaphore::Semaphore() : _taken(false) {}


bool Semaphore::give() {
    bool result = false;
    if (_taken) {
        _taken = false;
        result=true;
    }
    return result;
}

bool Semaphore::take(uint32_t timeout_ms) {
    if (REVOMINIScheduler::_in_timerprocess()) {
        if(_take_nonblocking()) return true; // all OK if we got
        
/*
!!!    we got breaking changes from upstream - all drivers now calls ->take(WAIT_FOREVER) so I can't allow to kill UAV

        if(timeout_ms) {       // no panic in air! just return
             AP_HAL::panic("PANIC: Semaphore::take used from inside timer process");
*/

// let set global flag and check it in scheduler so reschedule such tasks in next tick        
            _error=true; // remember that it was
//                      or to add queue for such tasks and use setjmp/longjmp to emulate waiting in semaphore
            
            return false; 
//        } 
    }
    return _take_from_mainloop(timeout_ms);
}

bool Semaphore::_take_from_mainloop(uint32_t timeout_ms) {
    /* Try to take immediately */
    if (_take_nonblocking()) {
        return true;
    } 

    bool ret=false;

    uint32_t t  = REVOMINIScheduler::_micros(); 
    uint32_t dt = timeout_ms*1000; // timeout time

    do {
        REVOMINIScheduler::yield(100); // 100uS max task time - this is more useful  // REVOMINIScheduler::_delay_microseconds(10);
        if (_take_nonblocking()) {
            ret= true;
            break;
        }
    } while(timeout_ms == HAL_SEMAPHORE_BLOCK_FOREVER ||  (REVOMINIScheduler::_micros() - t) < dt);

#ifdef SEM_PROF 
    sem_time += REVOMINIScheduler::_micros()-t; // calculate semaphore wait time
#endif

    return ret;
}

bool Semaphore::_take_nonblocking() {

    noInterrupts();
    if (!_taken) {
        _taken = true;
        interrupts();
        return true;
    }
    interrupts();
    return false;
}

