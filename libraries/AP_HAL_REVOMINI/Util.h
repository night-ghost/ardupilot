
#ifndef __AP_HAL_REVOMINI_UTIL_H__
#define __AP_HAL_REVOMINI_UTIL_H__

#include <AP_HAL/AP_HAL.h>
#include "AP_HAL_REVOMINI_Namespace.h"

#include <AP_Param/AP_Param.h>
#include <AP_Param_Helper/AP_Param_Helper.h>

extern "C" {
 void get_board_serial(uint8_t *serialid);
};

class REVOMINI::REVOMINIUtil : public AP_HAL::Util {
public:
    REVOMINIUtil(): gps_shift(0) {}

    bool run_debug_shell(AP_HAL::BetterStream *stream) { return false; } // shell in FC? you must be kidding!

    void set_soft_armed(const bool b) { 
        if(soft_armed != b){ 
            soft_armed = b;
            REVOMINIScheduler::arming_state_changed(b); 
        }
    }
    
    uint64_t get_system_clock_ms() const {
        int32_t offs=  hal_param_helper->_time_offset * 3600 * 1000; // in ms
        
        return AP_HAL::millis() + (gps_shift+500)/1000 + offs;
    }

    void set_system_clock(uint64_t time_utc_usec){
        gps_shift = time_utc_usec - REVOMINIScheduler::_micros64();
    }


    uint32_t available_memory(void) override
    {
        return 128*1024;
    }
    
    bool get_system_id(char buf[40])  override {
        uint8_t serialid[12];
        memset(serialid, 0, sizeof(serialid));
        get_board_serial(serialid);

        const char *board_type = BOARD_OWN_NAME;

        // this format is chosen to match the human_readable_serial()
        // function in auth.c
        snprintf(buf, 40, "%s %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X",
             board_type,
             (unsigned)serialid[0], (unsigned)serialid[1], (unsigned)serialid[2], (unsigned)serialid[3],
             (unsigned)serialid[4], (unsigned)serialid[5], (unsigned)serialid[6], (unsigned)serialid[7],
             (unsigned)serialid[8], (unsigned)serialid[9], (unsigned)serialid[10],(unsigned)serialid[11]);
        return true;
    }
    
    // create a new semaphore
    Semaphore *new_semaphore(void)  override { return new REVOMINI::Semaphore; } 
    
private:
    uint64_t gps_shift; // shift from board time to real time
};

#endif // __AP_HAL_REVOMINI_UTIL_H__
