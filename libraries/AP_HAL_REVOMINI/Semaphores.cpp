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
#if 0
    if (REVOMINIScheduler::in_interrupt()) { // this should not happens
        return svc_give();
    }
#endif
    return _give(); // call to semafores from interrupt will cause HardFault
}

bool Semaphore::take_nonblocking() {       
    return _take_nonblocking(); 
}


bool Semaphore::take(uint32_t timeout_ms) {
#if 0
    if (REVOMINIScheduler::in_interrupt()) { // this should not happens
        if(svc_take_nonblocking()) {
            return true; // all OK if we got
        }
        
// let set global flag and check it in scheduler so reschedule such tasks in next tick        
        _error=true; // remember that it was        
        return false; 
    }
#endif
    return _take_from_mainloop(timeout_ms);
}

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
    REVOMINIScheduler::task_want_semaphore(_task, NULL, 0); 
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
    REVOMINIScheduler::task_want_semaphore(_task, this, timeout_ms); 
    return false;
}
