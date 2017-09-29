
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

    inline void *get_owner(){ return _task; }
    
    static inline bool get_error(){ bool t=_error; _error=false; return t; }

    void lock(bool f);

#ifdef SEM_PROF 
    static uint64_t sem_time;    
#endif

protected:
    bool _take_from_mainloop(uint32_t timeout_ms);
    bool _take_nonblocking();

    volatile bool _taken;
    void * _task; // owner

    static bool _error;

};

#endif // __AP_HAL_REVOMINI_SEMAPHORES_H__
