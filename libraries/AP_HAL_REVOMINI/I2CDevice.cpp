/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * I2CDriver.cpp --- AP_HAL_REVOMINI I2C driver.
 *
 * Copyright (C) 2013, Virtualrobotix.com Roberto Navoni , Emile 
 * All Rights Reserved.R Written by Roberto Navoni  <info@virtualrobotix.com>, 11 January 2013
 */
#pragma GCC optimize ("O2")

#include <AP_HAL/AP_HAL.h>
#include <AP_Param_Helper/AP_Param_Helper.h>

#include "I2CDevice.h"
#include <i2c.h>

using namespace REVOMINI;

extern const AP_HAL::HAL& hal;

REVOMINI::Semaphore REVOI2CDevice::_semaphores[3]; // 2 HW and 1 SW

const timer_dev * REVOI2CDevice::_timers[3] = { // one timer per bus for all devices
    TIMER9,
    TIMER10,
    TIMER4,
};

bool REVOI2CDevice::lateInitDone=false;


REVOI2CDevice * REVOI2CDevice::devices[MAX_I2C_DEVICES]; // links to all created devices
uint8_t REVOI2CDevice::dev_count; // number of devices

#ifdef I2C_DEBUG
 I2C_State REVOI2CDevice::log[I2C_LOG_SIZE] IN_CCM;
 uint8_t   REVOI2CDevice::log_ptr=0;
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
        , _slow(false)
        , _failed(false)
        , need_reset(false)
        , _dev(NULL)
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

    if(need_reset) _do_bus_reset();

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
            s_i2c.init_hw( 
                _I2C1->gpio_port, _I2C1->scl_pin,
                _I2C1->gpio_port, _I2C1->sda_pin,
                _timers[_bus]
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
            s_i2c.init_hw( 
                _I2C2->gpio_port, _I2C2->scl_pin,
                _I2C2->gpio_port, _I2C2->sda_pin,
                _timers[_bus]
            );
 #else
	    dev = _I2C2;
 #endif
	    break;
#else
            return; // not initialized so always returns false

#endif

#if defined(BOARD_SOFT_SCL) && defined(BOARD_SOFT_SDA)
    case 2:         // this bus can use only soft I2C driver
#if defined(BOARD_I2C_BUS_SLOW) && BOARD_I2C_BUS_SLOW==2
            _slow=true;
#endif

#ifdef BOARD_I2C_FLEXI
            if(hal_param_helper->_flexi_i2c){ // move external I2C to flexi port
                s_i2c.init_hw( 
                    _I2C2->gpio_port, _I2C2->scl_pin,
                    _I2C2->gpio_port, _I2C2->sda_pin,
                    _timers[_bus]
                );
            } else 
#endif
            { //                         external I2C on Input port
                s_i2c.init_hw( 
                    PIN_MAP[BOARD_SOFT_SCL].gpio_device,     PIN_MAP[BOARD_SOFT_SCL].gpio_bit,
                    PIN_MAP[BOARD_SOFT_SDA].gpio_device,     PIN_MAP[BOARD_SOFT_SDA].gpio_bit,
                    _timers[_bus]
                );
            }        
            break;
#endif

    default:
            return;
    }
    _dev = dev; // remember

    
    if(_dev) {
//        i2c_init(_dev, _offs, _slow?I2C_125KHz_SPEED:I2C_250KHz_SPEED);
//        i2c_init(_dev, _offs, _slow?I2C_75KHz_SPEED:I2C_250KHz_SPEED);
        i2c_init(_dev, _offs, _slow?I2C_250KHz_SPEED:I2C_400KHz_SPEED);
    }else {
        s_i2c.init( );

        if(_slow) {
            s_i2c.set_low_speed(true);
        }

    }
    _initialized=true;
}



void REVOI2CDevice::register_completion_callback(Handler h) { 
    if(h && _completion_cb) {// IOC from last call still not called - some error occured so bus reset needed
        _completion_cb=0;
        _do_bus_reset();
    }
    
    _completion_cb=h;

    REVOMINIScheduler::set_task_ioc(h!=0);
}
    



bool REVOI2CDevice::transfer(const uint8_t *send, uint32_t send_len, uint8_t *recv, uint32_t recv_len)
{

    uint16_t retries=_retries;
    
again:


    uint32_t ret=0;
    uint8_t last_op=0;

    if(!_initialized) {
        init();
        if(!_initialized) return false;
    }


    if(!_dev){ // no hardware so use soft I2C
            
        if(recv_len) memset(recv, 0, recv_len); // for DEBUG
            
        if(recv_len==0){ // only write
            ret=s_i2c.writeBuffer( _address, send_len, send );            
        }else if(send_len==1){ // only read - send byte is address
            ret=s_i2c.read(_address, *send, recv_len, recv);                
        } else {
            ret=s_i2c.transfer(_address, send_len, send, recv_len, recv);
        }
            
        if(ret == I2C_NO_DEVICE) 
            return false;

        if(ret == I2C_OK) 
            return true;

        if((_retries-retries) > 0) { // don't reset and count for fixed at 2nd try errors
            _lockup_count ++;          
            last_error = ret;  
              
            if(!s_i2c.bus_reset()) return false;    
        }

        if(retries--) goto again;

        return false;
    } // software I2C

// Hardware

    uint32_t t = hal_micros();
    while(_dev->state->ioc){ //       wait for previous transfer finished
        if(hal_micros() - t > 1000) return false;
        hal_yield(0);
    }
    
    _dev->state->ioc = _completion_cb; // we got bus so now set handler
        
    if(recv_len==0) { // only write
        last_op=1;
        ret = i2c_write(_dev, _address, send, send_len);
    } else {
        last_op=0;
        ret = i2c_read(_dev,  _address, send, send_len, recv, recv_len);
    }


#ifdef I2C_DEBUG
     I2C_State &sp = log[log_ptr]; // remember last operation
     
     sp.start    = i2c_get_operation_time(&sp.op_sr1);
     sp.time     = REVOMINIScheduler::_micros();
     sp.bus      =_bus;
     sp.addr     =_address;
     sp.send_len = send_len;
     sp.recv_len = recv_len;
     sp.ret      = ret;
     sp.sr1      = _dev->I2Cx->SR1;
     sp.sr2      = _dev->I2Cx->SR2;
     if(log_ptr<I2C_LOG_SIZE-1) log_ptr++;
     else                       log_ptr=0;
#endif


    if(ret == I2C_PENDING) return true; // DMA transfer with callback

    if(ret == I2C_OK) return true;

// something went wrong and completion callback never will be called, so release bus semaphore
    if(_completion_cb)  {
        _completion_cb = 0;     // to prevent 2nd bus reset
        register_completion_callback((Handler)0);
    }

    if(ret == I2C_ERR_STOP || ret == I2C_STOP_BERR || ret == I2C_STOP_BUSY) { // bus or another errors on Stop, or bus busy after Stop.
                                                                            //   Data is good but bus reset required
        need_reset = true;
        _initialized=false; // will be reinitialized at next transfer

        _dev->I2Cx->CR1 |= I2C_CR1_SWRST; // set for some time
        
        // we not count such errors as _lockup_count
    
        Revo_handler h = { .mp=FUNCTOR_BIND_MEMBER(&REVOI2CDevice::do_bus_reset, void) }; // schedule reset as io_task
        REVOMINIScheduler::_register_io_process(h.h, IO_ONCE); 
        
        return true; // data is OK
    } 
    
    if(ret != I2C_NO_DEVICE) { // for all errors except NO_DEVICE do bus reset

        if(ret == I2C_BUS_BUSY) {
            _dev->I2Cx->CR1 |= I2C_CR1_SWRST;           // set SoftReset for some time 
            hal_yield(0);
            _dev->I2Cx->CR1 &= (uint16_t)(~I2C_CR1_SWRST); // clear SoftReset flag            
        }

        if((_retries-retries) > 0 || ret==I2C_BUS_ERR){ // not reset bus or log error on 1st try, except ArbitrationLost error
            last_error = ret;   // remember
            if(last_op) last_error+=50; // to distinguish read and write errors
            _lockup_count ++;  
            _initialized=false; // will be reinitialized at next transfer
        
            _do_bus_reset();
        
            if(_failed) return false;
        }
    }

    if(retries--) goto again;

    return false;
}


void REVOI2CDevice::do_bus_reset(){ // public - with semaphores
    if(_semaphores[_bus].take(HAL_SEMAPHORE_BLOCK_FOREVER)){
        _do_bus_reset();
        _semaphores[_bus].give();
    }
}

void REVOI2CDevice::_do_bus_reset(){ // private

    _dev->I2Cx->CR1 &= (uint16_t)(~I2C_CR1_SWRST); // clear soft reset flag

    if(!need_reset) return; // already done
    
    i2c_deinit(_dev); // disable I2C hardware
    if(!i2c_bus_reset(_dev)) {
        _failed = true;         // can't do it in limited time
    }
    need_reset = false; // done
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


