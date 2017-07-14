#ifndef __AP_HAL_REVOMINI_RCINPUT_H__
#define __AP_HAL_REVOMINI_RCINPUT_H__

#pragma GCC push_options
#pragma GCC optimize ("O2")
#include <AP_HAL/AP_HAL.h>

#include "AP_HAL_REVOMINI.h"
#include "UARTDriver.h"
#include <usart.h>
#include "RC_parser.h"

#pragma GCC pop_options

#define REVOMINI_RC_INPUT_MIN_CHANNELS 4
#define REVOMINI_RC_INPUT_NUM_CHANNELS 20


#define  RC_DEAD_TIME 60000 // 60 seconds no data changes

#ifndef BOARD_SPEKTRUM_RX_PIN
 #ifdef BOARD_DSM_USART
    #define BOARD_SPEKTRUM_RX_PIN (BOARD_DSM_USART->rx_pin)
 #endif
#endif

enum BOARD_RC_MODE {
    BOARD_RC_NONE=0,
    BOARD_RC_SBUS,
    BOARD_RC_DSM,
};



class REVOMINI::REVOMINIRCInput : public AP_HAL::RCInput {
public:
    REVOMINIRCInput();
    void init()  override;
    static void late_init(uint8_t b);

    uint16_t read(uint8_t ch) override;
    uint8_t  read(uint16_t* periods, uint8_t len) override;
    
    bool     new_input() override;
    uint8_t  num_channels() override;

    bool set_overrides(int16_t *overrides, uint8_t len) override;
    bool set_override(uint8_t channel, int16_t override) override;
    void clear_overrides() override;
    
    bool rc_bind(int dsmMode) override;
    
private:
    static _parser *parsers[];
    static uint8_t num_parsers;
    static uint8_t _last_read_from;
    
    static bool is_PPM;

    static uint64_t _last_read;
    static uint8_t  _valid_channels;


    uint16_t _read_dsm(uint8_t ch);
    uint16_t _read_ppm(uint8_t ch,uint8_t n);
    
    static uint16_t last_4;
    
    /* override state */
    static uint16_t _override[REVOMINI_RC_INPUT_NUM_CHANNELS];
    static bool _override_valid;
    

};

#endif // __AP_HAL_REVOMINI_RCINPUT_H__
