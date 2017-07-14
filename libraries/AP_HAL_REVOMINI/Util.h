
#ifndef __AP_HAL_REVOMINI_UTIL_H__
#define __AP_HAL_REVOMINI_UTIL_H__

#include <AP_HAL/AP_HAL.h>
#include "AP_HAL_REVOMINI_Namespace.h"

extern "C" {
 void get_board_serial(uint8_t *serialid);
};

class REVOMINI::REVOMINIUtil : public AP_HAL::Util {
public:
    bool run_debug_shell(AP_HAL::BetterStream *stream) { return false; }
    
    uint64_t get_system_clock_ms() const {
        return AP_HAL::millis();
    }

    uint32_t available_memory(void) override
    {
        return 128*1024;
    }
    
    bool get_system_id(char buf[40])  override {
        uint8_t serialid[12];
        memset(serialid, 0, sizeof(serialid));
        get_board_serial(serialid);

        const char *board_type = "RevoMini";

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
};

#endif // __AP_HAL_REVOMINI_UTIL_H__
