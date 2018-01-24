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

static uint32_t op_time;
static uint32_t op_sr1;

inline uint32_t i2c_get_operation_time(uint16_t *psr1){
    if(psr1) *psr1 = op_sr1;
    return op_time;
}
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

    if(!REVOMINIScheduler::in_interrupt()) { // drivers that calls  register_completion_callback() from interrupt has exclusive bus
        REVOMINIScheduler::set_task_ioc(h!=0);
    }
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
#ifdef I2C_DEBUG
    {
     I2C_State &sp = log[log_ptr]; // remember last operation
     sp.st_sr1      = _dev->I2Cx->SR1;
     sp.st_sr2      = _dev->I2Cx->SR2;
     }
#endif

    if(recv_len==0) { // only write
        last_op=1;
        ret = i2c_write(_address, send, send_len);
    } else {
        last_op=0;
        ret = i2c_read( _address, send, send_len, recv, recv_len);
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

void REVOI2CDevice::_io_cb(){

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



/*
    moved from low layer to be properly integrated to multitask

*/


/* Send a buffer to the i2c port */
uint32_t REVOI2CDevice::i2c_write(uint8_t addr, const uint8_t *tx_buff, uint8_t len) {

    const uint8_t *buffer = tx_buff;

    uint32_t state = I2C_ERROR;
    uint16_t sr1;
    uint32_t t;

    uint32_t ret = wait_stop_done(true);
    if(ret!=I2C_OK) return ret;

#ifdef I2C_DMA_SEND 
    bool dma_mode = false; //  (len < DMA_BUFSIZE);

    dma_stream tx_stream = _dev->dma.stream_tx;

    if(dma_mode) {
        //  проверить, не занят ли поток DMA перед использованием
        t = hal_micros();
        while(dma_is_stream_enabled(tx_stream) || dma_is_stream_enabled(_dev->dma.stream_rx) ) {
            // wait for transfer termination
            if (hal_micros() - t > I2C_TIMEOUT) {
                dma_disable(tx_stream);  // something went wrong so let it get stopped
                dma_disable(_dev->dma.stream_rx);
                return I2C_DMA_BUSY; // DMA stream busy
            }
            hal_yield(0); 
        }
    
        const uint8_t *dma_tx;

        if(ADDRESS_IN_RAM(tx_buff)){
            dma_tx = tx_buff;
        } else {
            memmove(_dev->state->buff, tx_buff, len);
            dma_tx = _dev->state->buff;
        }
        _dev->state->len=0; // clear need to memmove            

        // init DMA beforehand    
        dma_init(tx_stream); 
        dma_clear_isr_bits(tx_stream); 

        DMA_InitTypeDef DMA_InitStructure;
        DMA_StructInit(&DMA_InitStructure);
    
        DMA_InitStructure.DMA_Channel               = _dev->dma.channel;
        DMA_InitStructure.DMA_Memory0BaseAddr       = (uint32_t)dma_tx;
        DMA_InitStructure.DMA_BufferSize            = len;
        DMA_InitStructure.DMA_PeripheralBaseAddr    = (uint32_t)(&(_dev->I2Cx->DR));
        DMA_InitStructure.DMA_PeripheralDataSize    = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize        = DMA_MemoryDataSize_Byte;
        DMA_InitStructure.DMA_PeripheralInc         = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc             = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_Mode                  = DMA_Mode_Normal;
        DMA_InitStructure.DMA_Priority              = DMA_Priority_High;
        DMA_InitStructure.DMA_FIFOMode              = DMA_FIFOMode_Disable;
        DMA_InitStructure.DMA_FIFOThreshold         = DMA_FIFOThreshold_Full;
        DMA_InitStructure.DMA_MemoryBurst           = DMA_MemoryBurst_Single;
        DMA_InitStructure.DMA_PeripheralBurst       = DMA_PeripheralBurst_Single;
        DMA_InitStructure.DMA_DIR                   = DMA_DIR_MemoryToPeripheral;
        
        dma_init_transfer(tx_stream, &DMA_InitStructure);

        dma_attach_interrupt(tx_stream, REVOMINIScheduler::get_handler(FUNCTOR_BIND_MEMBER(&REVOI2CDevice::isr, void)), DMA_CR_TCIE);

        dma_enable(tx_stream);        
    } // DMA mode

#endif
    

    state++;


    // Bus got!  enable Acknowledge for our operation
    _dev->I2Cx->CR1 |= I2C_CR1_ACK; 
    _dev->I2Cx->CR1 &= ~I2C_NACKPosition_Next; 

    // Send START condition
    _dev->I2Cx->CR1 |= I2C_CR1_START;

//    _dev->I2Cx->CR2 |= I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN | ;    // Enable interrupts


    // Test on EV5 and clear it (cleared by reading SR1 then writing to DR)
    t = hal_micros();

    while( ((sr1=_dev->I2Cx->SR1) & I2C_FLAG_SB & FLAG_MASK) == 0) { // wait for start bit generated
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6
        
        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        
        
        
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout - time of timeout much large than we use so it is useless, but...
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }
	if (hal_micros() - t > I2C_TIMEOUT)
	    return state; // 3 - failed to start

	hal_yield(0); 
    }


    state++;
    
    // Send address for write
    i2c_send_address(_dev, addr<<1, I2C_Direction_Transmitter );

    _dev->I2Cx->CR1 &= (uint16_t)(~I2C_CR1_STOP);    /* clear STOP condition - just to touch CR1*/

    t = hal_micros();
    // Test on EV6 and clear it
    while ( ((sr1 = _dev->I2Cx->SR1) & I2C_FLAG_ADDR & FLAG_MASK) == 0)  {
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(sr1 & I2C_FLAG_AF & FLAG_MASK) {
            _dev->I2Cx->SR1 = ~I2C_SR1_AF; // reset it
            
            state = I2C_NO_DEVICE; // Acknolege Failed
            goto err_exit;
        }

        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }

	if (hal_micros() - t > I2C_TIMEOUT)
	    goto err_exit; // 4 failed to send address
    }

#ifdef I2C_DMA_SEND 

    if(dma_mode) {
        // let setup DMA mode now
        _dev->I2Cx->CR2 |= I2C_CR2_DMAEN;    // Enable I2C RX request - all reads will be in DMA mode

        /* Clear ADDR bit by reading SR1 then SR2 register (SR1 has already been read) */
        (void) _dev->I2Cx->SR2;

        if(_completion_cb) return I2C_PENDING;


        uint32_t timeout = i2c_bit_time * 9 * len * 3; // time to transfer all data *3

        if(!REVOMINIScheduler::in_interrupt()) { // if function called from task - store it and pause
            _task = REVOMINIScheduler::get_current_task();
            REVOMINIScheduler::task_pause(timeout);
        } else {
            _task=0;
        }

        // need to wait until DMA transfer complete */
        t = hal_micros();
        while ( dma_is_stream_enabled(tx_stream)) {
            if (hal_micros() - t > timeout) {
                _dev->I2Cx->CR2 &= ~(I2C_CR2_LAST | I2C_CR2_DMAEN); // Disable I2C DMA request 
                dma_disable(tx_stream);
                dma_clear_isr_bits(tx_stream); 
                dma_detach_interrupt(tx_stream);
                state = I2C_DMA_ERROR; // 100 DMA error
                goto err_exit;
            }
            hal_yield(0);
        }

        //** DMA disabled and stop generated in ISR
        return I2C_OK;
        
    } else 
#endif
    { // not DMA

        /* Clear ADDR register by reading SR1 then SR2 register (SR1 has already been read) */
        (void) _dev->I2Cx->SR2;

        state++;

        while( ((sr1=_dev->I2Cx->SR1) & I2C_FLAG_TXE & FLAG_MASK)  == 0) { // wait for TX empty
            if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

            if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                state = I2C_BUS_ERR;
                goto err_exit;
            }
            if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
                _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
                return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
            }

            if (hal_micros() - t > I2C_TIMEOUT) goto err_exit; // 5 - TXe not set on 1st byte
        }

        state++;

        _dev->I2Cx->DR = *buffer++; // 1st byte


        if (len < 2) { // only 1 byte
            /* Test on EV8 and clear it */
            t = hal_micros();
            while( ((sr1=_dev->I2Cx->SR1) & I2C_FLAG_TXE & FLAG_MASK)  == 0) { // wait for TX empty
                if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

                if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                    _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                    state = I2C_BUS_ERR;
                    goto err_exit;
                }
                if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
                    _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
                    return I2C_ERR_TIMEOUT;                         // STOP generated by hardware
                }

                if (hal_micros() - t > I2C_TIMEOUT) goto err_exit; // 6 1-st byte transmit failed
            }

            state++;

            _dev->I2Cx->CR1 |= I2C_CR1_STOP;         	/* Send STOP condition */

        } else {
            do {
                t = hal_micros();
                while( ((sr1=_dev->I2Cx->SR1) & I2C_FLAG_TXE & FLAG_MASK)  == 0) { // wait for TX empty
                    if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

                    if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                        _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                        state = I2C_BUS_ERR;
                        goto err_exit;
                    }
                    if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
                        _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
                        return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
                    }

                    if (hal_micros() - t > I2C_TIMEOUT) {
                                // byte   1 2 3 4 5...
    	                goto err_exit; // 6 7 8 9 10 byte transmit failed
    		    }		    
                }
                state++;

                if(--len == 0) { // last is sent, no more bytes
            
	            t = hal_micros();
                    while( ((sr1=_dev->I2Cx->SR1) & I2C_FLAG_TXE & FLAG_MASK)  == 0) { // wait for TX empty
                        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

                        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                            _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                            state = I2C_BUS_ERR;
                            goto err_exit;
                        }
                        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
                            _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
                            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
                        }

	                if (hal_micros() - t > I2C_TIMEOUT) goto err_exit; // 7 2nd byte transmit failed
                    }
                                
    		    _dev->I2Cx->CR1 |= I2C_CR1_STOP; /* Send STOP condition */
                } else 
    	            _dev->I2Cx->DR = *buffer++; // next byte
            } while(len);

        }
        
#ifdef I2C_DEBUG
        op_time = t;
#endif

        Handler h;
        if( (h=_completion_cb) ){    // handle io completion
            _completion_cb=0; // only once and before call because handler can set it itself
            revo_call_handler(h,(uint32_t)_dev);
        }

        return I2C_OK;
    }

err_exit:// after any error make STOP to release bus
    _dev->I2Cx->CR1 |= I2C_CR1_STOP;                    /* Send STOP condition */
#ifdef I2C_DEBUG
    op_time = t; // time of failed operation start
    op_sr1 = sr1;
#endif
    return state;
}



void REVOI2CDevice::isr(){

    if(dma_get_isr_bits(_dev->dma.stream_rx) & DMA_FLAG_TCIF) { // was receive
        dma_disable(_dev->dma.stream_rx);

        _dev->I2Cx->CR2 &= ~(I2C_CR2_LAST | I2C_CR2_DMAEN);        /* Disable I2C DMA request */

        dma_clear_isr_bits(_dev->dma.stream_rx); 

        // transfer done!
        dma_detach_interrupt(_dev->dma.stream_rx);    
        
        if(_dev->state->len){    // we need to memmove
            memmove(_dev->state->dst, _dev->state->buff, _dev->state->len);
        }
    }
    
#ifdef I2C_DMA_SEND 
    uint32_t sr1;
    uint32_t t;

    if(dma_get_isr_bits(_dev->dma.stream_tx) & DMA_FLAG_TCIF) { // was transmit
        dma_disable(_dev->dma.stream_tx);

        _dev->I2Cx->CR2 &= ~(I2C_CR2_LAST | I2C_CR2_DMAEN);        /* Disable I2C DMA request */

        dma_clear_isr_bits(_dev->dma.stream_tx); 

        // transfer done!
        dma_detach_interrupt(_dev->dma.stream_tx);    
        
// Stop condition should be programmed during EV8_2 event, when either TxE or BTF is set. (p. 837)

// we can program ITBUFEN and enable interrupt one byte later


        while( ((sr1=_dev->I2Cx->SR1) & I2C_FLAG_TXE & FLAG_MASK)  == 0) { // wait for TX empty
            if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

            if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                break;
            }
            if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
                _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
                break;
            }

            if (hal_micros() - t > I2C_TIMEOUT) break; // failed
        }
    }
#endif

    _dev->I2Cx->CR1 |= I2C_CR1_STOP;     /* Send STOP condition */

#if 0  // bad thing to wait in ISR

   // Wait to make sure that STOP control bit has been cleared - bus released
    t = hal_micros();
    while (_dev->I2Cx->CR1 & I2C_CR1_STOP ){
        if((sr1=_dev->I2Cx->SR1) & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost or bus error
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            break;
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            break;
        }

        if (hal_micros() - t > I2C_TIMEOUT) break;
    }
#endif

    Handler h;
    if( (h=_completion_cb) ){    // io completion
        
        _completion_cb=0; // only once and before call because handler can set it itself
        
        revo_call_handler(h,(uint32_t)_dev);
    }
    
    if(_task){ // resume paused task
        REVOMINIScheduler::task_resume(_task);
        _task=NULL;
    }    
}

uint32_t REVOI2CDevice::wait_stop_done(bool is_write){
    uint32_t sr1;
    uint32_t t;

    uint8_t ret;
    uint8_t i;
    for(i=0; i<10; i++){

        ret=I2C_OK;
       // Wait to make sure that STOP control bit has been cleared - bus released
        t = hal_micros();
        while (_dev->I2Cx->CR1 & I2C_CR1_STOP ){
            if((sr1=_dev->I2Cx->SR1) & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

            if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost or bus error
                _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                ret= I2C_STOP_BERR; // bus error on STOP
                break;
            }
            if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
                _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
                ret= I2C_ERR_TIMEOUT;                             // STOP generated by hardware
                break;
            }

            if (hal_micros() - t > I2C_SMALL_TIMEOUT) {
                ret=I2C_ERR_STOP;
                break;
            }
        }

        /* wait while the bus is busy */
        t = hal_micros();
        while ((_dev->I2Cx->SR2 & (I2C_FLAG_BUSY>>16) & FLAG_MASK) != 0) {
            if (hal_micros() - t > I2C_SMALL_TIMEOUT) {
                ret=2; // bus busy
                break;
            }
	
            hal_yield(0); 
        }


        if(ret==I2C_OK) return ret;
        
        if(i>0){
            _dev->I2Cx->CR1 |= I2C_CR1_SWRST;           // set SoftReset for some time 
            hal_yield(0);
            _dev->I2Cx->CR1 &= (uint16_t)(~I2C_CR1_SWRST); // clear SoftReset flag                    
        }

        if(i>1){
            last_error = ret;   // remember
            if(is_write) last_error+=50;

            _lockup_count ++;  
            _initialized=false; // will be reinitialized in init()
        
            need_reset=true;
            init();
            
            if(!_initialized) return ret;
        
        }

    }

    return I2C_OK;
}


uint32_t REVOI2CDevice::i2c_read(uint8_t addr, const uint8_t *tx_buff, uint8_t txlen, uint8_t *rx_buff, uint8_t rxlen)
{
    uint8_t *dma_rx;
    
    uint32_t state=I2C_ERROR; 
    bool dma_mode = (rxlen < DMA_BUFSIZE);

    uint16_t sr1;
    uint32_t t;
    
    // in case of DMA transfer
    dma_stream rx_stream = _dev->dma.stream_rx;

    uint32_t ret = wait_stop_done(false); // wait for bus release from previous transfer and force it if needed
    if(ret!=I2C_OK) return ret;

    if(dma_mode) {
        //  проверить, не занят ли поток DMA перед использованием
        t = hal_micros();
        while(dma_is_stream_enabled(rx_stream)/* || dma_is_stream_enabled(_dev->dma.stream_tx)*/ ) {
            // wait for transfer termination
            if (hal_micros() - t > I2C_TIMEOUT) {
                dma_disable(rx_stream);  // something went wrong so let it get stopped
                dma_disable(_dev->dma.stream_tx);
                break;                    // DMA stream grabbed
            }
            hal_yield(0); 
        }

        if(ADDRESS_IN_RAM(rx_buff)){
            dma_rx = rx_buff;
            _dev->state->len=0; // clear need to memmove            
        } else {
            dma_rx = _dev->state->buff;
            _dev->state->len=rxlen; // need to memmove
            _dev->state->dst=rx_buff;
        }

        // init DMA beforehand    
        dma_init(rx_stream); 
        dma_clear_isr_bits(rx_stream); 

        DMA_InitTypeDef DMA_InitStructure;
        DMA_StructInit(&DMA_InitStructure);
    
        DMA_InitStructure.DMA_Channel               = _dev->dma.channel;
        DMA_InitStructure.DMA_Memory0BaseAddr       = (uint32_t)dma_rx;
        DMA_InitStructure.DMA_BufferSize            = rxlen;
        DMA_InitStructure.DMA_PeripheralBaseAddr    = (uint32_t)(&(_dev->I2Cx->DR));
        DMA_InitStructure.DMA_PeripheralDataSize    = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize        = DMA_MemoryDataSize_Byte;
        DMA_InitStructure.DMA_PeripheralInc         = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc             = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_Mode                  = DMA_Mode_Normal;
        DMA_InitStructure.DMA_Priority              = DMA_Priority_High;
        DMA_InitStructure.DMA_FIFOMode              = DMA_FIFOMode_Disable;
        DMA_InitStructure.DMA_FIFOThreshold         = DMA_FIFOThreshold_Full;
        DMA_InitStructure.DMA_MemoryBurst           = DMA_MemoryBurst_Single;
        DMA_InitStructure.DMA_PeripheralBurst       = DMA_PeripheralBurst_Single;
        DMA_InitStructure.DMA_DIR                   = DMA_DIR_PeripheralToMemory;
        
        dma_init_transfer(rx_stream, &DMA_InitStructure);

        dma_attach_interrupt(rx_stream, REVOMINIScheduler::get_handler(FUNCTOR_BIND_MEMBER(&REVOI2CDevice::isr, void)), DMA_CR_TCIE); 

        dma_enable(rx_stream);        
    } // DMA mode

    state++; 

    _dev->I2Cx->CR1 &= ~I2C_NACKPosition_Next; // I2C_NACKPosition_Current
    _dev->I2Cx->CR1 |= I2C_CR1_ACK;      // Bus got!  enable Acknowledge for our operation
    
    _dev->I2Cx->CR1 |= I2C_CR1_START;    // Send START condition

    // Test on EV5 and clear it (cleared by reading SR1 then writing to DR)
    t = hal_micros();
    while( ((sr1=_dev->I2Cx->SR1) & I2C_FLAG_SB & FLAG_MASK) == 0) { // wait for start bit generated
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }
        if (hal_micros() - t > I2C_SMALL_TIMEOUT)  return state; // 3 error Master can't be selected (bus has owner)
    }

    state++; 

    // Send address for write
    i2c_send_address(_dev, addr<<1, I2C_Direction_Transmitter );
    
    _dev->I2Cx->CR1 &= (uint16_t)(~I2C_CR1_STOP);    /* clear STOP condition - just to touch CR1 */

    t = hal_micros();
    // Test on EV6 and clear it
    while ( ((sr1 = _dev->I2Cx->SR1) & I2C_FLAG_ADDR & FLAG_MASK) == 0)  {
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(sr1 & I2C_FLAG_AF & FLAG_MASK) {
            _dev->I2Cx->SR1 = ~I2C_SR1_AF; // reset it
            state = I2C_NO_DEVICE; // Acknolege Failed
            goto err_exit;

        }
        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }

        if (hal_micros() - t > I2C_TIMEOUT) {
            goto err_exit;  // 4 TX mode not acknoleged
        }
    }

    state++; 

    /* Clear ADDR flag by reading SR1 then SR2 register (SR1 has already been read) */
    (void) _dev->I2Cx->SR2;

    while( ((sr1=_dev->I2Cx->SR1) & I2C_FLAG_TXE & FLAG_MASK)  == 0) { // wait for TX empty
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }

        if (hal_micros() - t > I2C_SMALL_TIMEOUT) goto err_exit; // 5 - TXe not set on 1st byte
    }

    state++;

    while(tx_buff && txlen--) {
        _dev->I2Cx->DR = *tx_buff++; // send next byte

        // Test on EV8 and clear it
        t = hal_micros();
        while( ((sr1=_dev->I2Cx->SR1) & I2C_FLAG_TXE & FLAG_MASK) == 0) { // wait for TX empty
            if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

            if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                state = I2C_BUS_ERR;
                goto err_exit;
            }
            if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
                _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
                return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
            }

            if (hal_micros() - t > I2C_TIMEOUT)  goto err_exit; // 6 write error
        }
        
        state++; 
    }

    // Send START condition a second time
    _dev->I2Cx->CR1 |= I2C_CR1_START;

    // Test on EV5 and clear it (cleared by reading SR1 then writing to DR)
    t = hal_micros();
    while( ((sr1=_dev->I2Cx->SR1) & I2C_FLAG_SB & FLAG_MASK) == 0) { // wait for Restart generated - bit was Cleared by reading the SR1 register followed by writing the DR register
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }
        if (hal_micros() - t > I2C_TIMEOUT)   goto err_exit; // 7 restart error SMALL_TIMEOUT is too small
    }

    state++; 

    // Send device address for read
    i2c_send_address(_dev, addr<<1, I2C_Direction_Receiver );

    _dev->I2Cx->CR1 &= (uint16_t)(~I2C_CR1_STOP);    /* clear STOP condition - just to touch CR1 */

//[ wait for end of address sending
    t = hal_micros();
    while ( ((sr1 = _dev->I2Cx->SR1) & I2C_FLAG_ADDR & FLAG_MASK) == 0)  {
        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

        if(sr1 & I2C_FLAG_AF & FLAG_MASK) {
            _dev->I2Cx->SR1 = ~I2C_SR1_AF;
            goto err_exit; // 8 Acknolege Failed - not "no device"! this happens when we try to read non-existent register from chip
        }

        if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
            state = I2C_BUS_ERR;
            goto err_exit;
        }
        if(sr1 & I2C_FLAG_TIMEOUT & FLAG_MASK) { // bus timeout
            _dev->I2Cx->SR1 = (uint16_t)(~I2C_FLAG_TIMEOUT); // reset it
            return I2C_ERR_TIMEOUT;                             // STOP generated by hardware
        }
	    
        if (hal_micros() - t > I2C_TIMEOUT) {
            state+=1; // 9 send read address error
            goto err_exit;
        }
    }
//]
 
    state+=2; // 10+ read data error

    if(dma_mode) {
//------------
        // let setup DMA mode now
        if(rxlen == 1) {                 // Disable Acknowledge 
            _dev->I2Cx->CR1 &= ~I2C_CR1_ACK;
            _dev->I2Cx->CR2 &= ~I2C_CR2_LAST; // disable DMA generated NACK
        } else if(rxlen == 2) {              // Disable Acknowledge and change NACK position
            _dev->I2Cx->CR1 |= I2C_NACKPosition_Next; // move NACK to next byte
            _dev->I2Cx->CR1 &= ~I2C_CR1_ACK;
            _dev->I2Cx->CR2 &= ~I2C_CR2_LAST; // disable DMA generated NACK
        } else {                             // Enable ACK and Last DMA bit
            _dev->I2Cx->CR1 |= I2C_CR1_ACK;
            _dev->I2Cx->CR2 |= I2C_CR2_LAST;  
        }
        _dev->I2Cx->CR2 |= I2C_CR2_DMAEN;    // Enable I2C RX request - all reads will be in DMA mode


        /* Clear ADDR bit by reading SR1 then SR2 register (SR1 has already been read) */
        (void) _dev->I2Cx->SR2;

        if(_completion_cb) return I2C_PENDING;
        
        uint32_t timeout = i2c_bit_time * 9 * rxlen * 3; // time to transfer all data *3
        
        if(!REVOMINIScheduler::in_interrupt()) { // if function called from task - store it and pause
            _task = REVOMINIScheduler::get_current_task();
            REVOMINIScheduler::task_pause(timeout);
        } else {
            _task=0;
        }
        
        t = hal_micros();
        // need to wait until DMA transfer complete */
        while ( dma_is_stream_enabled(rx_stream)) {
            if (hal_micros() - t > timeout) {
                _dev->I2Cx->CR2 &= ~(I2C_CR2_LAST | I2C_CR2_DMAEN); // Disable I2C DMA request 
                dma_disable(rx_stream);
                dma_clear_isr_bits(rx_stream); 
                dma_detach_interrupt(rx_stream);
                state = I2C_DMA_ERROR; // 100 DMA error
                goto err_exit;
            }
            hal_yield(0);
        }

        //** DMA disabled and stop generated in ISR
    } else { // not DMA

//------------
        if (rxlen == 1) { // 1 byte reads - by hands

        // Disable Acknowledgement - send NACK for single byte BEFORE resetting ADDR
	    _dev->I2Cx->CR1 &= ~I2C_CR1_ACK;

	    /* Clear ADDR flag by reading SR1 then SR2 register (SR1 has already been read) */
	    (void) _dev->I2Cx->SR2;

	    _dev->I2Cx->CR1 |= I2C_CR1_STOP;         // Send STOP condition after this byte

	    /* Wait for the byte to be received */
	    t = hal_micros();
	    while ( ((sr1=_dev->I2Cx->SR1) & I2C_FLAG_RXNE & FLAG_MASK) == 0)  {
	        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

                if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                    _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                    state = I2C_BUS_ERR;
                    goto err_exit;  // 10 1st data byte read error
                }
	        if (hal_micros() - t > I2C_TIMEOUT) return state; // 10 read data error - STOP already generated
	    }

            state++;

	    /*!< Read the byte received  */
	    *rx_buff = (uint8_t)(_dev->I2Cx->DR);

        } else if (rxlen==2) { // 2 byte reads - by hands, special case

/*
For 2-byte reception:
 Wait until ADDR = 1 (SCL stretched low until the ADDR flag is cleared)
 Set ACK low, set POS high
 Clear ADDR flag
 Wait until BTF = 1 (Data 1 in DR, Data2 in shift register, SCL stretched low until a data 1 is read)
 Set STOP high
 Read data 1 and 2

*/
	    // Disable Acknowledgement - send NACK for 2 bytes BEFORE resetting ADDR
	    _dev->I2Cx->CR1 &= ~I2C_CR1_ACK; //      disable ACK
	    _dev->I2Cx->CR1 |= I2C_NACKPosition_Next; // move NACK to next byte 

	    /* Clear ADDR register by reading SR1 then SR2 register (SR1 has already been read) */
	    (void) _dev->I2Cx->SR2;

	    /* Wait for the byte to be received */
	    t = hal_micros();
	    while ( ((sr1=_dev->I2Cx->SR1) & I2C_FLAG_RXNE & FLAG_MASK) == 0)  {
	        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

                if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                    _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                    state = I2C_BUS_ERR;
                    goto err_exit;
                }

	        if (hal_micros() - t > I2C_TIMEOUT) goto err_exit; // 10 read data error
	    }

            state++;

	    *rx_buff++ = (uint8_t)(_dev->I2Cx->DR);	/*!< Read the 1st byte received  */

	    _dev->I2Cx->CR1 |= I2C_CR1_STOP;                 /* Send STOP condition */

	    /* Wait for the byte to be received */
	    t = hal_micros();
	    while ( ((sr1=_dev->I2Cx->SR1) & I2C_FLAG_RXNE & FLAG_MASK) == 0)  {
	        if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

                if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                    _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                    state = I2C_BUS_ERR;
                    goto err_exit;
                }
	        if (hal_micros() - t > I2C_TIMEOUT) return state; // 10 read data 2 error - stop already generated
	    }

            state++;
	
            *rx_buff = (uint8_t)(_dev->I2Cx->DR);    /*!< Read the 2nd byte received  */

        } else { // More than 2 Byte Master Reception procedure 
    
            /* Clear ADDR bit by reading SR1 then SR2 register (SR1 has already been read) */
            (void) _dev->I2Cx->SR2;

            do {
                /* Wait for the byte to be received */
	        t = hal_micros();
        	while ( ((sr1=_dev->I2Cx->SR1) & I2C_FLAG_RXNE & FLAG_MASK) == 0)  {
        	    if(sr1 & I2C_FLAG_BERR & FLAG_MASK) _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_BERR); // Errata 2.4.6

                    if(sr1 & I2C_FLAG_ARLO & FLAG_MASK) { // arbitration lost
                        _dev->I2Cx->SR1 = (uint16_t)(~I2C_SR1_ARLO); // reset them
                        state = I2C_BUS_ERR;
                        goto err_exit;
                    }

	            if (hal_micros() - t > I2C_TIMEOUT) {
	            //                byte 0  1  2  3  4  5  6
	                goto err_exit; // 10 11 12 13 14 15 16...
	            }
	        }
	     
	        state++; 

	        /*!< Read the byte received  */
	        *rx_buff++ = (uint8_t)(_dev->I2Cx->DR);
	        rxlen -= 1; // 1 byte done
	    
	    
	        if(rxlen == 1) { // last second byte
	            _dev->I2Cx->CR1 &= ~I2C_CR1_ACK;     // Disable Acknowledgement - send NACK for last byte 

	            _dev->I2Cx->CR1 |= I2C_CR1_STOP;     /* Send STOP condition after last byte */
	        }
            } while(rxlen);
        }      

        // all transfers finished, STOP started
    }

#ifdef I2C_DEBUG
    op_time = t;
#endif

    return I2C_OK;

err_exit:// after any error make STOP to release bus
    _dev->I2Cx->CR1 |= I2C_CR1_STOP;                 /* Send STOP condition */
#ifdef I2C_DEBUG
    op_time = t; // time of failed operation start
    op_sr1 = sr1;
#endif
    return state;

}

/*
    errata 2.4.6
Spurious Bus Error detection in Master mode
Description
In Master mode, a bus error can be detected by mistake, so the BERR flag can be wrongly
raised in the status register. This will generate a spurious Bus Error interrupt if the interrupt
is enabled. A bus error detection has no effect on the transfer in Master mode, therefore the
I2C transfer can continue normally.
Workaround
If a bus error interrupt is generated in Master mode, the BERR flag must be cleared by
software. No other action is required and the on-going transfer can be handled normally

*/
