#pragma once 

#include <AP_HAL/HAL.h>

#include "RC_parser.h"
#include "RCInput.h"


class REVOMINI::SBUS_parser : public REVOMINI::_parser {
public:
    SBUS_parser() {}

    void init(uint8_t ch);
    void late_init(uint8_t ch);
    inline uint64_t get_last_signal()    const { noInterrupts(); uint64_t t= _last_signal; interrupts(); return t; } 
    inline uint64_t get_last_change()    const { noInterrupts(); uint64_t t=  last_change; interrupts(); return t; }
    inline uint8_t  get_valid_channels() const { noInterrupts(); uint8_t  t=  _channels;   interrupts(); return t; }
    inline uint16_t get_val(uint8_t ch)  const { noInterrupts(); uint16_t t=  _val[ch];    interrupts(); return t; }
    
private:

    static REVOMINIUARTDriver uartSDriver; 
    static volatile uint64_t _last_signal;
    static volatile uint16_t _val[REVOMINI_RC_INPUT_NUM_CHANNELS];
    static volatile uint8_t  _channels;
    static uint64_t last_change;

    static void add_uart_input(); // add some input bytes, for SBUS over a serial port
    static void _io_completion();
    
    static uint8_t _ioc;
    
// state of add_sbus_input
    static struct SBUS {
        uint8_t frame[26];
        uint8_t partial_frame_count;
        uint32_t last_input_uS;
    } sbus;

};

