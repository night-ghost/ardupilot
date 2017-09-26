#pragma once

/*

    copied from AP_Compass_HMC5843.h and altered to ose new IO_Completion API
    
*/


#include <AP_Common/AP_Common.h>
#include <AP_HAL/AP_HAL.h>
#include <AP_HAL/Device.h>
#include <AP_Math/AP_Math.h>
#include <AP_HAL/utility/sparse-endian.h>

#include "AP_Compass.h"
#include "AP_Compass_Backend.h"

class AuxiliaryBus;
class AuxiliaryBusSlave;
class AP_InertialSensor;
class AP_Revo_BusDriver;

class AP_Compass_Revo : public AP_Compass_Backend
{
public:
    enum Type {
        Unknown,
        HMC5883L,
        HMC5983,
        HMC5843,
    };
        
    static AP_Compass_Backend *probe(Compass &compass,
                                     AP_HAL::OwnPtr<AP_HAL::Device> dev,
                                     bool force_external = false,
                                     enum Rotation rotation = ROTATION_NONE);

    static AP_Compass_Backend *probe_mpu6000(Compass &compass, enum Rotation rotation = ROTATION_NONE);

    static constexpr const char *name = "HMC5843";

    virtual ~AP_Compass_Revo();

    void read() override;

private:
    AP_Compass_Revo(Compass &compass, AP_Revo_BusDriver *bus,
                       bool force_external, enum Rotation rotation);

    bool init();
    bool _check_whoami();
    bool _calibrate();
    bool _setup_sampling_mode();

    void _calc_sample(int16_t rx, int16_t ry, int16_t rz);
    void _timer();
    void _ioc();
    void _write_ioc();
    void _read_fifo();

    /* Read a single sample */
    bool _read_sample();

    // ask for a new sample
    void _take_sample();

    AP_Revo_BusDriver *_bus;
    AP_HAL::DigitalSource *_drdy_pin;

    float _scaling[3];
    float _gain_scale;

    int16_t _mag_x;
    int16_t _mag_y;
    int16_t _mag_z;
    int16_t _mag_x_accum;
    int16_t _mag_y_accum;
    int16_t _mag_z_accum;
    uint8_t _accum_count;
    
    volatile struct PACKED { // let it be here, not in stack
        be16_t rx;
        be16_t ry;
        be16_t rz;
    } val; 

    typedef struct Compass_Sample {
        int16_t rx;
        int16_t ry;
        int16_t rz;
    } compass_sample;

    volatile bool in_progress;
    
#define COMPASS_QUEUE_LEN 15
    compass_sample samples[COMPASS_QUEUE_LEN+1];
    uint8_t read_ptr;
    volatile uint8_t write_ptr;

    uint8_t _compass_instance;
    enum Type _type;

    enum Rotation _rotation;
    
    bool _initialised:1;
    bool _force_external:1;

};

class AP_Revo_BusDriver
{
public:
    virtual ~AP_Revo_BusDriver() { }

    virtual bool block_read(uint8_t reg, uint8_t *buf, uint32_t size) = 0;
    virtual bool register_read(uint8_t reg, uint8_t *val) = 0;
    virtual bool register_write(uint8_t reg, uint8_t val) = 0;

    virtual AP_HAL::Semaphore *get_semaphore() = 0;

    virtual bool configure() { return true; }
    virtual bool start_measurements() { return true; }

    virtual AP_HAL::Device::PeriodicHandle register_periodic_callback(uint32_t, AP_HAL::Device::PeriodicCb) = 0;

    // set device type within a device class
    virtual void set_device_type(uint8_t devtype) = 0;

    // return 24 bit bus identifier
    virtual uint32_t get_bus_id(void) const = 0;

    virtual void set_retries(uint8_t retries) {}
    
    virtual AP_HAL::Device * get_device() const = 0; // temporary method to use non-virtual methods of bus driver
};

class AP_Revo_BusDriver_HALDevice : public AP_Revo_BusDriver
{
public:
    AP_Revo_BusDriver_HALDevice(AP_HAL::OwnPtr<AP_HAL::Device> dev);

    bool block_read(uint8_t reg, uint8_t *buf, uint32_t size) override;
    bool register_read(uint8_t reg, uint8_t *val) override;
    bool register_write(uint8_t reg, uint8_t val) override;

    AP_HAL::Semaphore *get_semaphore() override;

    AP_HAL::Device::PeriodicHandle register_periodic_callback(uint32_t period_usec, AP_HAL::Device::PeriodicCb) override;

    // set device type within a device class
    void set_device_type(uint8_t devtype) override {
        _dev->set_device_type(devtype);
    }

    // return 24 bit bus identifier
    uint32_t get_bus_id(void) const override {
        return _dev->get_bus_id();
    }

    void set_retries(uint8_t retries) override {
        return _dev->set_retries(retries);
    }
    
    AP_HAL::Device * get_device() const { return _dev.get(); }// temporary method to use non-virtual methods of bus driver
    
private:
    AP_HAL::OwnPtr<AP_HAL::Device> _dev;
};

class AP_Revo_BusDriver_Auxiliary : public AP_Revo_BusDriver
{
public:
    AP_Revo_BusDriver_Auxiliary(AP_InertialSensor &ins, uint8_t backend_id,
                                   uint8_t addr);
    virtual ~AP_Revo_BusDriver_Auxiliary();

    bool block_read(uint8_t reg, uint8_t *buf, uint32_t size) override;
    bool register_read(uint8_t reg, uint8_t *val) override;
    bool register_write(uint8_t reg, uint8_t val) override;

    AP_HAL::Semaphore *get_semaphore() override;

    bool configure() override;
    bool start_measurements() override;

    AP_HAL::Device::PeriodicHandle register_periodic_callback(uint32_t period_usec, AP_HAL::Device::PeriodicCb) override;

    // set device type within a device class
    void set_device_type(uint8_t devtype) override;

    // return 24 bit bus identifier
    uint32_t get_bus_id(void) const override;
    
private:
    AuxiliaryBus *_bus;
    AuxiliaryBusSlave *_slave;
    bool _started;
};
