#include <AP_HAL/AP_HAL.h>
#include "i2c_soft.h"

// Software I2C driver
// Can be configured to use any suitable pins

using namespace REVOMINI;

//static uint16_t i2cErrorCount = 0;
extern const AP_HAL::HAL& hal;

#define SCL_H_NW      {scl_port->BSRRL = scl_pin; }
#define SCL_H         {scl_port->BSRRL = scl_pin; if(!wait_scl()) /* wait for extended clock done failed */ goto abort ; _delay();}
#define SCL_L_NW      {scl_port->BSRRH = scl_pin; }
#define SCL_L         {scl_port->BSRRH = scl_pin; _delay();}

#define SDA_H         {sda_port->BSRRL = sda_pin; _delay();}
#define SDA_L_NW      {sda_port->BSRRH = sda_pin; }
#define SDA_L         {sda_port->BSRRH = sda_pin; _delay();}

#define SCL_read      ((scl_port->IDR & scl_pin)!=0)
#define SDA_read      ((sda_port->IDR & sda_pin)!=0)

#define I2C_yield(x)     hal_yield(x)

static void delay_10us(){
    hal_delay_microseconds(10);
}

void Soft_I2C::_delay(void) {   hal_delay_us_ny(_dly_time); } // delay at each line change so speed is near 250kHz

bool Soft_I2C::_Start(void)
{
    SDA_H;            // just in case
    SCL_H_NW;
    _delay(); // time to discharge line
    
    if (!SCL_read)       return false; // bus busy
    if (!SDA_read)       return false; // bus busy
    SDA_L;
    if (SDA_read)        return false; // something wrong
    SCL_L;
    return true;
}

bool  Soft_I2C::_Stop(void)
{
    SCL_L_NW;           // just in case
    SDA_L_NW;
    SCL_H;
    SDA_H;
    return true;

abort: return false;
}

bool  Soft_I2C::_Ack(void)
{
    SCL_L_NW;
    SDA_L;
    SCL_H;
    SCL_L;
    return true;

abort: return false;
}

bool  Soft_I2C::_NoAck(void)
{
    SCL_L_NW;
    SDA_H;
    SCL_H;
    SCL_L;
    return true;

abort: return false;
}

bool Soft_I2C::_WaitAck(void)
{
    bool ret;
    
    SCL_L_NW;
    SDA_H;
    SCL_H;
    ret = SDA_read?false:true;
    SCL_L;
    return ret;

abort: return false;
}

bool Soft_I2C::_SendByte(uint8_t bt)
{
    for(uint8_t i = 8;i--;bt <<= 1) {
        SCL_L;
        if (bt & 0x80) { SDA_H; }
        else           { SDA_L; }
        SCL_H;
    }
    SCL_L;
    return true;

abort: return false;
}

bool Soft_I2C::_ReceiveByte(uint8_t *bp)
{
    uint8_t bt = 0;

    SDA_H;
    for(uint8_t i = 8;i--;) {
        bt <<= 1;
        SCL_L;
        SCL_H;
        if (SDA_read) bt |= 0x01;
    }
    SCL_L;
    *bp=bt;
    return true;

abort: return false;
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
    
    _dly_time=1;  // 1uS per edge = 250kHz
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
}

uint32_t  Soft_I2C::writeBuffer( uint8_t addr, uint8_t reg, uint8_t len, const uint8_t *data)
{

    int i;
    if (!_Start()) {
        i2cErrorCount++;
        return I2C_ERROR;
    }
    bool f;
    
    f= _SendByte(addr << 1 | I2C_Direction_Transmitter);
    if (!f || !_WaitAck()) {
        _Stop();
        return I2C_NO_DEVICE;
    }
    f = f && _SendByte(reg);
    f = f && _WaitAck();
    for (i = 0; i < len; i++) {
        f = f && _SendByte(data[i]);
        if (!f || !_WaitAck()) {
            _Stop();
            i2cErrorCount++;
            return I2C_ERROR;
        }
    }
    _Stop();
    if(f)
        return I2C_OK;
    else
        return I2C_ERROR;
}

uint32_t Soft_I2C::write( uint8_t addr, uint8_t reg, uint8_t data)
{
    if (!_Start()) {
        i2cErrorCount++;
        return I2C_ERROR;
    }
    bool f = _SendByte(addr << 1 | I2C_Direction_Transmitter);
    if (!f || !_WaitAck()) {
        _Stop();
        return I2C_NO_DEVICE;
    }
    f = f && _SendByte(reg);
    f = f && _WaitAck();
    f = f && _SendByte(data);
    f = f && _WaitAck();
    _Stop();
    if(f)
        return I2C_OK;
    else
        return I2C_ERROR;
}

uint32_t Soft_I2C::read( uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
    if (!_Start()) {
        i2cErrorCount++;
        return I2C_ERROR;
    }
    bool f = _SendByte(addr << 1 | I2C_Direction_Transmitter);
    if (!_WaitAck()) {
        _Stop();
        return I2C_NO_DEVICE;
    }
    f = f && _SendByte(reg);
    f = f && _WaitAck();
    f = f && _Start();
    f = f && _SendByte(addr << 1 | I2C_Direction_Receiver);
    f = f && _WaitAck();
    uint8_t cnt=0;
    while (len && f) {
        f = f && _ReceiveByte(&buf[cnt++]);
        if (len == 1) f = f && _NoAck();
        else          f = f && _Ack();
        I2C_yield(0);
        len--;
    }
    _Stop();

    if(f)
        return I2C_OK;
    else
        return I2C_ERROR;
}


uint32_t Soft_I2C::transfer(uint8_t  addr, uint8_t  send_len, const uint8_t *send, uint8_t len, uint8_t *buf){
    if (!_Start()) {
        i2cErrorCount++;
        return I2C_ERROR;
    }
    bool f = _SendByte(addr << 1 | I2C_Direction_Transmitter);
    if (!f || !_WaitAck()) {
        _Stop();
        return I2C_NO_DEVICE;
    }
    uint8_t cnt=0;
    while(send_len++ && f){
        f = f && _SendByte(send[cnt++]);
        if (!f || !_WaitAck()) {
            _Stop();
            i2cErrorCount++;
            return I2C_ERROR;
        }
        I2C_yield(0);
    }

    f = f && _Start();
    f = f && _SendByte(addr << 1 | I2C_Direction_Receiver);
    f = f && _WaitAck();
    cnt=0;
    while (len && f) {
        f = f && _ReceiveByte(&buf[cnt++]);
        if(len == 1) f = f && _NoAck();
        else         f = f && _Ack();
        len--;
        I2C_yield(0);
    }
    _Stop();

    if(f)
        return I2C_OK;
    else
        return I2C_ERROR;

}

// wait for bus release
bool Soft_I2C::wait_scl(){
    if(_failed) return false;
    
    uint32_t rtime = stopwatch_getticks();
    uint32_t dt    = rtime + us_ticks * 1000; // 1000uS

    while (stopwatch_getticks()  < dt) {
        if (SCL_read)  return true; // line released
        I2C_yield(0); // пока ожидаем - пусть другие работают
    }
    
    return false;
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
            SCL_H_NW; // may be another thread causes LOW
            REVOMINIScheduler::yield(10); // пока ожидаем - пусть другие работают

            if(systick_uptime()-t > MAX_I2C_TIME) goto error;
        }
            
        delay_10us();   // 50kHz

        /* Pull low */
        SCL_L;
        delay_10us();

        /* Release high again */
        SCL_H_NW;
        delay_10us();
        SDA_H;
    }

    /* Generate start then stop condition */
    SDA_L;
    delay_10us();
    SCL_L;
    delay_10us();
    SCL_H_NW;
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


