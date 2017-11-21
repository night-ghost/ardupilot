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

/*
    copied form AP_Compass_HMC5843.cpp and altered to use new IO_Completion API
    it can be a common HMC5843 driver after this API will be in mainline


 *       AP_Compass_HMC5843.cpp - Arduino Library for HMC5843 I2C magnetometer
 *       Code by Jordi Muñoz and Jose Julio. DIYDrones.com
 *
 *       Sensor is connected to I2C port
 *       Sensor is initialized in Continuos mode (10Hz)
 *
 */
 
#pragma GCC optimize("O3")
 
#include <AP_HAL/AP_HAL.h>

#ifdef HAL_COMPASS_HMC5843_I2C_ADDR

#include <assert.h>
#include <utility>
#include <stdio.h>

#include <AP_Math/AP_Math.h>
#include <AP_HAL/AP_HAL.h>
#include <AP_HAL/utility/sparse-endian.h>

#include "AP_Compass_Revo.h"
#include <AP_InertialSensor/AP_InertialSensor.h>
#include <AP_InertialSensor/AuxiliaryBus.h>

#include <AP_HAL_REVOMINI/Scheduler.h>
#include <AP_HAL_REVOMINI/I2CDevice.h>

extern const AP_HAL::HAL& hal;

/*
 * Defaul address: 0x1E
 */

#define HMC5843_REG_CONFIG_A 0x00
// Valid sample averaging for 5883L
#define HMC5843_SAMPLE_AVERAGING_1 (0x00 << 5)
#define HMC5843_SAMPLE_AVERAGING_2 (0x01 << 5)
#define HMC5843_SAMPLE_AVERAGING_4 (0x02 << 5)
#define HMC5843_SAMPLE_AVERAGING_8 (0x03 << 5)

#define HMC5843_CONF_TEMP_ENABLE   (0x80)

// Valid data output rates for 5883L
#define HMC5843_OSR_0_75HZ (0x00 << 2)
#define HMC5843_OSR_1_5HZ  (0x01 << 2)
#define HMC5843_OSR_3HZ    (0x02 << 2)
#define HMC5843_OSR_7_5HZ  (0x03 << 2)
#define HMC5843_OSR_15HZ   (0x04 << 2)
#define HMC5843_OSR_30HZ   (0x05 << 2)
#define HMC5843_OSR_75HZ   (0x06 << 2)
// Sensor operation modes
#define HMC5843_OPMODE_NORMAL 0x00
#define HMC5843_OPMODE_POSITIVE_BIAS 0x01
#define HMC5843_OPMODE_NEGATIVE_BIAS 0x02
#define HMC5843_OPMODE_MASK 0x03

#define HMC5843_REG_CONFIG_B 0x01
#define HMC5883L_GAIN_0_88_GA (0x00 << 5)
#define HMC5883L_GAIN_1_30_GA (0x01 << 5)
#define HMC5883L_GAIN_1_90_GA (0x02 << 5)
#define HMC5883L_GAIN_2_50_GA (0x03 << 5)
#define HMC5883L_GAIN_4_00_GA (0x04 << 5)
#define HMC5883L_GAIN_4_70_GA (0x05 << 5)
#define HMC5883L_GAIN_5_60_GA (0x06 << 5)
#define HMC5883L_GAIN_8_10_GA (0x07 << 5)

#define HMC5843_GAIN_0_70_GA (0x00 << 5)
#define HMC5843_GAIN_1_00_GA (0x01 << 5)
#define HMC5843_GAIN_1_50_GA (0x02 << 5)
#define HMC5843_GAIN_2_00_GA (0x03 << 5)
#define HMC5843_GAIN_3_20_GA (0x04 << 5)
#define HMC5843_GAIN_3_80_GA (0x05 << 5)
#define HMC5843_GAIN_4_50_GA (0x06 << 5)
#define HMC5843_GAIN_6_50_GA (0x07 << 5)

#define HMC5843_REG_MODE 0x02
#define HMC5843_MODE_CONTINUOUS 0x00
#define HMC5843_MODE_SINGLE     0x01

#define HMC5843_REG_DATA_OUTPUT_X_MSB 0x03

#define HMC5843_REG_ID_A 0x0A

#define HMC5843_REG_STATUS 0x09


AP_Compass_Revo::AP_Compass_Revo(Compass &compass, AP_Revo_BusDriver *bus,
                                       bool force_external, enum Rotation rotation)
    : AP_Compass_Backend(compass)
    , _bus(bus)
    , in_progress(false)
    , read_ptr(0)
    , write_ptr(0)
    , _rotation(rotation)
    , _force_external(force_external)
{
}

AP_Compass_Revo::~AP_Compass_Revo()
{
    delete _bus;
}

AP_Compass_Backend *AP_Compass_Revo::probe(Compass &compass,
                                              AP_HAL::OwnPtr<AP_HAL::Device> dev,
                                              bool force_external,
                                              enum Rotation rotation)
{
    if (!dev) {
        return nullptr;
    }
    AP_Revo_BusDriver *bus = new AP_Revo_BusDriver_HALDevice(std::move(dev));
    if (!bus) {
        return nullptr;
    }

    AP_Compass_Revo *sensor = new AP_Compass_Revo(compass, bus, force_external, rotation);
    if (!sensor || !sensor->init()) {
        delete sensor;
        return nullptr;
    }

    return sensor;
}

AP_Compass_Backend *AP_Compass_Revo::probe_mpu6000(Compass &compass, enum Rotation rotation)
{
    return nullptr;
}

bool AP_Compass_Revo::init()
{
    AP_HAL::Semaphore *bus_sem = _bus->get_semaphore();

    if (!bus_sem || !bus_sem->take(HAL_SEMAPHORE_BLOCK_FOREVER)) {
        printf("HMC5843: Unable to get bus semaphore\n");
        return false;
    }

    // high retries for init
    _bus->set_retries(10);
    
    if (!_bus->configure()) {
        printf("HMC5843: Could not configure the bus\n");
        goto errout;
    }

    if (!_check_whoami()) {
        printf("HMC5843: not a HMC device\n");
        goto errout;
    }

    if (!_calibrate()) {
        printf("HMC5843: Could not calibrate sensor\n");
        goto errout;
    }

    if (!_setup_sampling_mode()) {
        goto errout;
    }

    if (!_bus->start_measurements()) {
        printf("HMC5843: Could not start measurements on bus\n");
        goto errout;
    }

    _gain_scale = (1.0f / 1090) * 1000;

    _initialised = true;

    // lower retries for run
    _bus->set_retries(3);
    
    bus_sem->give();

#if defined(HMC5883_DRDY_PIN)

// even we have DRDY pin we can't use attach_interrupt() because we should own the bus semaphore before transfer
    _drdy_pin = hal.gpio->channel(HMC5883_DRDY_PIN);
    _drdy_pin->mode(HAL_GPIO_INPUT);
#endif


    // perform an initial read
    read();

    _compass_instance = register_compass();

    set_rotation(_compass_instance, _rotation);
    
    _bus->set_device_type(DEVTYPE_HMC5883);
    set_dev_id(_compass_instance, _bus->get_bus_id());

    if (_force_external) {
        set_external(_compass_instance, true);
    }

    task_handle = REVOMINIScheduler::register_timer_task(1000, FUNCTOR_BIND_MEMBER(&AP_Compass_Revo::_read_fifo, void), NULL);

    REVOMINIScheduler::i_know_new_api(); // request scheduling in timers interrupt
    // read from sensor at 75Hz
    _bus->register_periodic_callback(13333,
                                     FUNCTOR_BIND_MEMBER(&AP_Compass_Revo::_timer, void));


    printf("HMC5843 found on bus 0x%x\n", (uint16_t)_bus->get_bus_id());
    
    return true;

errout:
    bus_sem->give();
    return false;
}

void AP_Compass_Revo::_calc_sample(int16_t rx, int16_t ry, int16_t rz){ // all calculations
    _mag_x = -rx; // rotate pitch_180
    _mag_y =  ry;
    _mag_z = -rz;

    // the _mag_N values are in the range -2048 to 2047, so we can
    // accumulate up to 15 of them in an int16_t. Let's make it 14
    // for ease of calculation. We expect to do reads at 10Hz, and
    // we get new data at most 75Hz, so we don't expect to
    // accumulate more than 8 before a read
    // get raw_field - sensor frame, uncorrected
    Vector3f raw_field = Vector3f(_mag_x, _mag_y, _mag_z);
    raw_field *= _gain_scale;
    
    // rotate to the desired orientation
    if (is_external(_compass_instance)) {
        raw_field.rotate(ROTATION_YAW_90);
    }

    // rotate raw_field from sensor frame to body frame
    rotate_field(raw_field, _compass_instance);
    
    // publish raw_field (uncorrected point sample) for calibration use
    publish_raw_field(raw_field, _compass_instance);
    
    // correct raw_field for known errors
    correct_field(raw_field, _compass_instance);
    
    _mag_x_accum += raw_field.x;
    _mag_y_accum += raw_field.y;
    _mag_z_accum += raw_field.z;
    _accum_count++;
    if (_accum_count == 14) {
        _mag_x_accum /= 2;
        _mag_y_accum /= 2;
        _mag_z_accum /= 2;
        _accum_count = 7;
    }
}



/*
 * take a reading from the magnetometer
 *
 * bus semaphore has been taken already by HAL
 */
void AP_Compass_Revo::_timer()
{

#if defined(HMC5883_DRDY_PIN) && 0
    if(_drdy_pin->read() == 0) {
        return; // data not ready
    }
#endif

    if(in_progress){ // there was no IO_Completion interrupt - some went wrong
        _take_sample(); // try to get next sample
        return;
    }

    REVOMINI::REVOI2CDevice *rd = (REVOMINI::REVOI2CDevice *)(_bus->get_device()); // to use non-virtual functions directly
    rd->register_completion_callback(FUNCTOR_BIND_MEMBER(&AP_Compass_Revo::_ioc, void)); // IO completion interrupt

    in_progress=true;

    _bus->block_read(HMC5843_REG_DATA_OUTPUT_X_MSB, (uint8_t *) &val, sizeof(val)); // start transfer
    
    // we own the semaprore and bus semaphore will NOT be given() by bus driver in time of transfer because it was locked by register_completion_callback()
}

void AP_Compass_Revo::_ioc() // transfer complete
{
    int16_t rx, ry, rz;

    bool result=true;

    if(_type == HMC5843){
        rx = be16toh(val.rx);
        ry = be16toh(val.ry);
        rz = be16toh(val.rz);
    } else {
        rx = be16toh(val.rx);
        ry = be16toh(val.rz);
        rz = be16toh(val.ry);
    }

// temperature in regs 0x31, 0x32

    if (rx == -4096 || ry == -4096 || rz == -4096) {
        // no valid data available
        result = false;
    }
    REVOMINI::REVOI2CDevice *rd = (REVOMINI::REVOI2CDevice *)(_bus->get_device()); // to use non-virtual functions directly
    rd->register_completion_callback((Handler)NULL);                               // allow the bus driver to release the bus semaphore
    _bus->get_semaphore()->give(); // release bus semaphore

    _take_sample(); // next sample
    
    if (result) {
        compass_sample &sp = samples[write_ptr++];
        if(write_ptr >= COMPASS_QUEUE_LEN) write_ptr=0; 
        sp.rx=rx;
        sp.ry=ry;
        sp.rz=rz;
        
//        _calc_sample(rx,ry,rz); // moved out to not wait & calculate in interrupt
    }

    in_progress=false;    
}


void AP_Compass_Revo::_read_fifo(){
    if(read_ptr!=write_ptr) {
        if (!_sem->take(HAL_SEMAPHORE_BLOCK_FOREVER)){
            return;
        }
        compass_sample &sp = samples[read_ptr++];
        if(read_ptr >= COMPASS_QUEUE_LEN) read_ptr=0;
        
        _calc_sample(sp.rx,sp.ry,sp.rz);
        _sem->give();
    }
}

/*
 * Take accumulated reads from the magnetometer or try to read once if no
 * valid data
 *
 * bus semaphore must not be locked
 */
void AP_Compass_Revo::read()
{
    if (!_initialised) {
        // someone has tried to enable a compass for the first time
        // mid-flight .... we can't do that yet (especially as we won't
        // have the right orientation!)
        return;
    }

    if (_sem->take_nonblocking()) {
    
        if (_accum_count == 0) {
            _sem->give();
            return;
        }

        Vector3f field(_mag_x_accum * _scaling[0],
                       _mag_y_accum * _scaling[1],
                   _mag_z_accum * _scaling[2]);
        field /= _accum_count;

        _accum_count = 0;
        _mag_x_accum = _mag_y_accum = _mag_z_accum = 0;

        _sem->give();
    
        publish_filtered_field(field, _compass_instance);
    }
}

bool AP_Compass_Revo::_setup_sampling_mode()
{
    uint8_t mode = HMC5843_CONF_TEMP_ENABLE | HMC5843_OSR_75HZ | HMC5843_SAMPLE_AVERAGING_8;
    uint8_t gain = HMC5883L_GAIN_1_30_GA;
    
    if (!_bus->register_write(HMC5843_REG_CONFIG_A, mode) ) {
        return false;
    }

    hal.scheduler->delay_microseconds(2);
    
//[ autodetect compass type
    uint8_t ma;
    
    if(!_bus->register_read(HMC5843_REG_CONFIG_A, &ma)) return false;
    
    if(ma == mode) {        // a 5883L supports the sample averaging config
        _type = HMC5983;
    } else if(ma == (HMC5843_OSR_75HZ | HMC5843_SAMPLE_AVERAGING_8) ){
        _type = HMC5883L;
    } else if(ma == (HMC5843_CONF_TEMP_ENABLE | HMC5843_OSR_75HZ) || ma == HMC5843_OSR_75HZ){
        gain = HMC5843_GAIN_1_50_GA;
        _type = HMC5843;
    } else { // can't detect compass type
        hal.console->printf("can't detect HMC5843 type! RegA=%x\n", ma);
        return false;
    }
//]
    mode = HMC5843_CONF_TEMP_ENABLE | HMC5843_OSR_75HZ | HMC5843_SAMPLE_AVERAGING_1; // return original settings
    if( !_bus->register_write(HMC5843_REG_CONFIG_A, mode) ||
        !_bus->register_write(HMC5843_REG_CONFIG_B, gain) ||
        !_bus->register_write(HMC5843_REG_MODE,     HMC5843_MODE_SINGLE)) return false;
        
    return true;
}

/*
 * Read Sensor data - bus semaphore must be taken
 */
bool AP_Compass_Revo::_read_sample()
{
    int16_t rx, ry, rz;

    if (!_bus->block_read(HMC5843_REG_DATA_OUTPUT_X_MSB, (uint8_t *) &val, sizeof(val))){
        return false;
    }

    if(_type == HMC5843){
        rx = be16toh(val.rx);
        ry = be16toh(val.ry);
        rz = be16toh(val.rz);
    } else {
        rx = be16toh(val.rx);
        ry = be16toh(val.rz);
        rz = be16toh(val.ry);
    }

// temperature in regs 0x31, 0x32

    if (rx == -4096 || ry == -4096 || rz == -4096) {
        // no valid data available
        return false;
    }

    _mag_x = -rx; // rotate pitch_180
    _mag_y =  ry;
    _mag_z = -rz;

    return true;
}


/*
  ask for a new oneshot sample
 */
void AP_Compass_Revo::_take_sample()
{
    _bus->register_write(HMC5843_REG_MODE,
                         HMC5843_MODE_SINGLE);
}

bool AP_Compass_Revo::_check_whoami()
{
    uint8_t id[3];
    if (!_bus->block_read(HMC5843_REG_ID_A, id, 3)) {
        // can't talk on bus
        return false;        
    }
    if (id[0] != 'H' ||
        id[1] != '4' ||
        id[2] != '3') {
        // not a HMC5x83 device
        return false;
    }

    return true;
}

bool AP_Compass_Revo::_calibrate()
{
    uint8_t calibration_gain;
    int numAttempts = 0, good_count = 0;
    bool success = false;

    calibration_gain = HMC5883L_GAIN_2_50_GA;

    /*
     * the expected values are based on observation of real sensors
     */
	float expected[3] = { 1.16*600, 1.08*600, 1.16*600 };

    uint8_t base_config = HMC5843_OSR_15HZ;
    uint8_t num_samples = 0;
    
    while (success == 0 && numAttempts < 25 && good_count < 5) {
        numAttempts++;

        // force positiveBias (compass should return 715 for all channels)
        if (!_bus->register_write(HMC5843_REG_CONFIG_A,
                                  base_config | HMC5843_OPMODE_POSITIVE_BIAS)) {
            // compass not responding on the bus
            continue;
        }

        hal.scheduler->delay(50);

        // set gains
        if (!_bus->register_write(HMC5843_REG_CONFIG_B, calibration_gain) ||
            !_bus->register_write(HMC5843_REG_MODE, HMC5843_MODE_SINGLE)) {
            continue;
        }

        // read values from the compass
        hal.scheduler->delay(50);
        if (!_read_sample()) {
            // we didn't read valid values
            continue;
        }

        num_samples++;

        float cal[3];

        // hal.console->printf("mag %d %d %d\n", _mag_x, _mag_y, _mag_z);

        cal[0] = fabsf(expected[0] / _mag_x);
        cal[1] = fabsf(expected[1] / _mag_y);
        cal[2] = fabsf(expected[2] / _mag_z);

        // hal.console->printf("cal=%.2f %.2f %.2f\n", cal[0], cal[1], cal[2]);

        // we throw away the first two samples as the compass may
        // still be changing its state from the application of the
        // strap excitation. After that we accept values in a
        // reasonable range
        if (numAttempts <= 2) {
            continue;
        }

#define IS_CALIBRATION_VALUE_VALID(val) (val > 0.7f && val < 1.35f)

        if (IS_CALIBRATION_VALUE_VALID(cal[0]) &&
            IS_CALIBRATION_VALUE_VALID(cal[1]) &&
            IS_CALIBRATION_VALUE_VALID(cal[2])) {
            // hal.console->printf("car=%.2f %.2f %.2f good\n", cal[0], cal[1], cal[2]);
            good_count++;

            _scaling[0] += cal[0];
            _scaling[1] += cal[1];
            _scaling[2] += cal[2];
        }

#undef IS_CALIBRATION_VALUE_VALID

#if 0
        /* useful for debugging */
        hal.console->printf("MagX: %d MagY: %d MagZ: %d\n", (int)_mag_x, (int)_mag_y, (int)_mag_z);
        hal.console->printf("CalX: %.2f CalY: %.2f CalZ: %.2f\n", cal[0], cal[1], cal[2]);
#endif
    }

    _bus->register_write(HMC5843_REG_CONFIG_A, base_config);
    
    if (good_count >= 5) {
        _scaling[0] = _scaling[0] / good_count;
        _scaling[1] = _scaling[1] / good_count;
        _scaling[2] = _scaling[2] / good_count;
        success = true;
    } else {
        /* best guess */
        _scaling[0] = 1.0;
        _scaling[1] = 1.0;
        _scaling[2] = 1.0;
        if (num_samples > 5) {
            // a sensor can be broken for calibration but still
            // otherwise workable, accept it if we are reading samples
            success = true;
        }
    }

#if 0
    printf("scaling: %.2f %.2f %.2f\n",
           _scaling[0], _scaling[1], _scaling[2]);
#endif
    
    return success;
}

/* AP_HAL::Device implementation of the HMC5843 */
AP_Revo_BusDriver_HALDevice::AP_Revo_BusDriver_HALDevice(AP_HAL::OwnPtr<AP_HAL::Device> dev)
    : _dev(std::move(dev))
{
    // set read and auto-increment flags on SPI
    if (_dev->bus_type() == AP_HAL::Device::BUS_TYPE_SPI) {
        _dev->set_read_flag(0xC0);
    }
}

bool AP_Revo_BusDriver_HALDevice::block_read(uint8_t reg, uint8_t *buf, uint32_t size)
{
    return _dev->read_registers(reg, buf, size);
}

bool AP_Revo_BusDriver_HALDevice::register_read(uint8_t reg, uint8_t *val)
{
    return _dev->read_registers(reg, val, 1);
}

bool AP_Revo_BusDriver_HALDevice::register_write(uint8_t reg, uint8_t val)
{
    return _dev->write_register(reg, val);
}

AP_HAL::Semaphore *AP_Revo_BusDriver_HALDevice::get_semaphore()
{
    return _dev->get_semaphore();
}

AP_HAL::Device::PeriodicHandle AP_Revo_BusDriver_HALDevice::register_periodic_callback(uint32_t period_usec, AP_HAL::Device::PeriodicCb cb)
{
    return _dev->register_periodic_callback(period_usec, cb);
}


/* HMC5843 on an auxiliary bus of IMU driver */
AP_Revo_BusDriver_Auxiliary::AP_Revo_BusDriver_Auxiliary(AP_InertialSensor &ins, uint8_t backend_id,
                                                               uint8_t addr)
{
    /*
     * Only initialize members. Fails are handled by configure or while
     * getting the semaphore
     */
    _bus = ins.get_auxiliary_bus(backend_id);
    if (!_bus) {
        return;
    }

    _slave = _bus->request_next_slave(addr);
}

AP_Revo_BusDriver_Auxiliary::~AP_Revo_BusDriver_Auxiliary()
{
    /* After started it's owned by AuxiliaryBus */
    if (!_started) {
        delete _slave;
    }
}

bool AP_Revo_BusDriver_Auxiliary::block_read(uint8_t reg, uint8_t *buf, uint32_t size)
{
    if (_started) {
        /*
         * We can only read a block when reading the block of sample values -
         * calling with any other value is a mistake
         */
        assert(reg == HMC5843_REG_DATA_OUTPUT_X_MSB);

        int n = _slave->read(buf);
        return n == static_cast<int>(size);
    }

    int r = _slave->passthrough_read(reg, buf, size);

    return r > 0 && static_cast<uint32_t>(r) == size;
}

bool AP_Revo_BusDriver_Auxiliary::register_read(uint8_t reg, uint8_t *val)
{
    return _slave->passthrough_read(reg, val, 1) == 1;
}

bool AP_Revo_BusDriver_Auxiliary::register_write(uint8_t reg, uint8_t val)
{
    return _slave->passthrough_write(reg, val) == 1;
}

AP_HAL::Semaphore *AP_Revo_BusDriver_Auxiliary::get_semaphore()
{
    return _bus->get_semaphore();
}


bool AP_Revo_BusDriver_Auxiliary::configure()
{
    if (!_bus || !_slave) {
        return false;
    }
    return true;
}

bool AP_Revo_BusDriver_Auxiliary::start_measurements()
{
    if (_bus->register_periodic_read(_slave, HMC5843_REG_DATA_OUTPUT_X_MSB, 6) < 0) {
        return false;
    }

    _started = true;

    return true;
}

AP_HAL::Device::PeriodicHandle AP_Revo_BusDriver_Auxiliary::register_periodic_callback(uint32_t period_usec, AP_HAL::Device::PeriodicCb cb)
{
    return _bus->register_periodic_callback(period_usec, cb);
}

// set device type within a device class
void AP_Revo_BusDriver_Auxiliary::set_device_type(uint8_t devtype)
{
    _bus->set_device_type(devtype);
}

// return 24 bit bus identifier
uint32_t AP_Revo_BusDriver_Auxiliary::get_bus_id(void) const
{
    return _bus->get_bus_id();
}

#endif