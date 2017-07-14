#pragma once 

#include "RC_parser.h"
#include "RCInput.h"

#include <AP_HAL/HAL.h>

#ifdef BOARD_SPEKTRUM_RX_PIN

class REVOMINI::DSM_parser : public REVOMINI::_parser {
public:
    DSM_parser() {}

    void init(uint8_t ch);
    inline uint64_t get_last_signal() const    { noInterrupts(); uint64_t t= _dsm_last_signal; interrupts(); return t; } 
    inline uint64_t get_last_change() const    { noInterrupts(); uint64_t t=  last_dsm_change; interrupts(); return t; }
    inline uint8_t  get_valid_channels() const { noInterrupts(); uint8_t t=  _dsm_channels; interrupts(); return t; }
    inline uint16_t get_val(uint8_t ch)  const { noInterrupts(); uint16_t t=  _dsm_val[ch]; interrupts(); return t; }
    
    bool bind(int dsmMode) const override;

private:

    static REVOMINIUARTDriver uartSDriver; 
    static volatile uint64_t _dsm_last_signal;
    static volatile uint16_t _dsm_val[REVOMINI_RC_INPUT_NUM_CHANNELS];
    static volatile uint8_t  _dsm_channels;
    static uint64_t last_dsm_change;

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

