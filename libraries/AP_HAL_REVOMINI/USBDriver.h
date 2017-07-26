
#ifndef __AP_HAL_REVOMINI_USBDRIVER_H__
#define __AP_HAL_REVOMINI_USBDRIVER_H__

#include <AP_HAL_REVOMINI/AP_HAL_REVOMINI.h>

#include <gpio_hal.h>
//#include <usb.h> can't include here because defines there conflicts wil AP_Math
#include <usart.h>

#define DEFAULT_TX_TIMEOUT 10000

extern "C" {
 extern int usb_open(void);
 extern int usb_close(void);
 uint32_t usb_data_available(void);
 void usb_reset_rx();
}

namespace REVOMINI {

class USBDriver : public AP_HAL::UARTDriver  {
public:
    USBDriver(bool usb);

  /* REVOMINI implementations of UARTDriver virtual methods */
    void begin(uint32_t b);
    void begin(uint32_t b, uint16_t rxS, uint16_t txS) {    begin(b); }

    inline void end()   {  /* if(_usb_present)*/ usb_close(); } 
    inline bool is_initialized(){ return _initialized; }
    inline void set_blocking_writes(bool blocking) { _blocking=blocking; }
    inline bool tx_pending() {   return false; }

    void flush() { return; };

  /* implementations of Stream virtual methods */
    uint32_t available() override;
    inline uint32_t txspace() override  {   return 255; }
    int16_t read() override;

  /* implementations of Print virtual methods */
    size_t write(uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);

private:
    uint8_t _usb_present;
    bool _initialized;
    bool _blocking;
};

} // namespace

#endif // __AP_HAL_EMPTY_UARTDRIVER_H__
