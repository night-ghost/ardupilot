#pragma once

//#define MAVLINK_EXTERNAL_RX_BUFFER 1
//#define m_mavlink_message 1

//#define MAVLINK_GET_CHANNEL_BUFFER

#define MAX_OVERLOAD_COUNT 10
#define MAX_FROZEN_COUNT 10

//#include <GCS_MAVLink/include/mavlink/v1.0/mavlink_types.h>

//#include "mavlink_types.h"

extern union UU msgbuf;

//#define MAVLINK_HELPER 

// #define  m_mavlink_buffer (&msgbuf.m)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"

#include "GCS_MAVLink.h"



union UU {
    mavlink_message_t m;

    byte bytes[0x40]; // for font uploading 
} msgbuf;


#include "protocols/MAVLink.h"

#pragma GCC diagnostic pop

#define MAX_OVERLOAD_COUNT 10
#define MAX_FROZEN_COUNT 10





