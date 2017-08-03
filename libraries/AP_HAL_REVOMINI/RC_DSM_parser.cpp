#include <exti.h>
#include <timer.h>
#include "RCInput.h"
#include <pwm_in.h>
#include <AP_HAL/utility/dsm.h>
#include "sbus.h"
#include "GPIO.h"
#include "ring_buffer_pulse.h"

#include "RC_DSM_parser.h"

using namespace AP_HAL;
using namespace REVOMINI;


extern const AP_HAL::HAL& hal;

#if defined(BOARD_SPEKTRUM_RX_PIN)
volatile uint16_t DSM_parser::_val[REVOMINI_RC_INPUT_NUM_CHANNELS] IN_CCM;
volatile uint64_t DSM_parser::_last_signal IN_CCM;
uint64_t          DSM_parser::_last_change IN_CCM;
volatile uint8_t  DSM_parser::_channels = 0;
struct DSM_parser::DSM        DSM_parser::dsm IN_CCM;
uint8_t           DSM_parser::_ioc=0;
#endif

#if defined(BOARD_DSM_USART)
REVOMINIUARTDriver DSM_parser::uartSDriver(BOARD_DSM_USART);
#elif defined(BOARD_SPEKTRUM_RX_PIN)
REVOMINIUARTDriver DSM_parser::uartSDriver(_UART5);
#endif



void DSM_parser::init(uint8_t ch)  {
#if defined(BOARD_SPEKTRUM_RX_PIN)

    memset((void *)&_val[0],    0, sizeof(_val));
    
    _last_signal=0;
    _last_change =0;

    uint32_t sig = board_get_rtc_register(RTC_DSM_BIND_REG);
    if( (sig & ~DSM_BIND_SIGN_MASK) == DSM_BIND_SIGNATURE) {
        board_set_rtc_register(0, RTC_DSM_BIND_REG);
        _rc_bind(sig & DSM_BIND_SIGN_MASK);
    }
    
    REVOMINIGPIO::_pinMode(BOARD_SPEKTRUM_RX_PIN, INPUT_PULLUP);
 #ifdef BOARD_SPEKTRUM_PWR_PIN
    REVOMINIGPIO::_pinMode(BOARD_SPEKTRUM_PWR_PIN, OUTPUT);
    REVOMINIGPIO::_write(BOARD_SPEKTRUM_PWR_PIN, BOARD_SPEKTRUM_PWR_ON);
 #endif

    _ioc = REVOMINIScheduler::register_io_completion(_io_completion);

    // initialize DSM UART
    uartSDriver.begin(115200);
    uartSDriver.setCallback(add_dsm_uart_input);
}

/*
  add some bytes of input in DSM serial stream format, coping with partial packets - UART input callback
 */
void DSM_parser::add_dsm_uart_input() {
    if(_ioc) {
        REVOMINIScheduler::do_io_completion(_ioc);
    }
}

void DSM_parser::_io_completion(){
    
    while(uartSDriver.available()){
        
        // at least 1 byte we have
        const uint8_t dsm_frame_size = sizeof(dsm.frame);

        uint32_t now = AP_HAL::millis();    
        if (now - dsm.last_input_ms > 5) {
            // resync based on time
            dsm.partial_frame_count = 0;
        }
        dsm.last_input_ms = now;
    
        if (dsm.partial_frame_count + 1 > dsm_frame_size) {
            return; // we can't add bytes to buffer
        }
        


        dsm.frame[dsm.partial_frame_count] = uartSDriver.read();
        dsm.partial_frame_count += 1;

	if (dsm.partial_frame_count == dsm_frame_size) {
            dsm.partial_frame_count = 0;
            uint16_t values[16] {};
            uint16_t num_values=0;
            if (dsm_decode(AP_HAL::micros64(), dsm.frame, values, &num_values, 16) &&
                num_values >= 5) {
                for (uint8_t i=0; i<num_values; i++) {
                    if (values[i] != 0) {
                        if(_val[i] != values[i]) _last_change = systick_uptime();
                        _val[i] = values[i];
                    }
                }
                /*
                  the apparent number of channels can change on DSM,
                  as they are spread across multiple frames. We just
                  use the max num_values we get
                 */
                if (num_values > _channels) {
                    _channels = num_values;
                }
                _last_signal = systick_uptime();
            }
        }
    }
#endif // defined(BOARD_SPEKTRUM_RX_PIN)
}



#ifdef BOARD_SPEKTRUM_RX_PIN
void DSM_parser::_rc_bind(uint16_t dsmMode){
    

    REVOMINIScheduler::_delay(72);

    for (int i = 0; i < dsmMode; i++) {                         /*Pulse RX pin a number of times*/
	REVOMINIScheduler::_delay_microseconds(120);
	REVOMINIGPIO::_write(BOARD_SPEKTRUM_RX_PIN, 0);
	REVOMINIScheduler::_delay_microseconds(120);
	REVOMINIGPIO::_write(BOARD_SPEKTRUM_RX_PIN, 1);
    }

    REVOMINIScheduler::_delay(50);

}



bool DSM_parser::bind(int dsmMode) const {
#ifdef BOARD_SPEKTRUM_PWR_PIN
    uartSDriver.end();

    REVOMINIGPIO::_write(BOARD_SPEKTRUM_PWR_PIN, BOARD_SPEKTRUM_PWR_OFF); /*power down DSM satellite*/

    REVOMINIScheduler::_delay(500);

    REVOMINIGPIO::_pinMode(BOARD_SPEKTRUM_RX_PIN, OUTPUT);           /*Set UART RX pin to active output mode*/

    REVOMINIGPIO::_write(BOARD_SPEKTRUM_PWR_PIN, BOARD_SPEKTRUM_PWR_ON);     /* power up DSM satellite*/

    _rc_bind(dsmMode);

    uartSDriver.begin(115200);                                  	/*Restore USART RX pin to RS232 receive mode*/

#else
    // store request to bing in BACKUP RAM
    board_set_rtc_register(DSM_BIND_SIGNATURE | ( dsmMode & DSM_BIND_SIGN_MASK), RTC_DSM_BIND_REG);
     
#endif
    return true;
}

#endif // BOARD_SPEKTRUM_RX_PIN
