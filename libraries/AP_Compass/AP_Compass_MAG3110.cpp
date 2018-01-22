/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <utility>

#include <AP_HAL/AP_HAL.h>
#include <AP_Math/AP_Math.h>
#include <stdio.h>

#include "AP_Compass_MAG3110.h"

extern const AP_HAL::HAL &hal;



// Registers
#define MAG3110_MAG_REG_STATUS       0x00
#define MAG3110_MAG_REG_HXL          0x01
#define MAG3110_MAG_REG_HXH          0x02
#define MAG3110_MAG_REG_HYL          0x03
#define MAG3110_MAG_REG_HYH          0x04
#define MAG3110_MAG_REG_HZL          0x05
#define MAG3110_MAG_REG_HZH          0x06
#define MAG3110_MAG_REG_WHO_AM_I     0x07
#define MAG3110_MAG_REG_SYSMODE      0x08
#define MAG3110_MAG_REG_CTRL_REG1    0x10
#define MAG3110_MAG_REG_CTRL_REG2    0x11

#define BIT_STATUS_REG_DATA_READY    (1 << 3)




AP_Compass_MAG3110::AP_Compass_MAG3110(Compass &compass, AP_HAL::OwnPtr<AP_HAL::Device> dev)
    : AP_Compass_Backend(compass)
    , _dev(std::move(dev))
{
}

AP_Compass_Backend *AP_Compass_MAG3110::probe(Compass &compass,
                                              AP_HAL::OwnPtr<AP_HAL::Device> dev,
                                              enum Rotation rotation)
{
    if (!dev) {
        return nullptr;
    }
    AP_Compass_MAG3110 *sensor = new AP_Compass_MAG3110(compass, std::move(dev));
    if (!sensor || !sensor->init(rotation)) {
        delete sensor;
        return nullptr;
    }

    return sensor;
}


bool AP_Compass_MAG3110::init(enum Rotation rotation)
{

    bool success = _hardware_init();

    if (!success) {
        return false;
    }

    _initialised = true;

    /* register the compass instance in the frontend */
    _compass_instance = register_compass();

    set_rotation(_compass_instance, rotation);

    _dev->set_device_type(DEVTYPE_MAG3110);
    set_dev_id(_compass_instance, _dev->get_bus_id());

    // read at 75Hz
    _dev->register_periodic_callback(13333, FUNCTOR_BIND_MEMBER(&AP_Compass_MAG3110::_update, void)); 

    return true;
}


bool AP_Compass_MAG3110::_hardware_init()
{

    AP_HAL::Semaphore *bus_sem = _dev->get_semaphore();
    if (!bus_sem->take(HAL_SEMAPHORE_BLOCK_FOREVER)) {
        AP_HAL::panic("MAG3110: Unable to get semaphore");
    }

    // initially run the bus at low speed
    _dev->set_speed(AP_HAL::Device::SPEED_LOW);

    bool ret=false;
    
    _dev->set_retries(5);
    
    uint8_t sig = 0;
    bool ack = _dev->read_registers(MAG3110_MAG_REG_WHO_AM_I, &sig, 1);    
    if (!ack || sig != 0xC4) goto exit;

    ack = _dev->write_register(MAG3110_MAG_REG_CTRL_REG1, 0x01); //  active mode 80 Hz ODR with OSR = 1
    if (!ack) goto exit;

    hal.scheduler->delay(20);
    
    ack = _dev->write_register(MAG3110_MAG_REG_CTRL_REG2, 0xA0); // AUTO_MRST_EN + RAW
    if (!ack) goto exit;

//    hal.scheduler->delay(10);

    ret = true;

    _dev->set_retries(3);
    
    printf("MAG3110 found on bus 0x%x\n", (uint16_t)_dev->get_bus_id());

exit:
    _dev->set_speed(AP_HAL::Device::SPEED_HIGH);
    bus_sem->give();
    return ret;
}


// Read Sensor data
bool AP_Compass_MAG3110::_read_sample()
{
    {
        uint8_t status;
        bool ack = _dev->read_registers(MAG3110_MAG_REG_STATUS, &status, 1);
    
        if (!ack || (status & BIT_STATUS_REG_DATA_READY) == 0) {
            return false;
        }
    }

    uint8_t buf[6];
    bool ack = _dev->read_registers(MAG3110_MAG_REG_HXL, buf, 6);
    if (!ack) {
        return false;
    }

    _mag_x = (int16_t)(buf[0] << 8 | buf[1]);
    _mag_y = (int16_t)(buf[2] << 8 | buf[3]);
    _mag_z = (int16_t)(buf[4] << 8 | buf[5]);

    return true;
}


#define MAG_SCALE 1.0/10000  // 1 Tesla full scale of +-10000

void AP_Compass_MAG3110::_update()
{
    if (!_read_sample()) {
        return;
    }

    Vector3f raw_field = Vector3f(_mag_x, _mag_y, _mag_z) * MAG_SCALE;


    bool ret=true;
    
    float len = raw_field.length();
    if(is_zero(compass_len)) {
        compass_len=len;
    } else {
#define FILTER_KOEF 0.1

        float d = abs(compass_len-len)/(compass_len+len);
        if(d*100 > 25) { // difference more than 50% from mean value
            printf("\ncompass len error: mean %f got %f\n", compass_len, len );
            ret= false;
            float k = FILTER_KOEF / (d*10); // 2.5 and more, so one bad sample never change mean more than 4%
            compass_len = compass_len * (1-k) + len*k; // complimentary filter 1/k on bad samples
        } else {
            compass_len = compass_len * (1-FILTER_KOEF) + len*FILTER_KOEF; // complimentary filter 1/10 on good samples
        }
    }
    
    if(ret) {

        // rotate raw_field from sensor frame to body frame
        rotate_field(raw_field, _compass_instance);

        // publish raw_field (uncorrected point sample) for calibration use
        publish_raw_field(raw_field, _compass_instance);

        // correct raw_field for known errors
        correct_field(raw_field, _compass_instance);

        if (_sem->take(HAL_SEMAPHORE_BLOCK_FOREVER)) {
            _mag_x_accum += raw_field.x;
            _mag_y_accum += raw_field.y;
            _mag_z_accum += raw_field.z;
            _accum_count++;
            if (_accum_count == 10) {
                _mag_x_accum /= 2;
                _mag_y_accum /= 2;
                _mag_z_accum /= 2;
                _accum_count /= 2;
            }
            _sem->give();
        }
    }
}


// Read Sensor data
void AP_Compass_MAG3110::read()
{
    if (!_initialised) {
        return;
    }

    if (!_sem->take_nonblocking()) {
        return;
    }
    
    if (_accum_count == 0) {
        /* We're not ready to publish*/
        _sem->give();
        return;
    }

    Vector3f field(_mag_x_accum, _mag_y_accum, _mag_z_accum);
    field /= _accum_count;

    _accum_count = 0;
    _mag_x_accum = _mag_y_accum = _mag_z_accum = 0;

    _sem->give();    

    publish_filtered_field(field, _compass_instance);
}

/*
bool AP_Compass_MAG3110::_mag_set_samplerate(uint16_t frequency)
{
    uint8_t setbits = 0;
    uint8_t clearbits = REG5_RATE_BITS_M;

    if (frequency == 0) {
        frequency = 100;
    }

    if (frequency <= 25) {
        setbits |= REG5_RATE_25HZ_M;
        _mag_samplerate = 25;
    } else if (frequency <= 50) {
        setbits |= REG5_RATE_50HZ_M;
        _mag_samplerate = 50;
    } else if (frequency <= 100) {
        setbits |= REG5_RATE_100HZ_M;
        _mag_samplerate = 100;
    } else {
        return false;
    }

    _register_modify(ADDR_CTRL_REG5, clearbits, setbits);

    return true;
}
*/
