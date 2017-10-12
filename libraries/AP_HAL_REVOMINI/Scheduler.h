#ifndef __AP_HAL_REVOMINI_SCHEDULER_H__
#define __AP_HAL_REVOMINI_SCHEDULER_H__


#pragma GCC push_options
#pragma GCC optimize ("O2")
#include <AP_HAL/AP_HAL.h>
#pragma GCC pop_options


#include "AP_HAL_REVOMINI_Namespace.h"
#include "handler.h"
#include "Config.h"

#include "Semaphores.h"
#include "GPIO.h"

#include <delay.h>
#include <systick.h>
#include <boards.h>
#include <timer.h>
#include <setjmp.h>

#define REVOMINI_SCHEDULER_MAX_TIMER_PROCS 10
#define REVOMINI_SCHEDULER_MAX_IO_PROCS 10
#define REVOMINI_SCHEDULER_MAX_SHEDULED_PROCS 32

#define USE_ISR_SCHED 1

#define SHED_FREQ 10000 // in Hz

#define MAIN_STACK_SIZE  10240U   // measured use of stack is only 1K - but it grows up to 4K when using FatFs, also this includes 2K stack for ISR
#define DEFAULT_STACK_SIZE  8192U // Default tasks stack size and stack max - io_thread can do work with filesystem
#define SLOW_TASK_STACK 1024U // 2048U     // small stack for sensors
#define STACK_MAX  65536U


struct task_t {
#ifdef PREEMPTIVE
        const uint8_t* sp;      //!< Task stack pointer, should be first to access from context switcher
#else
        jmp_buf context;        //!< Task context
#endif
        task_t* next;           //!< Next task
        task_t* prev;           //!< Previous task
        Handler handle;         //!< loop() in Revo_handler - to allow to change task, call via revo_call_handler
        const uint8_t* stack;   //!< Task stack, should be first to access from context switcher
        uint8_t id;             // id of task
        uint8_t priority;       // priority of task
        uint8_t curr_prio;      // current priority of task, usually higher than priority
        bool active;            // task not ended
        bool in_ioc;            // task starts IO_Completion so don't release bus semaphore
        uint8_t has_semaphore;  // count how many times task has a semaphore so let it run until gives it
        uint32_t ttw;           // time to work
        uint32_t t_yield;       // time of yield
        uint32_t start;         // microseconds of timeslice start
        uint32_t max_delay;     // maximal execution time of task - used in scheduler
        uint32_t in_isr;        // time in ISR when task runs
        uint32_t period;        // if set then task starts on time basis only
        REVOMINI::Semaphore *sem; // task should start after owning this semaphore
        REVOMINI::Semaphore *sem_wait; // task is waiting this semaphore
        uint32_t def_ttw;       // default TTW - not as hard as period
        uint32_t time_start;    // start time of task
#ifdef MTASK_PROF
        uint64_t time;  // full time
        uint32_t max_time; //  maximal execution time of task - to show
        uint32_t maxt_addr; // address of end of max-time code
        uint32_t count;     // call count to calc mean
        uint32_t sched_error; // delay on task start
        uint32_t sem_count;   // how many times task was not switched because has semaphore
#endif
        uint32_t guard; // stack guard
};

extern "C" {
    extern unsigned _estack; // defined by link script
    extern uint32_t us_ticks;
    extern void *__brkval;
    extern void *_sdata;
    extern void *_sccm;  // start of CCM
    extern void *_eccm;  // end of CCM vars

    void revo_call_handler(Handler hh, uint32_t arg); // universal caller for all type handlers - memberProc and Proc

    extern voidFuncPtr boardEmergencyHandler; // will be called on any fault or panic() before halt
    void PendSV_Handler();
    void SVC_Handler();
    void getNextTask();
    
    void switchContext();
    void __do_context_switch();

    extern task_t *s_running; // running task 
    extern task_t *next_task; // task to run next

    extern caddr_t stack_bottom; // for SBRK check
    
// publish to low-level functions
    void hal_yield(uint16_t ttw);
    void hal_delay(uint16_t t);
    void hal_delay_microseconds(uint16_t t);
    void hal_delay_us_ny(uint16_t t);
    uint32_t hal_micros();
}


#define RAMEND ((size_t)&_estack)


typedef enum REVO_cb_type {
    CB_MEMBERPROC = 0,
    CB_PERIODIC,
    CB_PERIODICBOOL,
} revo_cb_type;

#pragma pack(push, 1)
union Revo_cb { // кровь кишки ассемблер :) преобразование функторов в унифицированный вид
    AP_HAL::MemberProc                  mp;
    AP_HAL::Device::PeriodicCb          pcb;
    AP_HAL::Device::PeriodicCbBool      pcbb;
    Handler                             h;    // treat as handle
    uint32_t                            w[2]; // words, to check
};
#pragma pack(pop)

#if USE_ISR_SCHED
typedef struct RevoTimer {
    uint32_t period;            // interval in uS
    uint32_t last_run;          // last run time
//    union Revo_cb proc; // can't declare - got error "use of deleted function 'RevoTimer::RevoTimer()"
    Handler proc;          //AP_HAL::Device::PeriodicCb proc and AP_HAL::MemberProc mp together
    REVOMINI::Semaphore *sem; // semaphore to get
    REVOMINI::Semaphore *sem2;// semaphore to check
    revo_cb_type mode;

 #ifdef SHED_PROF
    uint32_t micros;    // max exec time
    uint32_t count;     // number of calls
    uint64_t fulltime;  // full consumed time to calc mean
 #endif
} revo_timer;

typedef struct RevoTick {
//    union Revo_cb proc; // can't declare - got error "use of deleted function 'RevoTimer::RevoTimer()"
    Handler proc;          //AP_HAL::Device::PeriodicCb proc and AP_HAL::MemberProc mp together
    REVOMINI::Semaphore *sem;
} revo_tick;
#endif

#ifdef SHED_DEBUG
typedef struct RevoSchedLog {
    uint32_t start;
    uint32_t end;
    uint32_t loop_count;
//    uint32_t ttw_skip_count;
    uint32_t ttw;
    uint32_t time_start;    
    uint32_t timeFromLast;
    uint32_t remains;
    uint32_t quant;
    uint8_t  task_id;
    uint8_t  prio;
    uint8_t  active;
} revo_sched_log;

#define SHED_DEBUG_SIZE 512
#endif

enum Revo_IO_Flags {
    IO_PERIODIC,
    IO_ONCE,
};

typedef struct REVO_IO {
    Handler h;
    Revo_IO_Flags flags;
} Revo_IO;

class REVOMINI::REVOMINIScheduler : public AP_HAL::Scheduler {
public:
  /**
   * Task run-time structure.
   */

    typedef struct IO_COMPLETION {
        Handler handler;
        REVOMINI::Semaphore *sem;
        bool request; 
#ifdef SHED_PROF
        uint64_t time;
        uint32_t count;
#endif
    } IO_Completion;



    REVOMINIScheduler();
    void     init();
    void     delay(uint16_t ms) { _delay(ms); } // uses internal static methods
    void     delay_microseconds(uint16_t us) { _delay_microseconds(us); }
    void     delay_microseconds_boost(uint16_t us) override { _delay_microseconds_boost(us); }
    
    inline   uint32_t millis() { yield(0);   return _millis(); } // this allows to run io_proc without calls to delay()
    inline   uint32_t micros() { yield(0);   return _micros(); }
    
    void     register_timer_process(AP_HAL::MemberProc proc) { _register_timer_process(proc, 1000); }
    inline void  suspend_timer_procs(){     _timer_suspended = true; }

    void     register_delay_callback(AP_HAL::Proc, uint16_t min_time_ms);
    static void  _register_io_process(Handler h, Revo_IO_Flags flags);
    void          register_io_process(AP_HAL::MemberProc proc) { Revo_handler h = { .mp=proc }; _register_io_process(h.h, IO_PERIODIC); }

    void     resume_timer_procs();


    static inline void     _register_timer_process(AP_HAL::MemberProc proc, uint32_t period) {
        Revo_cb r = { .mp=proc };

        _register_timer_task(period, r.h, NULL, CB_MEMBERPROC);
    }
    
    static void reschedule_proc(uint64_t proc);

    inline bool in_timerprocess() {   return _in_timer_proc; }

    static inline bool _in_timerprocess() {   return _in_timer_proc; }

    void     register_timer_failsafe(AP_HAL::Proc failsafe, uint32_t period_us) {   /* XXX Assert period_us == 1000 */  _failsafe = failsafe; }

    void     system_initialized();

    static void _reboot(bool hold_in_bootloader);
    void     reboot(bool hold_in_bootloader);

// drivers are not the best place for its own sheduler
    static AP_HAL::Device::PeriodicHandle register_timer_task(uint32_t period_us, AP_HAL::Device::PeriodicCb proc, REVOMINI::Semaphore *sem) {
        Revo_cb r = { .pcb=proc };
        return _register_timer_task(period_us, r.h, sem, CB_PERIODIC);
    }

    static AP_HAL::Device::PeriodicHandle register_timer_task(uint32_t period_us, AP_HAL::Device::PeriodicCbBool proc, REVOMINI::Semaphore *sem) {
        Revo_cb r = { .pcbb=proc };
        return _register_timer_task(period_us, r.h, sem, CB_PERIODICBOOL);
    }

    static void set_checked_semaphore(AP_HAL::Device::PeriodicHandle h, REVOMINI::Semaphore *sem);
    

/*
    try to get a semaphore and call handler on success
*/
    static void do_at_next_tick(Handler h, REVOMINI::Semaphore *sem);

    static void _delay(uint16_t ms);
    static void _delay_microseconds(uint16_t us);
    static void _delay_microseconds_boost(uint16_t us);

    static void _delay_us_ny(uint16_t us); // no yield delay

    static inline  uint32_t _millis() {    return systick_uptime(); } //systick_uptime returns 64-bit time
    static inline  uint64_t _millis64() {  return systick_uptime(); }

    static inline  uint32_t _micros() {   /* return systick_micros();*/ return timer_get_count32(TIMER5); }
    static         uint64_t _micros64(); 

    
    static bool           adjust_timer_task(AP_HAL::Device::PeriodicHandle h, uint32_t period_us);
    static bool           unregister_timer_task(AP_HAL::Device::PeriodicHandle h);
    void                  loop();      // to add ability to print out scheduler's stats in main thread


//    bool                  _run_1khz_procs();
    static inline bool in_interrupt(){ return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) /* || (__get_BASEPRI()) */; }


//{ this functions do a cooperative/preemptive multitask and inspired by Arduino-Scheduler (Mikael Patel), and scmrtos
    
  /**
   * Initiate scheduler and main task with given stack size. Should
   * be called before start of any tasks if the main task requires a
   * stack size other than the default size. Returns true if
   * successful otherwise false.
   * @param[in] stackSize in bytes.
   * @return bool.
   */
    static bool adjust_stack(size_t stackSize);
    
  /**
   * Start a task with given function and stack size. Should be
   * called from main task. The functions are executed by the
   * task. The taskLoop function is repeatedly called. Returns 
   * true if successful otherwise false (no memory for new task).
   * @param[in] taskSetup function (may be NULL).
   * @param[in] taskLoop function (may not be NULL).
   * @param[in] stackSize in bytes.
   * @return bool.
   */
  static void * _start_task(Handler h,  size_t stackSize);

  static inline void * start_task(voidFuncPtr taskLoop, size_t stackSize = DEFAULT_STACK_SIZE){
        Revo_handler r = { .vp=taskLoop };
        return _start_task(r.h, stackSize);
  }
  static inline void * start_task(AP_HAL::MemberProc proc,  size_t stackSize = DEFAULT_STACK_SIZE){
        Revo_handler r = { .mp=proc };
        return _start_task(r.h, stackSize);
  }
  
  // functions to alter task's properties
  static void set_task_period(void *h, uint32_t period);
  static void set_task_semaphore(void *h, REVOMINI::Semaphore *sem);
  static void set_task_ttw(void *h, uint32_t ttw);
  static void set_task_priority(void *h, uint8_t prio);
  static void inline set_task_ioc(bool v) { s_running->in_ioc=v; }
  static void inline set_task_active(void *h) {   task_t * task = (task_t*)h; task->active=true; }
  
  static void stop_task(void * h);
  static inline void * get_current_task() { return s_running; }

    /*
        task scheduler. Gives task ready to run with highest priority
    */
  static task_t *get_next_task(); 

  // informs that task owns semaphore so should have priority increase
  static inline void task_has_semaphore(bool v) { 
    task_t * curr_task = s_running;
    noInterrupts();
    if(v) {
        curr_task->has_semaphore++; 
    } else  if(curr_task->has_semaphore) {
        curr_task->has_semaphore--; 
    }
    interrupts();
  }

  // allows to block task on semaphore
  static inline void task_want_semaphore(void * _task, REVOMINI::Semaphore *sem) { 
    task_t * task = (task_t*)_task;
    task_t * curr_task = s_running;
    
    //Increase the priority of the semaphore's owner up to the priority of the current task
    if(task->priority < curr_task->priority) task->curr_prio = curr_task->priority;
    curr_task->sem_wait = sem;
  }

  /**               
   * Context switch to next task in run queue.
   */
  static void yield(uint16_t ttw=0); // optional time to wait
  
  /**
   * Return current task stack size.
   * @return bytes
   */
  static size_t task_stack();
  
  // check from what task it called
  static bool is_main_task();
//}


    static inline void register_IMU_handler(AP_HAL::MemberProc proc) {
        Revo_handler h = { .mp=proc };
        REVOMINIGPIO::_attach_interrupt(BOARD_MPU6000_DRDY_PIN, h.h, RISING, 11);
    }

//{ IO completion routines

    #define MAX_IO_COMPLETION 8
    
    typedef voidFuncPtr ioc_proc;

    static uint8_t register_io_completion(uint64_t handle);

    static inline uint8_t register_io_completion(ioc_proc cb) {
        Revo_handler r = { .vp=cb };
        return register_io_completion(r.h);
    }
    static inline uint8_t register_io_completion(AP_HAL::MemberProc proc) {
        Revo_handler r = { .mp=proc };
        return register_io_completion(r.h);
    }

    static inline void set_io_completion_sem(uint8_t id, REVOMINI::Semaphore *sem) {
        IO_Completion &ioc = io_completion[id-1];
        ioc.sem = sem;
    }

    static inline void do_io_completion(uint8_t id){ // schedule selected IO completion
        if(id) io_completion[id-1].request = true;
        need_io_completion=true;
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; // PENDSVSET
    }

    static void PendSV_Handler();
    static void SVC_Handler();
    static volatile bool need_io_completion;

#ifdef PREEMPTIVE
    static volatile bool need_switch_task; // should be public
#endif
//}


    static inline Handler get_handler(AP_HAL::MemberProc proc){
        Revo_handler h = { .mp = proc };
        return h.h;
    }
    static inline Handler get_handler(AP_HAL::Proc proc){
        Revo_handler h = { .hp = proc };
        return h.h;
    }
        
    static inline void setEmergencyHandler(voidFuncPtr handler) { boardEmergencyHandler = handler; }

    static inline void i_know_new_api() { new_api_flag=true; }
    
    static inline void MPU_buffer_overflow(){ MPU_overflow_cnt++; } 
    static inline void MPU_restarted() {      MPU_restart_cnt++; }
    static inline void MPU_stats(uint16_t count, uint32_t time) {
        if(count>MPU_count) {
            MPU_count=count;
            MPU_Time=time;
        }
    }
    
    static inline void arming_state_changed(bool v){ if(!v && on_disarm_handler) revo_call_handler(on_disarm_handler, 0); }
    static inline void register_on_disarm(Handler h){ on_disarm_handler=h; }

protected:

//{ multitask
    // executor for task's handler
    static void do_task(task_t * task);
    // gves first deleted task or NULL
    static task_t* get_empty_task();
/**
   * Initiate a task with the given functions and stack. When control
   * is yield to the task then the loop function is repeatedly called.
   * @param[in] h     task handler (may not be NULL).
   * @param[in] stack top reference.
   */
    static void *init_task(uint64_t h, const uint8_t* stack);

    // prepares TCB
    static uint32_t fill_task(task_t &tp);

    /** Task stack allocation top. */
    static size_t s_top;
  
    static uint16_t task_n; // counter of tasks
  
    static void check_stack(uint32_t sp);

 
#define await(cond) while(!(cond)) yield()
  
//} end of multitask
    
private:
    static AP_HAL::Device::PeriodicHandle _register_timer_task(uint32_t period_us, uint64_t proc, REVOMINI::Semaphore *sem, revo_cb_type mode=CB_MEMBERPROC);

    static volatile bool _in_timer_proc;

    static AP_HAL::Proc _delay_cb;
    static void * _delay_cb_handle;
    static uint16_t _min_delay_cb_ms;
    static bool _initialized;

    /* _timer_isr_event() and _run_timer_procs are static so they can be
     * called from an interrupt. */
    static void _timer_isr_event(uint32_t v /*TIM_TypeDef *tim */);
    static void _timer5_ovf(uint32_t v /*TIM_TypeDef *tim */ );
    static void _tail_timer_event(uint32_t v /*TIM_TypeDef *tim */);
    
    static void _run_timer_procs(bool called_from_isr);


    static uint32_t timer5_ovf_cnt;
    
    static AP_HAL::Proc _failsafe;

    static volatile bool _timer_suspended;
    static volatile bool _timer_event_missed;
    static uint32_t _scheduler_last_call;

    static Revo_IO _io_proc[REVOMINI_SCHEDULER_MAX_IO_PROCS];
    static uint8_t _num_io_proc;

#if USE_ISR_SCHED
    static revo_timer _timers[REVOMINI_SCHEDULER_MAX_SHEDULED_PROCS];
    static uint8_t    _num_timers;
    static void _run_timers(void);
    
    static revo_tick _ticks[REVOMINI_SCHEDULER_MAX_SHEDULED_PROCS];
    static uint8_t   _num_ticks;
#endif
    static void _run_io(void);

    void _print_stats();
    void stats_proc(void);
    
#ifdef SHED_PROF
    static uint64_t shed_time;
    static uint64_t task_time;
    static bool flag_10s;
    
    static uint64_t delay_time;
    static uint64_t delay_int_time;
    static uint32_t max_loop_time;
    
    bool _set_10s_flag();
    static uint64_t ioc_time;
    static uint64_t sleep_time;
    static uint32_t max_delay_err;


    static uint32_t tick_micros;    // max exec time
    static uint32_t tick_count;     // number of calls
    static uint64_t tick_fulltime;  // full consumed time to calc mean

#endif

#ifdef MTASK_PROF
    static uint32_t max_wfe_time;
    static uint64_t tsched_time;
    static uint32_t tsched_count;
    static uint32_t tsched_sw_count;
    static uint64_t tsched_time_y;
    static uint32_t tsched_count_y;
    static uint32_t tsched_sw_count_y;
    static uint64_t tsched_time_t;
    static uint32_t tsched_count_t;
    static uint32_t tsched_sw_count_t;


 #ifdef SHED_DEBUG
    static revo_sched_log logbuf[SHED_DEBUG_SIZE];
    static uint16_t sched_log_ptr;
 #endif
#endif


    static uint32_t lowest_stack;
    static uint32_t main_stack;
    static bool disable_stack_check;
    static uint32_t max_stack_pc;

    
    static struct IO_COMPLETION io_completion[MAX_IO_COMPLETION];

    static uint8_t num_io_completion;
    static bool _in_io_proc;

    static bool new_api_flag;
    static uint32_t MPU_overflow_cnt;
    static uint32_t MPU_restart_cnt;
    static uint32_t MPU_count;
    static uint32_t MPU_Time;
    
    
    static Handler on_disarm_handler;
};

void revo_call_handler(Handler h, uint32_t arg);

#endif // __AP_HAL_REVOMINI_SCHEDULER_H__
