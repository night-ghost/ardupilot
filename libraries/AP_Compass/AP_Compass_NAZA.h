#pragma once

#include "AP_Compass.h"
#include <AP_GPS/AP_GPS.h>

#define NAZA_NUM_COMPASSES 1

class AP_Compass_NAZA : public AP_Compass_Backend
{
public:
    AP_Compass_NAZA(Compass &compass);
    void read(void);
    bool init(void);

    // detect the sensor
    static AP_Compass_Backend *detect(Compass &compass);

private:
    uint8_t _instance;
};
