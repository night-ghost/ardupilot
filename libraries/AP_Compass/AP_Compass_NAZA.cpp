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
 *       AP_Compass_NAZA.cpp - backend for AP_Compass
 *
 */


#include <AP_HAL/AP_HAL.h>
#include "AP_Compass_NAZA.h"

extern const AP_HAL::HAL& hal;

// constructor
AP_Compass_NAZA::AP_Compass_NAZA(Compass &compass):
    AP_Compass_Backend(compass)
{
    _compass._setup_earth_field();
}

// detect the sensor
AP_Compass_Backend *AP_Compass_NAZA::detect(Compass &compass)
{
    AP_Compass_NAZA *sensor = new AP_Compass_NAZA(compass);
    if (sensor == nullptr) {
        return nullptr;
    }
    if (!sensor->init()) {
        delete sensor;
        return nullptr;
    }
    return sensor;
}

bool
AP_Compass_NAZA::init(void)
{
    if(AP::gps().has_compass()) {
        _instance = register_compass();
        return true;
    }
    return false;
}

void AP_Compass_NAZA::read()
{
    Vector3f field;
    if( AP::gps().get_compass(field) ){
        rotate_field(field, _instance);
        publish_raw_field(field, _instance);
        correct_field(field, _instance);
        uint32_t saved_last_update = _compass.last_update_usec(_instance);
        publish_filtered_field(field, _instance);
        set_last_update_usec(saved_last_update, _instance);
    }
}
