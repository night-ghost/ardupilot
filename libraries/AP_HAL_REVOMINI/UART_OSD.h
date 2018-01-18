/*
 * UART_OSD.cpp --- AP_HAL_REVOMINI OSD implementation via fake UART
 *
 */

#ifndef __AP_HAL_REVOMINI_OSDUART_H__
#define __AP_HAL_REVOMINI_OSDUART_H__

#include <AP_HAL_REVOMINI/AP_HAL_REVOMINI.h>

#include <gpio_hal.h>
#include "Scheduler.h"
#include "osd/osd.h"



class REVOMINI::UART_OSD : public AP_HAL::UARTDriver  {
public:
    UART_OSD();

    void begin(uint32_t b);
    void inline begin(uint32_t b, uint16_t rxS, uint16_t txS) {   begin(b); }
    void inline end() { } // no way to unview
    void flush() {}
    bool inline is_initialized(){ return _initialized; }

    inline void set_blocking_writes(bool blocking) {  _blocking = blocking; }

    inline bool tx_pending() {   return 0; }

    uint32_t available() override;
    uint32_t inline  txspace() override {    return 255; }
    int16_t read() override;

    size_t write(uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);

private:

    bool _initialized;
    bool _blocking;
};

#endif 
