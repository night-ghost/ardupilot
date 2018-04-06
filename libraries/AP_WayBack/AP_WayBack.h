/*

 this library collects points of drone position, records them in memory and eliminates 
 unnecessary data - redundant points and loops, so it always knows 
 the shortest way to home via visited points. The loops are reduced ASAP so this way is 
 not "true the best" from mathematic's view, but this approach greatly simplifies code.
 
 Copyright night_ghost@ykoctpa.ru 2017
  

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

#include <AP_HAL/AP_HAL.h>
#include <AP_AHRS/AP_AHRS.h>

#define NUM_TRACK_POINTS_DEFAULT 4000  // 4000 points by 8 bytes = 32K of RAM. Feel free to change size via parmeter
#define SIMPLIFY_MOVE_BACK          2  // move N points back
#define MIN_SIMPLIFY_POINTS         8  // this number should be adjusted upto value which not cause processing time to exceed 1s at full load
#define RAW_POINTS                  3

#define WAYBACK_PRIORITY 116 // speed 1/16 of main task


#ifdef WAYBACK_DEBUG
    #define DBG_PRINTLN(x)     { hal.uartA->printf("#%d\n"); }
    #define DBG_PRINT(x)       { hal.uartA->printf("#%d ");  }
    #define DBG_PRINTVARLN(x)  { hal.uartA->print("#" #x ": %d\n"); }
    #define DBG_PRINTVAR(x)    { hal.uartA->print("#" #x ": %d ");  }
    #define DBG_PRINTF(x,...)  { hal.uartA->printf("#" x,## __VA_ARGS__); }
#else
    #define DBG_PRINTLN(x)     {}
    #define DBG_PRINT(x)       {}
    #define DBG_PRINTVARLN(x)  {}
    #define DBG_PRINTVAR(x)    {}
    #define DBG_PRINTF(x,...)  {}
#endif

#define LOOPS_MOVE_BACK 12 // new segments not checked for loops

#define EPS_DISTANCE 1 // in meters, by default

#define SHORTCUT_FACTOR 2 // на сколько чувствительность "близости" точек больше чем Эпсилон

//#if FRAME_CONFIG == MULTICOPTER_FRAME

// copter's coordinates in meters
#define TRACK_EPS (_params.eps) // meters 

//#else

// all another coordinates are from GPS so 1 gradus is ~110KM 
//#define TRACK_EPS (_params.eps * 110000) //  meters in gradus of coords 

//#endif

template<typename T> void zeroIt(T& value)
{
    memset(&value,0,sizeof(value));
}


class AP_WayBack
{
public:

#pragma pack(push, 1)
    typedef struct POINT {
        float x;
        float y;
    } Point;

    typedef struct SEGMENT {
        Point &b;
        Point &e;
    } Segment;
#pragma pack(pop)

    AP_WayBack(){}
    
    ~AP_WayBack() {
        end();
    }
    
//main API
    // init - perform required initialisation
    void init();
    void init(float eps, uint16_t points, bool bs);

    // copter just armed so start to write track or RTH is turned off. If failed return false
    bool start(); 

    // we start RTH so stop to record points of track 
    void stop(); 

    // copter disarmed so free memory
    void end(){
        free(points);
        points=NULL;
    }
    
    void push_point(Vector3f p);

    static inline bool get_points_count()  { return points_count; }
    static inline bool is_active() { return recording; }

// get and remove the last point from track, false if none
    bool get_point(float &x, float &y) ; 
    bool get_point(float &x, float &y, float &z) ; 

    // tick - main call to get position from AHRS (called by scheduler at low priority) 
    // non-static to conrform ARDUPILOT API
    void tick(void);


// the main function - adding a point with all calculations
// public only for debug
    void add_point(float x, float y);

#ifdef WAYBACK_DEBUG
    // for debug
    static bool show_track(uint16_t &n, float &x, float &y);
    static void set_debug_mode(bool v) { _debug_mode = v; }
    
    static bool _debug_mode;
#endif


private:
    static void *_task; // handle of own task

    #define POINTS_QUEUE_LEN 8 
    static Vector3f _queue[POINTS_QUEUE_LEN];
    static uint16_t _read_ptr, _write_ptr;

    static uint16_t num_points; // index of last point in array
    static uint16_t points_count; // number of good points
    static uint16_t new_points;  // number of  points after last reduce

    static uint16_t last_loop_check; // last leg checked for loop
    static uint16_t last_reduce; // last leg checked for reduce
    static uint16_t last_big_reduce; // last leg checked when new point added
    static uint16_t last_raw_point;
    static bool     in_loop_reduce;
    static uint16_t hi_loop_border;
    
    // for move-back mode
    static uint16_t loop_leg_low; // lowest checked - we move back!
    static uint16_t loop_leg_high;// higest checked - will be next last_loop_check on finish
    static uint16_t loop_leg_ptr;//  current, goes to loop_leg_low

    static bool recording;
    static uint32_t last_point_time;
    
    
    static float _epsilon;

    static float max_alt; // maximal altitude during flight

// calculations
    static        float dist( float p1X, float p1Y, float p2X, float p2Y);


    static inline float dist(const Point &p1, const Point &p2) {
        return dist( p1.x, p1.y, p2.x, p2.y);
    }

    static inline float dist( float p1X, float p1Y, const Point &p){
        return dist( p1X, p1Y, p.x, p.y);
    }

    static inline float dist(const Segment &s) {
        return dist(s.b, s.e);
    }


// to simplify usage
    static inline bool is_good(Point p)  { return !isnan(p.x); }
    static inline void do_good(Point &p) { /* automatically after setting good coord */ }
    static inline void do_bad(Point &p)  { p.x=NAN; }
    
    static uint16_t move_back(uint16_t from, uint16_t steps);
    static uint16_t move_forw(uint16_t from, uint16_t steps);

    static bool simplify( uint16_t start, uint16_t end);

    uint16_t try_remove_loop(uint16_t start, uint16_t end);
    
    static void squizze(); // remove unused points from data array

    static bool removePoints(uint16_t start, uint16_t end);    

// mathematical magick

    // distance to line
    static         float find_perpendicular_distance(const Point &p, const Point &p1, const Point &p2);
    static inline  float find_perpendicular_distance(const Point &p, const Segment &s) { return find_perpendicular_distance(p, s.b, s.e ); }
    
    // distance to segment
    static        float distanceToSegment(const Point &p,             // point
                                   const Point &p1, const Point &p2, // segment
                                   Point *i); // return coordinates of closest point
    static inline float distanceToSegment(const Point &p, const Segment &s, Point *i){ // return coordinates of closest point
                            return distanceToSegment(p, s.b, s.e, i);
    }
                            
    static        bool linesIntersect(const Point &p1, const Point &p2,
                               const Point &p3, const Point &p4,
                               Point *cross);
    static inline bool linesIntersect(const Segment &s1, const Segment &s2,
                               Point *cross) { return linesIntersect(s1.b,s1.e,s2.b,s2.e,cross); }

    static        uint8_t linesAreClose(const Point &p1, const Point &p2,
                                 const Point &p3, const Point &p4,
                                 Point *closest,
                                 uint16_t *np);
    static inline uint8_t linesAreClose(const Segment &s1, const Segment &s2,
                                 Point *closest,
                                 uint16_t *np) { return linesAreClose(s1.b, s1.e, s2.b, s2.e, closest, np); }


    static bool rdp_simplify( uint16_t start, uint16_t end);    
    static bool reumannWitkam_simplify(uint16_t key, uint16_t end);

    static Point *points;  // the main track storage - huge size! so allocated only on start()
    static uint16_t max_num_points;

//[ parameter support
    static const struct AP_Param::GroupInfo        var_info[];
    static struct Params {
        AP_Int8 eps; // in meters
        AP_Int8 blind_shortcut; // if 1 then non-intersecting legs with distance<EPS will be treated as intersecting
    } _params;
    AP_Int16 _points_max;
//]    
    static bool initialized;
};

