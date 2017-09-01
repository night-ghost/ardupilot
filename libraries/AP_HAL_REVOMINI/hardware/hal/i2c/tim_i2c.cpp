
#pragma GCC push_options
#pragma GCC optimize ("O2")

#include <AP_HAL/AP_HAL.h>

#include "tim_i2c.h"
#include <i2c.h>

// Software I2C driver
// Can be configured to use any suitable pins

using namespace REVOMINI;

extern const AP_HAL::HAL& hal;

#define SCL_H         {scl_port->BSRRL = scl_pin; }
#define SCL_L         {scl_port->BSRRH = scl_pin; }

#define SDA_H         {sda_port->BSRRL = sda_pin; }
#define SDA_L         {sda_port->BSRRH = sda_pin; }

#define SCL_read      ((scl_port->IDR & scl_pin)!=0)
#define SDA_read      ((sda_port->IDR & sda_pin)!=0)

#define I2C_yield(x)     hal_yield(x)

static void delay_10us(){
    hal_delay_microseconds(10);
}

bool Soft_I2C::_Start(void)
{
    SDA_H;            // just in case
    SCL_H;
    
    if (!SCL_read)       return false; // bus busy
    if (!SDA_read)       return false; // bus busy

    timer_resume(TIMER4); // all another in interrupt

    return true;
}

void Soft_I2C::_tick(TIM_TypeDef *tim) { // ISR
    Soft_I2C::inst->tick();
}

void Soft_I2C::tick() { // ISR
    if(wait_scl) {
        if(!SCL_read) return; // wait SCL
        wait_scl = false;
    }
    
    f_sda = ! f_sda;
    
    if(f_sda) { // time to write/read data
        next_state = (State)0;

        State s = state;
        state = (State) (state+1);  // change to next state
        
        switch(s){ 
        case RESTART:
            data = (_addr << 1) | I2C_Direction_Receiver;
            state = A_0L;
            // no break
        case START:
            SCL_L;
            break;

        case STOP:
            SDA_H;
            done    = true;
            timer_pause(TIMER4);
            result = I2C_OK;
            break;
        
        case  A_AH: // ACK for address - read on high level of SCL
            if(SDA_read){ // nack;
                 result = I2C_NO_DEVICE;
                 done    = true;
                 timer_pause(TIMER4);
            } else { // ack - will send data
                if(send_len) { // send first
                    data = *send++;
                    --send_len;
                    state=W_0L;
                } else { // just receive, all sent before
                    state=R_0L;
                }
            }
            break;
            
        case  R_AL: // do ACK for read byte
            *recv++ = data;
            recv_len--;

            if(recv_len) {
                SDA_L; // ACK
                next_state=R_0L;
            } else {
                SDA_H; // NACK on last byte
                next_state = STOP;
            }
            
            break;
            
        case  W_AH: // ACK for write byte - read on high level of SCL
            if(SDA_read){ // nack;
                 result = I2C_ERROR;
                 done    = true;
                 timer_pause(TIMER4);
            } else {
                if(send_len == 0) { // all data sent
                    if(recv_len) {
                        next_state=RESTART; // restart to read
                    } else {
                        next_state=STOP; // last byte sent
                    }
                } else { 
                    data = *send++; 
                    send_len--; 
                    next_state=W_0L; // beginning of next byte
                }    
            }
                
            break;
            
        // send address
        case    A_0L:
        case    A_1L:
        case    A_2L:
        case    A_3L:
        case    A_4L:
        case    A_5L:
        case    A_6L:
        case    A_7L:
        // byte write
        case    W_0L:
        case    W_1L:
        case    W_2L:
        case    W_3L:
        case    W_4L:
        case    W_5L:
        case    W_6L:
        case    W_7L:
            if (data & 0x80) { SDA_H; }
            else             { SDA_L; }
            data <<=1;
            break;
            
        // byte read  - when SCL is high
        case    R_0H:
        case    R_1H:
        case    R_2H:
        case    R_3H:
        case    R_4H:
        case    R_5H:
        case    R_6H:
        case    R_7H:
            data <<= 1;
            if (SDA_read) data |= 0x01;
            break;
        
        default: 
            break;
        }
        return; // data part is over
    }
    
    // SCL part
    switch(state){
    case RESTART:
        SDA_H; // release SDA to restart
        SCL_H;
        break;

    case START:  // nothing to do - SCL already high
        break;
            
    // address 
    case    A_0L: // high to low - end of state
    case    A_1L:
    case    A_2L:
    case    A_3L:
    case    A_4L:
    case    A_5L:
    case    A_6L:
    // byte write
    case    W_0L:
    case    W_1L:
    case    W_2L:
    case    W_3L:
    case    W_4L:
    case    W_5L:
    case    W_6L:
    // byte read
    case    R_0L:
    case    R_1L:
    case    R_2L:
    case    R_3L:
    case    R_4L:
    case    R_5L:
    case    R_6L:
    case    R_7L:
        SCL_L;
        break;

    case    A_AL: // ack bit. All state changes are after it
    case    W_AL:
    case    R_AL:
        SCL_L;

        if(next_state) {
            state=next_state;
            if(state==RESTART) SDA_H; // release SDA to restart
        }
        break;
        
    case  A_7L:
    case  W_7L:
        SCL_L;
        SDA_H; // release SDA to slave
        break;

    case    A_0H: // low to high - strobe
    case    A_1H:
    case    A_2H:
    case    A_3H:
    case    A_4H:
    case    A_5H:
    case    A_6H:
    case    A_7H:
    case    A_AH:

    case    W_0H: 
    case    W_1H:
    case    W_2H:
    case    W_3H:
    case    W_4H:
    case    W_5H:
    case    W_6H:
    case    W_7H:
    case    W_AH:

    case    R_0H: 
    case    R_1H:
    case    R_2H:
    case    R_3H:
    case    R_4H:
    case    R_5H:
    case    R_6H:
    case    R_7H:
    case    R_AH:
        SCL_H;
        if (!SCL_read) wait_scl=true;
        break;
        

    case STOP:
        SCL_H;        
        if (!SCL_read) wait_scl=true;
        break;
    }
    
}


// prepare but don't touch pins
Soft_I2C::Soft_I2C( const gpio_dev *scl_dev, uint8_t scl_bit, const gpio_dev *sda_dev, uint8_t sda_bit)
: _scl_dev(scl_dev)
, _scl_bit(scl_bit)
, _sda_dev(sda_dev)
, _sda_bit(sda_bit)
, _failed(false)
{
    sda_port = sda_dev->GPIOx;
    sda_pin  = 1<<sda_bit;

    scl_port = scl_dev->GPIOx;
    scl_pin  = 1<<scl_bit;
    
}

Soft_I2C::Soft_I2C() 
:   _scl_dev(NULL),  _sda_dev(NULL)
{ // empty constructor for 1st initialization
}

// start using
void Soft_I2C::init() {

    if(_scl_dev && _sda_dev){
        gpio_set_mode(_scl_dev, _scl_bit, GPIO_OUTPUT_OD_PU);
        gpio_set_mode(_sda_dev, _sda_bit, GPIO_OUTPUT_OD_PU);    
    }
    
    uint32_t freq = configTimeBase(TIMER4, 0, 10000);       //10MHz
//    Revo_handler h = { .mp = FUNCTOR_BIND_MEMBER(&Soft_I2C::tick, void) };
//    timer_attach_interrupt(TIMER4, TIMER_UPDATE_INTERRUPT, h.h, 2); // high priority
    timer_attach_interrupt(TIMER4, TIMER_UPDATE_INTERRUPT, _tick, 2); // high priority
    timer_set_reload(TIMER6, 2 * freq / 1000000);             // period to generate 2uS requests - 500kHz interrupts /4 = 125kHz I2C. 
                                                        //  I hope that there will be a time between interrupts :)
    timer_resume(TIMER4);
}

uint8_t  Soft_I2C::writeBuffer( uint8_t addr, uint8_t reg, uint8_t len, const uint8_t *buf)
{

    data = addr << 1 | I2C_Direction_Transmitter;
    recv_len=0;
    send_len=len;
    send=buf;
    _addr = addr;
    
    f_read=false;
    state=START;
    result = I2C_OK;
    wait_scl=false;
    f_sda = true;   // we did START touching sda
    _failed = false;
    done=false;

    if (!_Start()) {
        i2cErrorCount++;
        return I2C_ERROR;
    }


    uint32_t t = hal_micros();

    while(!done) {
        if(hal_micros() - t > I2C_TIMEOUT) {
            timer_pause(TIMER4);
            SCL_H;
            delay_10us();       // STOP on bus
            SDA_H;
            return I2C_ERROR;
        }
        hal_yield(0);
    }
    return result;

}


uint8_t Soft_I2C::read( uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{

    data = addr << 1 | I2C_Direction_Transmitter;
    recv_len=len;
    recv=buf;
    send_len=1;
    send=&reg;
    _addr = addr;
    
    f_read=false;
    state=START;
    result = I2C_OK;
    wait_scl=false;
    f_sda = true;   // we did START touching sda
    _failed = false;
    done=false;

    if (!_Start()) {
        i2cErrorCount++;
        return I2C_ERROR;
    }

    uint32_t t = hal_micros();

    while(!done) {
        if(hal_micros() - t > I2C_TIMEOUT) {
            timer_pause(TIMER4);
            SCL_H;
            delay_10us();       // STOP on bus
            SDA_H;
            return I2C_ERROR;
        }
        hal_yield(0);
    }
    return result;

}


uint8_t Soft_I2C::transfer(uint8_t  addr, uint8_t  _send_len, const uint8_t *_send, uint8_t len, uint8_t *buf){

    data = addr << 1 | I2C_Direction_Transmitter;
    recv_len=len;
    recv=buf;
    send_len=_send_len;
    send = _send;
    _addr = addr;
    
    f_read=false;
    state=START;
    result = I2C_OK;
    wait_scl=false;
    f_sda = true;   // we did START touching sda
    _failed = false;
    done=false;

    if (!_Start()) {
        i2cErrorCount++;
        return I2C_ERROR;
    }

    uint32_t t = hal_micros();

    while(!done) {
        if(hal_micros() - t > I2C_TIMEOUT) {
            timer_pause(TIMER4);
            SCL_H;
            delay_10us();       // STOP on bus
            SDA_H;
            return I2C_ERROR;
        }
        hal_yield(0);
    }
    return result;
}


#define MAX_I2C_TIME 300 // 300ms before device turn off

bool Soft_I2C::bus_reset(void) {

    uint32_t t=systick_uptime();

again:
    /* Wait for any clock stretching to finish */
    while (!SCL_read) {// device can output 1 so check clock first
        REVOMINIScheduler::yield(10); // пока ожидаем - пусть другие работают
        
        if(systick_uptime()-t > MAX_I2C_TIME) goto error;
    }

    delay_10us();       // 50kHz

    while (!SDA_read) {
        /* Wait for any clock stretching to finish */
        while (!SCL_read) {
            SCL_H; // may be another thread causes LOW
            REVOMINIScheduler::yield(10); // пока ожидаем - пусть другие работают

            if(systick_uptime()-t > MAX_I2C_TIME) goto error;
        }
            
        delay_10us();   // 50kHz

        /* Pull low */
        SCL_L;
        delay_10us();

        /* Release high again */
        SCL_H;
        delay_10us();
        SDA_H;
    }

    /* Generate start then stop condition */
    SDA_L;
    delay_10us();
    SCL_L;
    delay_10us();
    SCL_H;
    delay_10us();
    SDA_H;
    
    {
        uint32_t rtime = stopwatch_getticks();
        uint32_t dt    = us_ticks * 50; // 50uS

        while ((stopwatch_getticks() - rtime) < dt) {
            if (!SCL_read)  goto again; // any SCL activity after STOP
        }
    }
    return true;

error:
    _failed=true;
    return false;
}


