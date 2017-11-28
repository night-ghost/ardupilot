
#ifndef __AP_HAL_REVOMINI_NAMESPACE_H__
#define __AP_HAL_REVOMINI_NAMESPACE_H__

#include <hal_types.h>

/* While not strictly required, names inside the REVOMINI namespace are prefixed
 * with REVOMINI for clarity. (Some of our users aren't familiar with all of the
 * C++ namespace rules.)
 */

namespace REVOMINI {
    class REVOMINIUARTDriver;
    class USBDriver;
    class SPIDeviceManager;
    class SPIDevice;
    class REVOMINIAnalogSource;
    class REVOMINIAnalogIn;
    class REVOMINIStorage;
    class REVOMINIGPIO;
    class REVOMINIDigitalSource;
    class REVOMINIRCInput;
    class REVOMINIRCOutput;
    class Semaphore;
    class REVOMINIScheduler;
    class REVOMINIUtil;
    class REVOI2CDevice;
    class I2CDeviceManager;
    class _parser;
    class PPM_parser;
    class DSM_parser;
    class SBUS_parser;
    class NRF_parser;
    class SerialDriver;
    class UART_OSD;
    class UART_PPM;
    class MassStorage;
}



#endif // __AP_HAL_REVOMINI_NAMESPACE_H__

