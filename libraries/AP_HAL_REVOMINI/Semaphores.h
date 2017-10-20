
#ifndef __AP_HAL_REVOMINI_SEMAPHORES_H__
#define __AP_HAL_REVOMINI_SEMAPHORES_H__

#include <AP_HAL/AP_HAL.h>
#include "AP_HAL_REVOMINI_Namespace.h"
#include <exti.h>
#include "Config.h"


class REVOMINI::Semaphore : public AP_HAL::Semaphore {
public:
    Semaphore();
    bool give();
    bool take(uint32_t timeout_ms);
    bool take_nonblocking();

//[ functions that called only in SVC level so need not to disable interrupts
    bool svc_give();
    bool svc_take(uint32_t timeout_ms);
    bool svc_take_nonblocking();
    inline void *get_owner() { return _task; }       // task that owns this semaphore
    inline bool is_taken()   { return _taken; }
    inline bool is_waiting() { return _is_waiting; } // does anyone want this semaphore when it was busy?
//]
    static inline bool get_error(){ bool t=_error; _error=false; return t; }

#ifdef SEM_PROF 
    static uint64_t sem_time;    
#endif

protected:
    bool _take_from_mainloop(uint32_t timeout_ms);
    bool _take_nonblocking();
    bool _give();

    volatile bool _taken;
    void * _task; // owner
    bool _is_waiting;

    static bool _error;

};

#endif // __AP_HAL_REVOMINI_SEMAPHORES_H__
