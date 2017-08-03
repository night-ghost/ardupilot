/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * I2CDriver.cpp --- AP_HAL_REVOMINI I2C driver.
 *
 * Copyright (C) 2013, Virtualrobotix.com Roberto Navoni , Emile 
 * All Rights Reserved.R Written by Roberto Navoni  <info@virtualrobotix.com>, 11 January 2013
 */

#include <AP_HAL/AP_HAL.h>
#include <AP_Param_Helper/AP_Param_Helper.h>

#include "I2CDevice.h"
#include <i2c.h>
#include "i2c_soft.h"

using namespace REVOMINI;

extern const AP_HAL::HAL& hal;

REVOMINI::Semaphore REVOI2CDevice::_semaphores[3]; // 2 HW and 1 SW
bool REVOI2CDevice::lateInitDone=false;


REVOI2CDevice * REVOI2CDevice::devices[MAX_I2C_DEVICES]; // links to all created devices
uint8_t REVOI2CDevice::dev_count; // number of devices

#ifdef I2C_DEBUG
uint8_t REVOI2CDevice::last_addr, REVOI2CDevice::last_op, REVOI2CDevice::last_send_len, REVOI2CDevice::last_recv_len, REVOI2CDevice::busy, REVOI2CDevice::last_status;
#endif


void REVOI2CDevice::lateInit(){
    lateInitDone=true;
}


REVOI2CDevice::REVOI2CDevice(uint8_t bus, uint8_t address)
        : _bus(bus)
        , _address(address)
        , _retries(1)
        , _lockup_count(0)
        , _initialized(false)
        , _dev(NULL)
        , _slow(false)
        , _failed(false)
    {



        // store link to created devices
        if(dev_count<MAX_I2C_DEVICES){
            devices[dev_count++] = this; // links to all created devices
        }                    
}

REVOI2CDevice::~REVOI2CDevice() { 
    for(int i=0;i<dev_count;i++){
        if(devices[i] == this){
            devices[i] = NULL;
        }
    }
    

}


void REVOI2CDevice::init(){
    if(!lateInitDone) {
        ((HAL_REVOMINI&) hal).lateInit();
    }

    if(_failed) return;

    if(_initialized) return;
    
    const i2c_dev *dev=NULL;

    switch(_bus) {
    case 0:         // this is always internal bus
	    _offs =0;
#if defined(BOARD_I2C_BUS_SLOW) && BOARD_I2C_BUS_SLOW==0
            _slow=true;
#endif

#if defined(BOARD_SOFT_I2C) || defined(BOARD_SOFT_I2C1)
            s_i2c = Soft_I2C( 
                _I2C1->gpio_port, _I2C1->scl_pin,
                _I2C1->gpio_port, _I2C1->sda_pin
            );
#else
	    dev = _I2C1;
#endif
	    break;


    case 1:     // flexi port - I2C2
#if !defined(BOARD_HAS_UART3) // in this case I2C on FlexiPort will be bus 2

	    _offs = 2;
 #if defined(BOARD_I2C_BUS_SLOW) && BOARD_I2C_BUS_SLOW==1
            _slow=true;
 #endif

 #if defined(BOARD_SOFT_I2C) || defined(BOARD_SOFT_I2C2)
            s_i2c = Soft_I2C( 
                _I2C2->gpio_port, _I2C2->scl_pin,
                _I2C2->gpio_port, _I2C2->sda_pin
            );
 #else
	    dev = _I2C2;
 #endif
	    break;
#else
            return; // not initialized so always returns false

#endif

#if defined( BOARD_SOFT_SCL) && defined(BOARD_SOFT_SDA)
    case 2:         // this bus can use only soft I2C driver
#if defined(BOARD_I2C_BUS_SLOW) && BOARD_I2C_BUS_SLOW==2
            _slow=true;
#endif

#ifdef BOARD_I2C_FLEXI
            if(hal_param_helper->_flexi_i2c){ // move external I2C to flexi port
                s_i2c = Soft_I2C( 
                    _I2C2->gpio_port, _I2C2->scl_pin,
                    _I2C2->gpio_port, _I2C2->sda_pin
                );
            } else 
#endif
            { //                         external I2C on Input port
                s_i2c = Soft_I2C( 
                    PIN_MAP[BOARD_SOFT_SCL].gpio_device,     PIN_MAP[BOARD_SOFT_SCL].gpio_bit,
                    PIN_MAP[BOARD_SOFT_SDA].gpio_device,     PIN_MAP[BOARD_SOFT_SDA].gpio_bit
                );
            }        
            break;
#endif

    default:
            volatile int xx=_bus; // for debug
            return;
    }
    _dev = dev; // remember

    
    if(_dev) {
//      i2c_init(_dev, _offs, I2C_400KHz_SPEED);
        i2c_init(_dev, _offs, _slow?I2C_100KHz_SPEED:I2C_250KHz_SPEED);
    }else {
        s_i2c.init( );

        if(_slow) {
            s_i2c.set_low_speed(true);
        }

    }
    _initialized=true;
}




bool REVOI2CDevice::transfer(const uint8_t *send, uint32_t send_len, uint8_t *recv, uint32_t recv_len)
{

    uint16_t retries=_retries;
    
again:

#ifdef I2C_DEBUG
    uint8_t was_addr=last_addr, was_send_len=last_send_len, was_recv_len=last_recv_len, was_busy=busy, was_status=last_status;

    if(busy){
        return false; // bus is busy, semaphores fails
    }

    last_addr=_address; last_send_len=send_len; last_recv_len=recv_len; busy=1;
#endif

	uint8_t numbytes=0;
	uint32_t ret=0;

	if(!_initialized) {
	    init();
            if(!_initialized) return false;
	}


        if(!_dev){ // no hardware so use soft I2C
            
            if(recv_len) memset(recv, 0, recv_len); // for DEBUG
            
            if(recv_len==0){ // only write
                numbytes = send_len-1;
                //                 uint8_t addr, uint8_t reg, uint8_t len, uint8_t * data)
                ret=s_i2c.writeBuffer( _address, *send, send_len-1, &send[1] );
            
            }else if(send_len==1){ // only read - send byte is address
                //            uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf
                ret=s_i2c.read(_address, *send, recv_len, recv);                
            } else {
                ret=s_i2c.transfer(_address, send_len, send, recv_len, recv);
            }

#ifdef I2C_DEBUG
            busy=false;
#endif
            
            if(ret == I2C_NO_DEVICE) 
                return false;

            if(ret == I2C_OK) 
                return true;
            
            if(!s_i2c.bus_reset()) return false;    

            if(retries--) goto again;

            return false;
        }


        
	if(recv_len==0) { // only write
#ifdef I2C_DEBUG
    last_op=1;
#endif

	    numbytes = send_len;
	    ret = i2c_write(_dev,  _address, send, &numbytes);

	} else if(send_len==1) { // only read - send byte is address
#ifdef I2C_DEBUG
    last_op=0;
#endif
	    numbytes=recv_len;
	    ret = i2c_read(_dev,  _address, send, 1, recv, &numbytes);
	} else {
#ifdef I2C_DEBUG
    last_op=0;
#endif
	    numbytes=recv_len;
	    ret = i2c_read(_dev,  _address, send, send_len, recv, &numbytes);
	}

#ifdef I2C_DEBUG
    busy=0; last_status=ret;
#endif


    if(ret == I2C_NO_DEVICE) 
        return false;
    if(ret == I2C_OK) 
        return true;


// all other errors
    _lockup_count ++;  
    _initialized=false;
	    
    i2c_deinit(_dev); // disable I2C hardware
    if(!i2c_bus_reset(_dev)) {
        _failed = true;
        return false;        
    }
    REVOMINIScheduler::_delay_microseconds(50);


    if(retries--) goto again;

    return false;
}



bool REVOI2CDevice::read_registers_multiple(uint8_t first_reg, uint8_t *recv,
                                 uint32_t recv_len, uint8_t times){

    while(times--) {
	bool ret = read_registers(first_reg, recv, recv_len);
	if(!ret) return false;
	recv += recv_len;
    }

    return true;
}


