#pragma once 

#include "RC_parser.h"
#include "RCInput.h"

#include <AP_HAL/HAL.h>

#ifdef BOARD_SPEKTRUM_RX_PIN

class REVOMINI::DSM_parser : public REVOMINI::_parser {
public:
    DSM_parser() {}

    void init(uint8_t ch);
    
    bool bind(int dsmMode) const override;

private:

    static REVOMINIUARTDriver uartSDriver; 

    static void add_dsm_uart_input(); // add some DSM input bytes, for RCInput over a serial port
    static void _io_completion();
    static uint8_t _ioc;
    
    static struct DSM { // state of add_dsm_uart_input
        uint8_t frame[16];
        uint8_t partial_frame_count;
        uint64_t last_input_ms;
    } dsm;

    static void _rc_bind(uint16_t dsmMode);    
};
#endif

