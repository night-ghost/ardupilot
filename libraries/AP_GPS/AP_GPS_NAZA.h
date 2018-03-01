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


#pragma once

#include "AP_GPS.h"
#include "GPS_Backend.h"

#define NAZA_MAX_PAYLOAD_SIZE   256

typedef struct {
    uint8_t res[4]; // 0
    uint8_t fw[4]; // 4
    uint8_t hw[4]; // 8
} naza_ver;

typedef struct {
    uint16_t x; // 0
    uint16_t y; // 2
    uint16_t z; // 4
} naza_mag;

typedef struct {
    uint16_t time; // GPS msToW 0
    uint16_t date;
    int32_t longitude; // 4
    int32_t latitude; // 8
    int32_t altitude_msl; // 12
    int32_t h_acc; // 16
    int32_t v_acc; // 20
    int32_t reserved;
    int32_t ned_north; // 28
    int32_t ned_east; // 32
    int32_t ned_down;  // 36
    uint16_t pdop;  // 40
    uint16_t vdop;  // 42
    uint16_t ndop; // 44
    uint16_t edop;  // 46
    uint8_t satellites; // 48
    uint8_t reserved3; //
    uint8_t fix_type; // 50
    uint8_t reserved4; //
    uint8_t fix_status; // 52
    uint8_t reserved5;
    uint8_t reserved6;
    uint8_t mask;   // 55
} naza_nav;


typedef enum {
    HEADER1 = 0x55,
    HEADER2 = 0xAA,
    ID_NAV = 0x10,
    ID_MAG = 0x20,
    ID_VER = 0x30,
    LEN_NAV = 0x3A,
    LEN_MAG = 0x06,
} naza_protocol_bytes;

typedef enum {
    NO_FIX = 0,
    FIX_2D = 2,
    FIX_3D = 3,
    FIX_DGPS = 4
} fixType_t;

// Receive buffer
static union {
    naza_mag mag;
    naza_nav nav;
    naza_ver ver;
    uint8_t bytes[NAZA_MAX_PAYLOAD_SIZE];
} _buffernaza;


typedef struct Mag {
    float x;
    float y;
    float z;
    bool valid;
} mag_t;

class AP_GPS_NAZA : public AP_GPS_Backend {
public:
    AP_GPS_NAZA(AP_GPS &_gps, AP_GPS::GPS_State &_state, AP_HAL::UARTDriver *_port);

    bool read(void);

    static bool _detect(struct MTK_detect_state &state, uint8_t data);
    static void send_init_blob(uint8_t instance, AP_GPS &gps) {}

    const char *name() const override { return "NAZA"; }

private:

    bool _decode(uint8_t c);
    bool _parse_gps();
    
    int32_t decodeLong(uint32_t idx, uint8_t mask);
    int16_t decodeShort(uint16_t idx, uint8_t mask);

// Packet checksum accumulators
    uint8_t _ck_a;
    uint8_t _ck_b;

// State machine state
    bool _skip_packet;
    uint8_t _step;
    uint8_t _msg_id;
    uint16_t _payload_length;
    uint16_t _payload_counter;
    
    
    mag_t _mag;
    
    struct {
        uint32_t errors;
        uint32_t packets;
    } _stats;
};

