
#ifndef __AP_HAL_REVOMINI_UARTDRIVER_H__
#define __AP_HAL_REVOMINI_UARTDRIVER_H__

#include <AP_HAL_REVOMINI/AP_HAL_REVOMINI.h>

#include <usart.h>

#include <gpio_hal.h>
#include <hal.h>
#include "Scheduler.h"

#define DEFAULT_TX_TIMEOUT 10000 // in uS - 10ms

class REVOMINI::REVOMINIUARTDriver : public AP_HAL::UARTDriver  {
public:
    REVOMINIUARTDriver(const struct usart_dev *usart);

    void begin(uint32_t b);
    inline void begin(uint32_t b, uint16_t rxS, uint16_t txS) {   begin(b); }
    inline void end() {  usart_disable(_usart_device); _initialized=false; }
    void flush();
    inline bool is_initialized(){ return _initialized; }
  
    inline void set_blocking_writes(bool blocking) {  _blocking = blocking; }

    inline bool tx_pending() {   return ( usart_txfifo_nbytes(_usart_device) > 0); }

    inline void setCallback(Handler cb) { usart_set_callback(_usart_device, cb); }

    uint32_t available() override;
    uint32_t inline  txspace() override {    return usart_txfifo_freebytes(_usart_device); }
    int16_t read() override;

    size_t write(uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);

    inline void disable(){ _usart_device = NULL; } // pins used for another needs

private:
    const struct usart_dev *_usart_device;
    bool _initialized;
    bool _blocking;
};

#endif // __AP_HAL_EMPTY_UARTDRIVER_H__
