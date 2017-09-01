
#pragma once

#include <stdbool.h>
#include <stm32f4xx.h>
#include <hal.h>
#include "systick.h"
#include "Scheduler.h"
#include "timer.h"

class Soft_I2C {
public:

    typedef enum STATE {
        START,
        // address 
        A_0L,
        A_0H,
        A_1L,
        A_1H,
        A_2L,
        A_2H,
        A_3L,
        A_3H,
        A_4L,
        A_4H,
        A_5L,
        A_5H,
        A_6L,
        A_6H,
        A_7L,
        A_7H,
        A_AL,
        A_AH,
        // byte write
        W_0L,
        W_0H,
        W_1L,
        W_1H,
        W_2L,
        W_2H,
        W_3L,
        W_3H,
        W_4L,
        W_4H,
        W_5L,
        W_5H,
        W_6L,
        W_6H,
        W_7L,
        W_7H,
        W_AL,
        W_AH,
        
        STOP,
        RESTART,
        // byte read
        R_0L,
        R_0H,
        R_1L,
        R_1H,
        R_2L,
        R_2H,
        R_3L,
        R_3H,
        R_4L,
        R_4H,
        R_5L,
        R_5H,
        R_6L,
        R_6H,
        R_7L,
        R_7H,
        R_AL,
        R_AH,
    } State;

    Soft_I2C( const gpio_dev *scl_dev, uint8_t scl_bit, const gpio_dev *sda_dev, uint8_t sda_bit);
    Soft_I2C();

    void init();

    uint8_t writeBuffer( uint8_t addr_, uint8_t reg_, uint8_t len_, const uint8_t *data);
    uint8_t read( uint8_t addr_, uint8_t reg, uint8_t len, uint8_t* buf);
    uint8_t transfer(uint8_t  addr, uint8_t  send_len, const uint8_t *send, uint8_t len, uint8_t *buf);

    uint16_t getErrorCounter(void) { return i2cErrorCount; }

    bool bus_reset(void);
    
    void set_low_speed(bool s) {  }


private:
    static Soft_I2C *inst; // this for statics

    static void _tick(TIM_TypeDef *tim); // ISR
    void tick(); // 2nd ISR

    const gpio_dev *_scl_dev;
    uint8_t         _scl_bit;
    const gpio_dev *_sda_dev;
    uint8_t         _sda_bit;

    volatile GPIO_TypeDef *scl_port; // for quick direct access to hardware
    uint16_t               scl_pin;
    volatile GPIO_TypeDef *sda_port;
    uint16_t               sda_pin;

    bool _failed;
    bool done;
    uint8_t result;

    uint16_t i2cErrorCount = 0;

    bool _Start(void);
    bool  _Stop(void);
    bool  _Ack(void);
    bool  _NoAck(void);
    bool _WaitAck(void);
    bool  _SendByte(uint8_t bt);
    bool _ReceiveByte(uint8_t *bp);

    volatile State state;
    State next_state;
    bool f_sda; // what line we touch
    bool wait_scl; // flag - we wait for SCL stretching
    bool f_read; // flag of mode
    

    uint8_t data; //data byte to output / from input
    
    const uint8_t *send;
    uint16_t send_len;
    uint8_t *recv;
    uint16_t recv_len;
    uint8_t _addr;
};

