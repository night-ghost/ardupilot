#include <stdio.h>

#include <AP_Common/AP_Common.h>

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>

#include "AP_GPS_NAZA.h"


AP_GPS_NAZA::AP_GPS_NAZA(AP_GPS &_gps, AP_GPS::GPS_State &_state, AP_HAL::UARTDriver *_port) :
    AP_GPS_Backend(_gps, _state, _port),
    _skip_packet(0),
    _step(0),
    _msg_id(0),
    _payload_length(0),
    _payload_counter(0)
{
    memset(&_buffernaza, 0, sizeof(_buffernaza));
}

bool AP_GPS_NAZA::read(void)
{
    bool parsed = false;

    while (port->available()) {
        char c = port->read();
        if (_decode(c)) {
            parsed = true;
        }
    }
    return parsed;
}

int32_t AP_GPS_NAZA::decodeLong(uint32_t idx, uint8_t mask)
{
    union { uint32_t l; uint8_t b[4]; } val;
    val.l=idx;
    for (int i = 0; i < 4; i++) val.b[i] ^= mask;
    return val.l;
}

int16_t AP_GPS_NAZA::decodeShort(uint16_t idx, uint8_t mask)
{
    union { uint16_t s; uint8_t b[2]; } val;
    val.s=idx;
    for (int i = 0; i < 2; i++) val.b[i] ^= mask;
    return val.s;
}


bool AP_GPS_NAZA::_decode(uint8_t data)
{
    bool parsed = false;

    switch (_step) {
        case 0: // Sync char 1 (0x55)
            if (HEADER1 == data) {
                _skip_packet = false;
                _step++;
            }
            break;
        case 1: // Sync char 2 (0xAA)
            if (HEADER2 != data) {
                _step = 0;
                break;
            }
            _step++;
            break;
        case 2: // Id
            _step++;
            _ck_b = _ck_a = data;   // reset the checksum accumulators
            _msg_id = data;
            break;
        case 3: // Payload length
            _step++;
            _ck_b += (_ck_a += data);       // checksum byte
            _payload_length = data; // payload length low byte
            if (_payload_length > NAZA_MAX_PAYLOAD_SIZE) {
                // we can't receive the whole packet, just log the error and start searching for the next packet.
                _stats.errors++;
                _step = 0;
                break;
            }
            // prepare to receive payload
            _payload_counter = 0;
            if (_payload_length == 0) {
                _step = 6;
            }
            break;
        case 4:
            _ck_b += (_ck_a += data);       // checksum byte
            if (_payload_counter < NAZA_MAX_PAYLOAD_SIZE) {
                _buffernaza.bytes[_payload_counter] = data;
            }
            if (++_payload_counter >= _payload_length) {
                _step++;
            }
            break;
        case 5:
            _step++;
            if (_ck_a != data) {
                _skip_packet = true;          // bad checksum
                _stats.errors++;
            }
            break;
        case 6:
            _step = 0;
            if (_ck_b != data) {
                _stats.errors++;
                break;              // bad checksum
            }

            _stats.packets++;

            if (_skip_packet) {
                break;
            }

            if (_parse_gps()) {
                parsed = true;
            }
    }
    return parsed;
}



bool AP_GPS_NAZA::_parse_gps(void)
{

    bool ret;

    switch (_msg_id) {
    case ID_NAV: {
        uint8_t mask;

        mask = _buffernaza.nav.mask; // TODO: call DecodeAll() and use vars directly

        make_gps_time(decodeShort(_buffernaza.nav.date, mask), decodeShort(_buffernaza.nav.time, mask) * 10);

        state.location.lng = decodeLong(_buffernaza.nav.longitude, mask);
        state.location.lat = decodeLong(_buffernaza.nav.latitude, mask);
        state.location.alt = decodeLong(_buffernaza.nav.altitude_msl, mask) * (1/10.0f);  //alt in cm

        uint8_t fixType = _buffernaza.nav.fix_type ^ mask;

        if (fixType == FIX_2D)
            state.status = AP_GPS::GPS_OK_FIX_2D;
        else if (fixType == FIX_3D)
            state.status = AP_GPS::GPS_OK_FIX_3D;
        else
            state.status = AP_GPS::NO_FIX;

        int32_t velNED[3];

        velNED[0] = decodeLong(_buffernaza.nav.ned_north, mask);  // cm/s
        velNED[1] = decodeLong(_buffernaza.nav.ned_east, mask);   // cm/s
        velNED[2] = decodeLong(_buffernaza.nav.ned_down, mask);   // cm/s

        state.hdop = decodeShort(_buffernaza.nav.pdop, mask); // pdop
        state.vdop = decodeShort(_buffernaza.nav.vdop, mask); // vdop

        state.horizontal_accuracy = decodeLong(_buffernaza.nav.h_acc, mask); // mm
        state.vertical_accuracy   = decodeLong(_buffernaza.nav.v_acc, mask); // mm
        state.have_horizontal_accuracy = true;
        state.have_vertical_accuracy = true;
        
//        gpsSol.eph = gpsConstrainEPE(h_acc / 10);   // hAcc in cm
//        gpsSol.epv = gpsConstrainEPE(v_acc / 10);   // vAcc in cm
        state.num_sats = _buffernaza.nav.satellites;
        state.ground_speed = sqrtf(velNED[0] * velNED[0] + velNED[1]*velNED[1] ) * 0.01; //cm/s

        // calculate gps heading from VELNE
        state.ground_course = wrap_360(degrees(atan2(velNED[1], velNED[0])));

        state.last_gps_time_ms = AP_HAL::millis();
        ret = true;
        }
        break;

    case ID_MAG: {
        uint8_t mask_mag;

        mask_mag = (_buffernaza.mag.z)&0xFF;
        mask_mag = (((mask_mag ^ (mask_mag >> 4)) & 0x0F) | ((mask_mag << 3) & 0xF0)) ^ (((mask_mag & 0x01) << 3) | ((mask_mag & 0x01) << 7));

        Vector3f mag;
        mag.x = decodeShort(_buffernaza.mag.x, mask_mag);
        mag.y = decodeShort(_buffernaza.mag.y, mask_mag);
        mag.z = (_buffernaza.mag.z ^ (mask_mag<<8));

        gps.update_compass(mag);
        }
        break;
    case ID_VER:
        break;

    default:
        return false;
    }

    return ret;
}

