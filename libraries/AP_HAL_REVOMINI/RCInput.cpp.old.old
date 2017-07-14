#include <exti.h>
#include <timer.h>
#include "RCInput.h"
#include <pwm_in.h>
#include <AP_HAL/utility/dsm.h>
#include "sbus.h"
#include "GPIO.h"
#include "ring_buffer_pulse.h"

// Constructors ////////////////////////////////////////////////////////////////
using namespace AP_HAL;
using namespace REVOMINI;


/*
    DSM satellite connection
        1   2   3   4
pins    *   *   *   *   *   *   *
use    gnd vcc 26  103 xxx xxx xxx
DSM    GND     rx  en

*/


extern const AP_HAL::HAL& hal;

extern pulse_buffer pulses; // PPM data from interrupt

#define RISING_EDGE 1
#define FALLING_EDGE 0
#define MINONWIDTH 950
#define MAXONWIDTH 2075
// PATCH FOR FAILSAFE AND FRSKY
//#define MINOFFWIDTH 1000
//#define MAXOFFWIDTH 22000

#define MINCHECK 900
#define MAXCHECK 2100

/* private variables to communicate with input capture isr */
volatile uint16_t REVOMINIRCInput::_pulse_capt[REVOMINI_RC_INPUT_NUM_CHANNELS] IN_CCM /* = {0} */;
//volatile uint32_t REVOMINIRCInput::_last_pulse[REVOMINI_RC_INPUT_NUM_CHANNELS] IN_CCM /* = {0}*/ ;

static const uint8_t input_channels[]={ 
    4,  // PB14 T12/1 - PPM
    5,  // PB15 T12/2 - buzz 
    12, // PC6  T8/1  - 6_tx 
    13, // PC7  T8/2  - 6_rx 
    14, // PC8  T8/3  - Soft_scl 
    15, // PC9  T8/4  - Soft_sda
};



volatile uint16_t REVOMINIRCInput::_dsm_val[REVOMINI_RC_INPUT_NUM_CHANNELS] IN_CCM;

volatile uint64_t REVOMINIRCInput::_ppm_last_signal IN_CCM;
volatile uint64_t REVOMINIRCInput::_dsm_last_signal IN_CCM;

volatile uint8_t  REVOMINIRCInput::_valid_channels = 0;

REVOMINIUARTDriver REVOMINIRCInput::uartSDriver(_UART5);

struct REVOMINIRCInput::DSM        REVOMINIRCInput::dsm;
struct REVOMINIRCInput::SBUS       REVOMINIRCInput::sbus;
struct REVOMINIRCInput::SbusState  REVOMINIRCInput::sbus_state;
struct REVOMINIRCInput::DSM_State  REVOMINIRCInput::dsm_state;


bool REVOMINIRCInput::_got_ppm = false;
bool REVOMINIRCInput::_got_dsm = false;

bool REVOMINIRCInput::_was_ppm = false;
bool REVOMINIRCInput::_was_dsm = false;

uint16_t REVOMINIRCInput::_override[8];

uint64_t REVOMINIRCInput::_last_read;
bool REVOMINIRCInput::_override_valid;

unsigned int REVOMINIRCInput::ppm_sum_channel=0;

bool REVOMINIRCInput::is_PPM = true;


enum BOARD_RC_MODE REVOMINIRCInput::_rc_mode=BOARD_RC_NONE;


/* constrain captured pulse to be between min and max pulsewidth. */
static inline uint16_t constrain_pulse(uint16_t p) {
    if (p > RC_INPUT_MAX_PULSEWIDTH) return RC_INPUT_MAX_PULSEWIDTH;
    if (p < RC_INPUT_MIN_PULSEWIDTH) return RC_INPUT_MIN_PULSEWIDTH;
    return p;
}



bool REVOMINIRCInput::_process_ppmsum_pulse(uint16_t value)
{
    static uint8_t channel_ctr;

    if (value >= 2700) { // Frame synchronization
	if( channel_ctr >= REVOMINI_RC_INPUT_MIN_CHANNELS ) {
	    _valid_channels = channel_ctr;
	}
	channel_ctr = 0;
	_got_ppm=true;

        return true;	    
    } else if(value > 700 && value < 2300) {
        if (channel_ctr < REVOMINI_RC_INPUT_NUM_CHANNELS) {
    	    _ppm_last_signal =  systick_uptime();
            _pulse_capt[channel_ctr] = value;

            channel_ctr++;
            if (channel_ctr >= REVOMINI_RC_INPUT_NUM_CHANNELS) {
                _valid_channels = REVOMINI_RC_INPUT_NUM_CHANNELS;
            }
        }
        return true;
    } else { // try another protocols
        return false;
    }
}


void REVOMINIRCInput::rxIntRC(uint16_t value0, uint16_t value1, bool state)
{

    if(state && _rc_mode!=BOARD_RC_SBUS) { // falling and not SBUS detected
        if(_rc_mode==BOARD_RC_DSM || !_process_ppmsum_pulse( (value0 + value1) >>1 ) ) { // process PPM only if no DSM detected

            // not PPM - try treat as DSM
            _process_dsm_pulse(value0>>1, value1>>1);
        }
    } else { // rising
            // try treat as SBUS
            _process_sbus_pulse(value1>>1, value0>>1); // was 0 so now is length of 0, last is a length of 1
    }
}




void REVOMINIRCInput::parse_pulses(){
//    add_dsm_uart_input();

    static Pulse last_p={0,0};
    
    while(!pb_is_empty(&pulses)){
        Pulse p = pb_remove(&pulses);

// addHist(p.length);

        rxIntRC(last_p.length, p.length, p.state);
        last_p = p;
    }

}


/*
  process a SBUS input pulse of the given width
  
  pulses are captured on each edges and SBUS parser called on rising edge
  
*/
 
void REVOMINIRCInput::_process_sbus_pulse(uint16_t width_s0, uint16_t width_s1)
{
    // convert to bit widths, allowing for up to 4usec error, assuming 100000 bps - inverted
    uint16_t bits_s0 = (width_s0+4) / 10;
    uint16_t bits_s1 = (width_s1+4) / 10;

    uint8_t byte_ofs = sbus_state.bit_ofs/12;
    uint8_t bit_ofs  = sbus_state.bit_ofs%12;
    uint16_t nlow;

//hal.console->printf("\np %d\\%d", bits_s0, bits_s1);


    if (bits_s1 == 0 || bits_s0 == 0) {
        // invalid data
//hal.console->printf("\nreset 0");
        goto reset;
    }

//hal.console->printf("\nb %d.%d",  byte_ofs,  bit_ofs);

    if (bits_s1+bit_ofs > 10) { // invalid data as last two bits must be stop bits
//hal.console->printf("\nreset 1");
        goto reset;
    }
        

    // pull in the high bits
    sbus_state.bytes[byte_ofs] |= ((1U<<bits_s1)-1) << bit_ofs;
    sbus_state.bit_ofs += bits_s1;
    bit_ofs += bits_s1;

    // pull in the low bits
    nlow = bits_s0;
    if (nlow + bit_ofs > 12) {
        nlow = 12 - bit_ofs;
    }
    bits_s0 -= nlow;
    sbus_state.bit_ofs += nlow;

//hal.console->printf(" v=%x",  sbus_state.bytes[byte_ofs]);

    if (sbus_state.bit_ofs == 25*12 && bits_s0 > 12) { // all frame got and was gap
        // we have a full frame
        uint8_t bytes[25];
        uint16_t i;

//hal.console->printf("\ngot frame");

        for (i=0; i<25; i++) {
            // get inverted data
            uint16_t v = ~sbus_state.bytes[i];
    
            if ((v & 1) != 0) {        // check start bit
//hal.console->printf("\nreset 3");
                goto reset;
            }
            
            if ((v & 0xC00) != 0xC00) {// check stop bits
//hal.console->printf("\nreset 4 %d", i);
                goto reset;
            }
            // check parity
            uint8_t parity = 0, j;
            for (j=1; j<=8; j++) {
                parity ^= (v & (1U<<j))?1:0;
            }
            if (parity != (v&0x200)>>9) {
//hal.console->printf("\nreset 5  %d", i);
                goto reset;
            }
            bytes[i] = ((v>>1) & 0xFF);
        }

//hal.console->printf("\nframe");

        uint16_t values[REVOMINI_RC_INPUT_NUM_CHANNELS];
        uint16_t num_values=0;
        bool sbus_failsafe=false, sbus_frame_drop=false;


        if (sbus_decode(bytes, values, &num_values,
                        &sbus_failsafe, &sbus_frame_drop,
                        REVOMINI_RC_INPUT_NUM_CHANNELS) &&
            num_values >= REVOMINI_RC_INPUT_MIN_CHANNELS) 
        {

//hal.console->printf(" OK");

            for (i=0; i<num_values; i++) {
                _dsm_val[i] = values[i];
            }
            _valid_channels = num_values;
            
            _rc_mode = BOARD_RC_SBUS; // lock input mode, SBUS has a parity and other checks so false positive is unreal
            
            if (!sbus_failsafe) {
                _got_dsm = true;
                _dsm_last_signal = systick_uptime();
            }
        }
        goto reset_ok;
    } else if (bits_s0 > 12) { // Was inter-frame gap but not full frame 
//hal.console->printf("\nreset 6");
        goto reset;
    }
    return;
reset:
//    hal.console->printf("\nreset");

reset_ok:
    memset(&sbus_state, 0, sizeof(sbus_state));
}



/*
  process a DSM satellite input pulse of the given width
  
  pulses are captured on each edges and DSM parser called on falling edge - eg. beginning of start bit
  
*/

void REVOMINIRCInput::_process_dsm_pulse(uint16_t width_s0, uint16_t width_s1)
{
    // convert to bit widths, allowing for up to 1usec error, assuming 115200 bps
    uint16_t bits_s0 = ((width_s0+4)*(uint32_t)115200) / 1000000;
    uint16_t bits_s1 = ((width_s1+4)*(uint32_t)115200) / 1000000;
    uint8_t bit_ofs, byte_ofs;
    uint16_t nbits;

    if (bits_s0 == 0 || bits_s1 == 0) {
        // invalid data
        goto reset;
    }

    byte_ofs = dsm_state.bit_ofs/10;
    bit_ofs = dsm_state.bit_ofs%10;

    if(byte_ofs > 15) {
        // invalid data
        goto reset;
    }

    // pull in the high bits
    nbits = bits_s0;
    if (nbits+bit_ofs > 10) {
        nbits = 10 - bit_ofs;
    }
    dsm_state.bytes[byte_ofs] |= ((1U<<nbits)-1) << bit_ofs;
    dsm_state.bit_ofs += nbits;
    bit_ofs += nbits;


    if (bits_s0 - nbits > 10) {
        if (dsm_state.bit_ofs == 16*10) {
            // we have a full frame
            uint8_t bytes[16];
            uint8_t i;
            for (i=0; i<16; i++) {
                // get raw data
                uint16_t v = dsm_state.bytes[i];

                // check start bit
                if ((v & 1) != 0) {
                    goto reset;
                }
                // check stop bits
                if ((v & 0x200) != 0x200) {
                    goto reset;
                }
                bytes[i] = ((v>>1) & 0xFF);
            }
            uint16_t values[8];
            uint16_t num_values=0;
            if (dsm_decode(AP_HAL::micros64(), bytes, values, &num_values, 8) &&
                num_values >= REVOMINI_RC_INPUT_MIN_CHANNELS) {

                _rc_mode = BOARD_RC_DSM; // lock input mode, DSM has a checksum so false positive is unreal

                for (i=0; i<num_values; i++) {
                    _dsm_val[i] = values[i];
                }
                _valid_channels = num_values;
                _got_dsm = true;
                _dsm_last_signal = systick_uptime();
            }
        }
        memset(&dsm_state, 0, sizeof(dsm_state));
    }

    byte_ofs = dsm_state.bit_ofs/10;
    bit_ofs  = dsm_state.bit_ofs%10;

    if (bits_s1+bit_ofs > 10) {
        // invalid data
        goto reset;
    }

    // pull in the low bits
    dsm_state.bit_ofs += bits_s1;
    return;
reset:
    memset(&dsm_state, 0, sizeof(dsm_state));
}




REVOMINIRCInput::REVOMINIRCInput()
{   }

void REVOMINIRCInput::init() {

    memset((void *)&_pulse_capt[0], 0, sizeof(_pulse_capt));
//    memset((void *)&_last_pulse[0], 0, sizeof(_last_pulse));
    memset((void *)&_override[0],   0, sizeof(_override));
    memset((void *)&_dsm_val[0],    0, sizeof(_dsm_val));


    _ppm_last_signal=0;
    _dsm_last_signal=0;

    REVOMINIGPIO::_pinMode(BOARD_SPEKTRUM_PWR_PIN, OUTPUT);
    REVOMINIGPIO::_pinMode(BOARD_SPEKTRUM_RX_PIN, INPUT_PULLUP);
    REVOMINIGPIO::_write(BOARD_SPEKTRUM_PWR_PIN, BOARD_SPEKTRUM_PWR_ON);

    // initialize DSM UART
    uartSDriver.begin(115200);
    uartSDriver.setCallback(add_dsm_uart_input);

/* OPLINK AIR port pinout
1       2       3       4       5       6       7
                        
gnd    +5      26      103                     
               rx      pow
               int     cs
used as:

*/



/*
    initial check for pin2-pin3 bridge. If detected switch to PPMSUM  
    default to standard PWM
*/
    


#ifdef PWM_SUPPORTED // there is no pins 2&3 in RevoMini

    is_PPM = false;
    uint8_t channel3_status = 0;
    const uint8_t pin2  = 5; //input pin 2
    const uint8_t pin3  = 12;//input pin 3

    
    //set pin2 as output and pin 3 as input
    REVOMINIGPIO::_pinMode(pin2, OUTPUT);
    REVOMINIGPIO::_pinMode(pin3, INPUT);

    //default pin3 to 0
    REVOMINIGPIO::_write(pin3, 0);
    REVOMINIScheduler::_delay(1);

    //write 1 to pin 2 and read pin3
    REVOMINIGPIO::_write(pin2, 1);
    REVOMINIScheduler::_delay(1);
    //if pin3 is 1 increment counter
    if (REVOMINIGPIO::_read(pin3) == 1)
	channel3_status++;

    //write 0 to pin 2 and read pin3
    REVOMINIGPIO::_write(pin2, 0);
    REVOMINIScheduler::_delay(1);
    //if pin3 is 0 increment counter
    if (REVOMINIGPIO::_read(pin3) == 0)
	channel3_status++;

    //write 1 to pin 2 and read pin3
    REVOMINIGPIO::_write(pin2, 1);
    REVOMINIScheduler::_delay(1);
    //if pin3 is 1 increment counter
    if (REVOMINIGPIO::_read(pin3) == 1)
	channel3_status++;

    //if counter is 3 then we are in PPMSUM
    if (channel3_status == 3)
        is_PPM = true;
#else
    is_PPM=true;
#endif

#ifdef PWM_SUPPORTED
#define NUM_PWM_CHANNELS 6 // PWM_CHANNELS defined only in .c

    if (!is_PPM) { //PWM
	// Init Radio In
	hal.console->println("Init Default PWM");
        for (byte channel = 0; channel < NUM_PWM_CHANNELS; channel++)
            pinData[channel].edge = FALLING_EDGE;
    }
#endif    

    clear_overrides();

    pwmInit(is_PPM); // PPM sum mode
    
#ifdef BOARD_SPEKTRUM_PWR_PIN
    REVOMINIGPIO::_pinMode(BOARD_SPEKTRUM_PWR_PIN, OUTPUT);
    REVOMINIGPIO::_write(BOARD_SPEKTRUM_PWR_PIN, BOARD_SPEKTRUM_PWR_ON);
#endif

}

uint8_t REVOMINIRCInput::valid_channels()
{
#ifdef PWM_SUPPORTED
    if(!is_PPM)
	return 1;
    else
#endif
	return _valid_channels;

}

bool REVOMINIRCInput::new_input()
{
    parse_pulses();

    return _override_valid || _new_ppm_input() || _new_dsm_input();
}

bool REVOMINIRCInput::_new_ppm_input()
{
    bool valid = _got_ppm;
    if(valid) _was_ppm=true;
    return valid;
}


bool REVOMINIRCInput::_new_dsm_input()
{
    bool valid = _got_dsm;
    if(valid) _was_dsm=true;
    return valid;
}

uint8_t REVOMINIRCInput::num_channels()
{
#ifdef PWM_SUPPORTED
    if(is_PPM){
        return _valid_channels;
    } 
    // PWM
    noInterrupts();
    uint8_t n = _rcin.channel_count;
    interrupts(); 
    return n;
#else
    return _valid_channels;
#endif
}

//#define LOST_TIME 50 // this is wrong! Any packet lost and viola... 
#define LOST_TIME 500


uint16_t REVOMINIRCInput::read(uint8_t ch)
{
    uint16_t data=0;
    uint32_t pulse=0;
    
    parse_pulses();
    
    if(ch>=REVOMINI_RC_INPUT_NUM_CHANNELS) return 0;

    if(_was_ppm){

        noInterrupts();
    
        _last_read = _ppm_last_signal;
        _got_ppm=false;
    
        _override_valid = false;
#ifdef PWM_SUPPORTED
        if (!is_PPM) {
            data = pwmRead(ch, &pulse);
        }  else 
#endif    
        {
            data = _pulse_capt[ch];
            pulse = _ppm_last_signal;
        }
        interrupts();
    }
    
    if(_was_dsm){
        noInterrupts();
        _last_read = _dsm_last_signal;
        data = _dsm_val[ch];
        pulse = _dsm_last_signal;
        _got_dsm=false;
        interrupts();
    }
    
    /* Check for override */
    uint16_t over = _override[ch];
    if(over) return over;

    if( (ch == 2) && (systick_uptime() - pulse > LOST_TIME)) 
        data = 900;

    return data;
}

uint8_t REVOMINIRCInput::read(uint16_t* periods, uint8_t len)
{
    uint32_t pulse=0;
    
    parse_pulses();
    
    if(len > REVOMINI_RC_INPUT_NUM_CHANNELS) len = REVOMINI_RC_INPUT_NUM_CHANNELS; // limit count
    
    if(_was_ppm){

        noInterrupts(); // to not interfere with ISR for full data

#ifdef PWM_SUPPORTED
        if (!is_PPM) { // PWM
            uint16_t pulses[REVOMINI_RC_INPUT_NUM_CHANNELS];
            for (uint8_t i = 0; i < len; i++)
                periods[i] = pwmRead(i, &pulses[i]);
                
            pulse=pulses[2];
        } else 
#endif
        {        // PPM
            for (uint8_t i = 0; i < len; i++) 
                periods[i] = _pulse_capt[i];
        }
        pulse = _ppm_last_signal;
        _got_ppm =false;
        interrupts();
    }

    if(_was_dsm){
        noInterrupts();
        _last_read = _dsm_last_signal;
        for (uint8_t i = 0; i < len; i++) 
            periods[i] = _dsm_val[i];
        pulse = _dsm_last_signal;
        _got_dsm = false;
        interrupts();
    }
    
// throttle channel lost for more than LOST_TIME (ms) - failsafe

    if ((systick_uptime() - pulse  > LOST_TIME) )
        periods[2] = 900;


    for (uint8_t i = 0; i < len; i++) {
	if (_override[i] != 0)
	    periods[i] = _override[i];
    }

    return len;
}



bool REVOMINIRCInput::set_overrides(int16_t *overrides, uint8_t len)
{
    bool res = false;
    for (int i = 0; i < len; i++) {
        res |= set_override(i, overrides[i]);
    }
    return res;
}

bool REVOMINIRCInput::set_override(uint8_t channel, int16_t override)
{
    if (override < 0) return false; /* -1: no change. */
    if (channel < 8) {
        _override[channel] = override;
        if (override != 0) {
    	    _override_valid = true;
            return true;
        }
    }
    return false;
}

void REVOMINIRCInput::clear_overrides()
{
    for (int i = 0; i < 8; i++) {
	set_override(i, 0);
    }
}


/*
  add some bytes of input in DSM serial stream format, coping with partial packets
 */
void REVOMINIRCInput::add_dsm_uart_input() {
    
    while(uartSDriver.available()){
        
        // at least 1 bute we have
        
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
                        _dsm_val[i] = values[i];
                    }
                }
                /*
                  the apparent number of channels can change on DSM,
                  as they are spread across multiple frames. We just
                  use the max num_values we get
                 */
                if (num_values > _valid_channels) {
                    _valid_channels = num_values;
                }
                _dsm_last_signal =  REVOMINIScheduler::_millis64();
                _got_dsm = true;
#if 0
                HAP_PRINTF("Decoded DSM %u channels %u %u %u %u %u %u %u %u\n",
                           (unsigned)num_values,
                           values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7]);
#endif
            }
        }
    }
}



bool REVOMINIRCInput::rc_bind(int dsmMode){
    uartSDriver.end();
    
    REVOMINIGPIO::_write(BOARD_SPEKTRUM_PWR_PIN, BOARD_SPEKTRUM_PWR_OFF); /*power down DSM satellite*/

    REVOMINIScheduler::_delay(500);

    REVOMINIGPIO::_pinMode(BOARD_SPEKTRUM_RX_PIN, OUTPUT);           /*Set UART RX pin to active output mode*/

    REVOMINIGPIO::_write(BOARD_SPEKTRUM_PWR_PIN, BOARD_SPEKTRUM_PWR_ON);     /* power up DSM satellite*/

    REVOMINIScheduler::_delay(72);

    for (int i = 0; i < dsmMode; i++) {                         /*Pulse RX pin a number of times*/
	REVOMINIScheduler::_delay_microseconds(120);
	REVOMINIGPIO::_write(BOARD_SPEKTRUM_RX_PIN, 0);
	REVOMINIScheduler::_delay_microseconds(120);
	REVOMINIGPIO::_write(BOARD_SPEKTRUM_RX_PIN, 1);
    }

    REVOMINIScheduler::_delay(50);

    uartSDriver.begin(115200);                                  	/*Restore USART RX pin to RS232 receive mode*/

    return true;
}


