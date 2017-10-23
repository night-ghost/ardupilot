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
Semaphore::Semaphore()
    : _taken(false)
    , _task(NULL)
    , _is_waiting(false)
{}


bool Semaphore::give() {
    if(REVOMINIScheduler::in_interrupt()) { // SVC from interrupt will cause HardFault, but we need to give 
        bool v=_is_waiting;
        bool ret=svc_give();                      // bus semaphores from IO_Complete ISR. This is atomic and don't break anything
        if(v) REVOMINIScheduler::context_switch_isr(); // if anyone waits for this semaphore reschedule tasks after interrupt
        return ret;
    }
    return _give(); 
}

bool Semaphore::take_nonblocking() {       
    return _take_nonblocking(); 
}

bool Semaphore::take(uint32_t timeout_ms) {
    // task switching can be asyncronous but we can't return to caller
    uint32_t now=REVOMINIScheduler::_micros();
    uint32_t dt = timeout_ms*1000;
    bool ret;
    do {
        ret = _take_from_mainloop(timeout_ms);
        if(ret) break;
    }while(REVOMINIScheduler::_micros()-now <dt || timeout_ms==HAL_SEMAPHORE_BLOCK_FOREVER);
    
    return ret;
}


// realization

bool NAKED Semaphore::_give() {
    asm volatile("svc 1 \r\n"
                 "bx lr \r\n");
}

bool NAKED Semaphore::_take_from_mainloop(uint32_t timeout_ms) {
    asm volatile("svc 2 \r\n"
                 "bx lr \r\n");

}

bool NAKED Semaphore::_take_nonblocking() {
    asm volatile("svc 3 \r\n"
                 "bx lr \r\n");
}


// this functions called only at SVC level so serialized by hardware and don't needs to disable interrupts

bool Semaphore::svc_give() {
    _is_waiting=false;
    if (_taken) {
        _taken = false;
        _task = NULL;
        return true;
    }
    return false;
}

bool Semaphore::svc_take_nonblocking() {
    void *me = REVOMINIScheduler::get_current_task();
    if (!_taken) {
        _taken = true;
        _task = me;     // remember task which owns semaphore 
        return true;
    }
    if(_task == me){     // the current task already owns this semaphore
        return true; 
    }
    _is_waiting=true;
    return false;
}

bool Semaphore::svc_take(uint32_t timeout_ms) {
    void *me = REVOMINIScheduler::get_current_task();
    if (!_taken) {
        _taken = true;
        _task = me; // remember task which owns semaphore 
        return true;
    }
    if(_task == me){     // the current task already owns this semaphore
        return true; 
    }
    _is_waiting=true;
    return false;
}
