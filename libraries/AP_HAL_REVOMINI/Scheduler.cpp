#pragma GCC optimize ("O2")


#include "Scheduler.h"

#include <AP_HAL_REVOMINI/AP_HAL_REVOMINI.h>


#include "Semaphores.h"
#include "I2CDevice.h"

#include <timer.h>


#include <AP_Notify/AP_Notify.h>
#include <AP_Math/AP_Math.h>

#include <systick.h>

#include "GPIO.h"

#include <usb.h>


/*

stats for Copter

Scheduler stats:

  % of full time: 26.79  Efficiency 0.924 max loop time 2904 
delay times: in main 85.83 including in semaphore  0.00  in timer  7.71 in isr  0.00 

Task times:
task 0x809966920008114 tim      0.0 int 0.000% tot 0.0000% mean time   1.0 max time 1
task 0x809A8252000811C tim    493.0 int 3.680% tot 0.9859% mean time   9.9 max time 22
task 0x809376D20007FE0 tim      2.6 int 0.019% tot 0.0051% mean time   1.0 max time 3
task 0x804667520009F68 tim   2167.8 int 16.180% tot 4.3357% mean time 436.9 max time 452
task 0x8048D7D2000A480 tim   2996.4 int 22.365% tot 5.9929% mean time 852.0 max time 864
task 0x80509812000A4E0 tim   7738.1 int 57.756% tot 15.4763% mean time 172.4 max time 2462

*/

using namespace REVOMINI;
extern const AP_HAL::HAL& hal;

#define ADDRESS_IN_FLASH(a) ((a)>FLASH_BASE && (a)<CCMDATARAM_BASE)


AP_HAL::Proc REVOMINIScheduler::_failsafe = NULL;
volatile bool REVOMINIScheduler::_timer_suspended = false;
volatile bool REVOMINIScheduler::_timer_event_missed = false;
volatile bool REVOMINIScheduler::_in_timer_proc = false;

revo_timer REVOMINIScheduler::_timers[REVOMINI_SCHEDULER_MAX_SHEDULED_PROCS] IN_CCM;
uint8_t    REVOMINIScheduler::_num_timers = 0;

uint8_t    REVOMINIScheduler::_num_io_proc=0;


AP_HAL::Proc REVOMINIScheduler::_delay_cb=NULL;
uint16_t REVOMINIScheduler::_min_delay_cb_ms=0;
void *   REVOMINIScheduler::_delay_cb_handle=0;

uint32_t REVOMINIScheduler::timer5_ovf_cnt=0;

bool REVOMINIScheduler::_initialized=false;


static void loc_ret(){}

#define STACK_GUARD 0x60a4d51aL

// Main task and run queue
static REVOMINIScheduler::task_t s_main = { 0 };


struct REVOMINIScheduler::IO_COMPLETION REVOMINIScheduler::io_completion[MAX_IO_COMPLETION] IN_CCM;
uint8_t REVOMINIScheduler::num_io_completion = 0;

// Reference running task
REVOMINIScheduler::task_t* REVOMINIScheduler::s_running IN_CCM; //  = &s_main; - CCM don't initialized!

// Initial top stack for task allocation
size_t REVOMINIScheduler::s_top IN_CCM; //  = MAIN_STACK_SIZE; - CCM not initialized!

uint16_t REVOMINIScheduler::task_n=0;

#ifdef SHED_PROF
uint64_t REVOMINIScheduler::shed_time = 0;
bool     REVOMINIScheduler::flag_10s = false;
uint64_t REVOMINIScheduler::task_time IN_CCM = 0;
uint64_t REVOMINIScheduler::delay_time IN_CCM = 0;
uint64_t REVOMINIScheduler::delay_int_time IN_CCM = 0;
uint32_t REVOMINIScheduler::max_loop_time=0;
#endif


#ifdef MTASK_PROF
 uint64_t REVOMINIScheduler::yield_time IN_CCM = 0;
 uint32_t REVOMINIScheduler::yield_count IN_CCM =0;
#endif

uint32_t REVOMINIScheduler::lowest_stack = (uint32_t)-1;
uint32_t REVOMINIScheduler::main_stack   = (uint32_t)-1;
uint32_t REVOMINIScheduler::max_stack_pc;
bool REVOMINIScheduler::disable_stack_check=false;


REVOMINIScheduler::REVOMINIScheduler()
{

    s_running = &s_main; //  CCM don't initialized! - Reference running task
    s_top = MAIN_STACK_SIZE;                       // Initial top stack for task allocation

    memset(&s_main,0, sizeof(s_main));

    Revo_handler h = { .vp=loc_ret };

    s_main.next = &s_main,
    s_main.prev = &s_main,
//    s_main.active = true,
    s_main.handle = h.h;
    s_main.guard = STACK_GUARD,

#ifdef MTASK_PROF
    s_main.start=_micros();
#endif

}

bool REVOMINIScheduler::is_main_task() { return s_running == &s_main; }

void REVOMINIScheduler::init()
{

    memset(_timers,       0, sizeof(_timers) );
    memset(io_completion, 0, sizeof(io_completion) );

    CLEAR_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPDEEP_Msk)); //we don't need deep sleep
    SET_BIT(  SCB->SCR, ((uint32_t)SCB_SCR_SEVONPEND_Msk)); //we need Event on each interrupt


    timer_foreach(timer_reset); // timer_reset(dev) moved out from configTimeBase so reset by hands

    uint32_t period    = (2000000UL / SHED_FREQ) - 1; 
    
                // dev    period   freq, kHz
    configTimeBase(TIMER7, period, 2000);       //2MHz 0.5us ticks
    timer_attach_interrupt(TIMER7, TIMER_UPDATE_INTERRUPT, _timer_isr_event, 11); // low priority - only PendSV and USB are lower
    timer_resume(TIMER7);

// timer5 - 32-bit general timer, unused for other needs
// so we can read micros32() directly from its counter and micros64() from counter and overflows
    configTimeBase(TIMER5, 0, 1000);       //1MHz 1us ticks
    timer_set_count(TIMER5,(1000000/SHED_FREQ)/2); // to not interfere with TIMER7
    timer_attach_interrupt(TIMER5, TIMER_UPDATE_INTERRUPT, _timer5_ovf, 2); // high priority
    timer_resume(TIMER5);


    // only Timer6 from spare timers has personal NVIC line - TIM6_DAC_IRQn
    uint32_t freq = configTimeBase(TIMER6, 0, 20000);       //20MHz - we here don't know real freq so can't set period
    timer_set_reload(TIMER6, freq / 1000000);             // period to generate 1uS requests
    timer_enable_irq(TIMER6, TIMER_UPDATE_INTERRUPT); // enable interrupt requests from timer but not enable them in NVIC - will be events
    timer_resume(TIMER6);
    
    
#ifdef SHED_PROF
// set flag for stats output each 10 seconds
    register_timer_task(10000000, FUNCTOR_BIND_MEMBER(&REVOMINIScheduler::_set_10s_flag, bool), NULL);
#endif

// for sheduler debug
//    register_io_process(FUNCTOR_BIND_MEMBER(&REVOMINIScheduler::stats_proc, void) );

}

void REVOMINIScheduler::_delay(uint16_t ms)
{
    uint32_t start = _micros();
#ifdef SHED_PROF
    uint32_t t=start;
#endif
    
    while (ms > 0) {
//        if(!_in_timer_proc)  // not switch context in interrupts -- yield() checks it itself
            yield(ms*1000); // time in micros
            
        while ((_micros() - start) >= 1000) {
            ms--;
            if (ms == 0) break;
            start += 1000;
        }
        if (_min_delay_cb_ms <= ms) { // MAVlink callback uses 5ms
            if (_delay_cb) {
                _delay_cb();
            }
        }
    }

#ifdef SHED_PROF
    uint32_t us=_micros()-t;
    if(_in_timer_proc)
        delay_int_time +=us;
    else
        delay_time     +=us;
#endif
}

void REVOMINIScheduler::_delay_microseconds_boost(uint16_t us){
    _delay_microseconds(us);
}

void REVOMINIScheduler::_delay_microseconds(uint16_t us)
{
#ifdef SHED_PROF
    uint32_t t = _micros(); 
#endif

    uint32_t rtime = stopwatch_getticks(); // start ticks
    uint32_t dt    = us_ticks * us;  // delay time in ticks

    uint32_t ny = 3 * us_ticks; // no-yield time 3 uS in ticks
    uint32_t tw;

    while ((tw = stopwatch_getticks() - rtime) < dt) { // tw - time waiting, in ticks
        if((dt - tw) > ny ) { // No Yeld time - 3uS to end of wait 
            yield((dt - tw) / us_ticks); // in micros
        }
    }    

#ifdef SHED_PROF
    us=_micros()-t; // real time
    
    if(_in_timer_proc)
        delay_int_time +=us;
    else
        delay_time     +=us;
#endif

}


void REVOMINIScheduler::register_delay_callback(AP_HAL::Proc proc, uint16_t min_time_ms)
{
    static bool init_done=false;
    if(!init_done){     // small hack to load HAL parameters in needed time

        ((HAL_REVOMINI&) hal).lateInit();
        
        init_done=true;
    }

    _delay_cb        = proc;
    _min_delay_cb_ms = min_time_ms;


/* 
1 - it should run in delay() only 
2 - it should be removed after init done
    if(proc) {
        _delay_cb_handle = start_task(proc);
    } else {
        stop_task(_delay_cb_handle);
    }
*/
}



void REVOMINIScheduler::register_io_process(AP_HAL::MemberProc proc)
{
    if(_num_io_proc>=REVOMINI_SCHEDULER_MAX_IO_PROCS) return;

    if(start_task(proc)) {
        _num_io_proc++;
    }
}



void REVOMINIScheduler::resume_timer_procs()
{
    _timer_suspended = false;
    if (_timer_event_missed == true) {
        _run_timer_procs(false);        // TODO here code executes on main thread, not in interrupt level!
        _timer_event_missed = false;
    }
}

void REVOMINIScheduler::check_stack(uint32_t sp) { // check for stack usage
    
    uint32_t * stack = (uint32_t *)sp;

    // Stack frame contains:
    // r0, r1, r2, r3, r12, r14, the return address and xPSR
    // - Stacked R0  = stack[0]
    // - Stacked R1  = stack[1]
    // - Stacked R2  = stack[2]
    // - Stacked R3  = stack[3]
    // - Stacked R12 = stack[4]
    // - Stacked LR  = stack[5]
    // - Stacked PC  = stack[6]
    // - Stacked xPSR= stack[7]
    
    if(disable_stack_check) return;
    
    if(is_main_task()){
        if(sp<main_stack) { main_stack=sp; max_stack_pc = stack[10]; }
    }else {
        if(sp<lowest_stack){ lowest_stack=sp; }
    }

}

void REVOMINIScheduler::_run_timer_procs(bool called_from_isr) {

    if (_in_timer_proc) {
        return;
    }
    _in_timer_proc = true;


    if (!_timer_suspended) {
        _run_timers(); 
    } else if (called_from_isr) {
        _timer_event_missed = true;
    }

    // and the failsafe, if one is setted 
    if (_failsafe) {
        static uint32_t last_failsafe=0;
        uint32_t t=_millis();
        if(t>last_failsafe){
            last_failsafe = t+10; // 10ms = 100Hz
            _failsafe();
        }
    }

    _in_timer_proc = false;
}

void REVOMINIScheduler::_timer_isr_event(TIM_TypeDef *tim) {
    uint32_t sp; 

 // Get stack pointer, assuming we the thread that generated
    // the svc call was using the psp stack instead of msp
    asm volatile ("mov %0, sp\n\t"  : "=rm" (sp) );

    check_stack(sp);
    
    _run_timer_procs(true);
}

void REVOMINIScheduler::_timer5_ovf(TIM_TypeDef *tim) {
    timer5_ovf_cnt++;
}

uint64_t REVOMINIScheduler::_micros64() {
#pragma pack(push, 1)
    union {
        uint64_t t;
        uint32_t w[2];
    } now;
#pragma pack(pop)

    noInterrupts();
    now.w[0] = _micros();
    now.w[1] = timer5_ovf_cnt;
    interrupts();
    return now.t;
}


void REVOMINIScheduler::system_initialized()
{
    if (_initialized) {
        AP_HAL::panic("PANIC: scheduler::system_initialized called more than once");
    }
    _initialized = true;
    
    board_set_rtc_register(0,RTC_SIGNATURE_REG); // clear bootloader flag after init done
}


void REVOMINIScheduler::_reboot(bool hold_in_bootloader) {

    if(hold_in_bootloader) {
#if 1
        if(is_bare_metal()) { // bare metal build without bootloader

            board_set_rtc_register(DFU_RTC_SIGNATURE, RTC_SIGNATURE_REG);

        } else
#endif
            board_set_rtc_register(BOOT_RTC_SIGNATURE, RTC_SIGNATURE_REG);
    }

    _delay(100);

    NVIC_SystemReset();

    _delay(1000);
}


void REVOMINIScheduler::reboot(bool hold_in_bootloader) {
    
    hal.console->println("GOING DOWN FOR A REBOOT\r\n");
    _reboot(hold_in_bootloader);
}

void REVOMINIScheduler::loop(){    // executes in main thread

     _print_stats();
}

void REVOMINIScheduler::stats_proc(void){
//    _print_stats(); only for debug

}

void REVOMINIScheduler::_print_stats(){
    static int cnt=0;
    

    if(flag_10s) {
        flag_10s=false;

        
        uint32_t t=_millis();
        const int Kf=100;
        
        switch(cnt++) {
        
        case 0:{
#ifdef SHED_PROF
            float eff= (task_time)/(float)(task_time+shed_time);

            static float shed_eff=0;

            if(is_zero(shed_eff)) shed_eff = eff;
            else              shed_eff = shed_eff*(1 - 1/Kf) + eff*(1/Kf);

            hal.console->printf("\nSched stats:\n  %% of full time: %5.2f  Efficiency %5.3f max loop time %ld \n", (task_time/10.0)/t /* in percent*/ , shed_eff, max_loop_time );
            hal.console->printf("delay times: in main %5.2f including in semaphore %5.2f  in timer %5.2f",         (delay_time/10.0)/t, (Semaphore::sem_time/10.0)/t,  (delay_int_time/10.0)/t);

#ifdef ISR_PROF
            hal.console->printf("in isr %5.2f max %5.2f", (isr_time/10.0/(float)us_ticks)/t, max_isr_time/(float)us_ticks );
#endif
#if 0
            hal.console->printf("\nIMU times: mean %5.2f max %5ld", (float)_IMU_fulltime/_IMU_count, _IMU_maxtime );
#endif
        } break;

        case 1:{

            hal.console->printf("\nTask times:\n");

            for(int i=0; i< _num_timers; i++) {
                if(_timers[i].proc){    // task not cancelled?
                    hal.console->printf("task 0x%llX tim %8.1f int %5.3f%% tot %6.4f%% mean time %5.1f max time %ld\n", _timers[i].proc, _timers[i].fulltime/1000.0, _timers[i].fulltime*100.0 / task_time, (_timers[i].fulltime / 10.0) / t, (float)_timers[i].fulltime/_timers[i].count, _timers[i].micros );
                    _timers[i].micros = 0; // reset max time
                }
            }
#endif
    
            }break;
            
        case 2:{
#ifdef MTASK_PROF    
            task_t* ptr = &s_main;

            hal.console->printf("\ntask switch time %7.3fms count %ld mean %6.3fuS\n", yield_time/(float)us_ticks/1000.0, yield_count, yield_time /(float)us_ticks / (float)yield_count );
        
            do {
                hal.console->printf("task %d times: full %8.1fms (%7.2f%%) max %lduS ad %lx\n",  ptr->id, ptr->time/1000.0, 100.0 * ptr->time/1000.0 / t, ptr->max_time, ptr->maxt_addr );
        
                ptr->max_time=0; // reset max time
                
                ptr = ptr->next;
            } while(ptr != &s_main);
#endif
            }break;

        case 3: {
            uint8_t n = REVOI2CDevice::get_dev_count();
            hal.console->printf("\nI2C stats\n");
    
            for(uint8_t i=0; i<n; i++){
                REVOI2CDevice * d = REVOI2CDevice::get_device(i);
                if(d){
                    hal.console->printf("bus %d addr %x errors %ld \n",d->get_bus(), d->get_addr(), d->get_error_count());   
                }
            }
            }break;

        case 4: {
            uint32_t heap_ptr = (uint32_t)__brkval; // here should be upper bound of sbrk()
            uint32_t bottom=(uint32_t)&_sdata;
            
            // 48K after boot 
            hal.console->printf("\nMemory used: %ldk:\n",(heap_ptr-bottom)/1024);
            hal.console->printf("Free stack: %ldk:\n",(lowest_stack - (uint32_t)&_eccm)/1024);
            hal.console->printf("Main stack use: %ldk at %lx\n",((uint32_t)&_sccm + 0x10000 /* 64K CCM */ - main_stack)/1024, max_stack_pc);

            } break;
        
        case 5: {
            hal.console->printf("\nIO completion sats\n");
            for(uint8_t i=0; i<num_io_completion; i++){
                struct IO_COMPLETION &io = io_completion[i];
                
                if(io.handler)
                    hal.console->printf("task %llx time %9.1fms (%7.3f%%)\n", io.handler,  io.time/1000.0, 100.0 * io.time / t / 1000);
                
            }
        
            }break;
            
        case 6:
        default:
            cnt=0;
            break;
        }
    }
}


#ifdef SHED_PROF
bool REVOMINIScheduler::_set_10s_flag(){
    flag_10s=true;
    return true;
}
#endif

/*
[    common realization of all Device.PeriodicCallback;
*/
AP_HAL::Device::PeriodicHandle REVOMINIScheduler::_register_timer_task(uint32_t period_us, Handler proc, REVOMINI::Semaphore *sem, revo_cb_type mode){
    uint8_t i;
    

#if 0 // это бесполезно, ибо переход к io_completion происходит без выполнения основной программы

    uint8_t ioc=0;

    if(period_us > 8000) { // slow tasks will run at IO_Completion level
        ioc=register_io_completion(proc);
        set_io_completion_sem(ioc, sem);
    }
#endif

#if 1
    if(period_us > 8000) { // slow tasks will run at individual IO tasks
        void *task = _start_task(proc, SLOW_TASK_STACK);
        set_task_period(task, period_us);
        set_task_semaphore(task, sem);
        return NULL;
    }
#endif

    for (i = 0; i < _num_timers; i++) {
        if ( _timers[i].proc == 0L /* free slot */ ) {
            goto store;        

        } else if (_timers[i].proc == proc /* the same */ ) {
            noInterrupts();            // 64-bits should be 
            _timers[i].proc = 0L; // clear proc - temporary disable task
            interrupts();
            goto store;
        }
    }

    if (_num_timers < REVOMINI_SCHEDULER_MAX_SHEDULED_PROCS) {
        /* this write to _timers[] can be outside the critical section
         * because that memory won't be used until _num_timers is
         * incremented or where proc is NULL. */
         
        i = _num_timers;

        _timers[i].proc = 0L; // clear proc - this entry will be skipped
        _num_timers++; // now nulled proc guards us
store:        
        revo_timer &rt=_timers[i];
        rt.period = period_us;
        rt.last_run = _micros(); // now
        rt.sem  = sem;
        rt.mode = mode;
#if 0
        rt.ioc = ioc;
#endif
#ifdef SHED_PROF
        rt.count = 0;
        rt.micros = 0;
        rt.fulltime = 0;
#endif
        noInterrupts();            // 64-bits should be 
        rt.proc = proc;    //     last one, not interferes - guard is over
        interrupts();
        return (AP_HAL::Device::PeriodicHandle)&rt;
    }

    return NULL;
}


bool REVOMINIScheduler::adjust_timer_task(AP_HAL::Device::PeriodicHandle h, uint32_t period_us)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align" // yes I know

    revo_timer *p = (revo_timer *)h;
#pragma GCC diagnostic pop
    p->period = period_us;
    
    return true;
}
bool REVOMINIScheduler::unregister_timer_task(AP_HAL::Device::PeriodicHandle h)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
    revo_timer *p = (revo_timer *)h;
#pragma GCC diagnostic pop

    noInterrupts(); // 64-bits should be 
    p->proc=0L;
    interrupts();
    return true;
}

void REVOMINIScheduler::reschedule_proc(uint64_t proc){
    for (uint8_t i = 0; i < _num_timers; i++) {
        revo_timer &tim = _timers[i];
        if (tim.proc == proc /* the same */ ) {
            tim.last_run    -= tim.period; // move back
            return;
        }
    }
}


#define TIMER_PERIOD (1000000 / SHED_FREQ)  //125  interrupts period in uS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

void REVOMINIScheduler::_run_timers(){
    uint32_t now = _micros();
    static uint32_t last_run = 0;

    uint32_t full_t = now;
#ifdef SHED_PROF
    uint32_t job_t = 0;
#endif                

    volatile uint32_t dt = now - last_run; // time from last run - just for debug

    last_run = now;

    bool is_error = Semaphore::get_error(); // reset error before tasks

    for(int i = 0; i<_num_timers; i++){
        revo_timer &tim = _timers[i];
        
        if(tim.proc){    // task not cancelled?
/*
    у нас время в 32-разрядном счетчике, который может переполниться
    now - tim.last_run - время с момента прошлого запуска, с учетом всех переполнений
    
*/
            if( (now - tim.last_run) > tim.period) { // time to run?
                uint8_t ret=1;  // OK by default

#if 0 
                if(tim.ioc) { // slow tasks runs at IO_completion level
                    do_io_completion(tim.ioc);
                } else 
#endif
                {         // fast tasks runs here
                    if(tim.sem && !tim.sem->take_nonblocking()) { // semaphore active? take!
                        // can't get semaphore, just do nothing - will try next time
                        continue;
                    }
#ifdef SHED_PROF
                    uint32_t t = _micros();
#endif          
                    Revo_cb r = { .h=tim.proc }; // don't touch it without hardware debugger!

                    switch(tim.mode){
                    case CB_PERIODIC:
                        (r.pcb)();              // call task
                        break;
                    case CB_PERIODICBOOL:
                        ret = (r.pcbb)();       // call task
                        break;
                    case CB_MEMBERPROC:
                        (r.mp)();              // call task
                        break;
                    }
                    if(tim.sem) tim.sem->give(); //  semaphore active? give back ASAP!
                
                    is_error = Semaphore::get_error(); // was there semaphore errors in this task?
                
                    if(is_error) ret=0; // bad result if was any, need to reschedule

                    now = _micros();
#ifdef SHED_PROF
                    t = now - t;               // work time

                    if(tim.micros < t)
                        tim.micros    =  t;      // max time
                    tim.count     += 1;          // number of calls
                    tim.fulltime  += t;          // full time, mean time = full / count
                    job_t += t;                  // time of all jobs
#endif        
                }
                
                if(ret) {  // ok?
                    tim.last_run    += tim.period; // прошлое время запуска - по надобности а не по факту
                } else {// reschedule
                }

            }
        }
    }


    full_t = _micros() - full_t;         // full time of scheduler
// исключить время работы прерывания из времени задачи, которую оно прервало
    s_running->in_isr += full_t;

#ifdef SHED_PROF
    uint32_t shed_t = full_t - job_t;   // net time

    if(full_t>max_loop_time)
        max_loop_time=full_t;

    task_time += job_t; // full time in tasks
    shed_time += shed_t;
    
#endif                

}
#pragma GCC diagnostic pop

// ]


//[ -------- realization of cooperative multitasking --------

bool REVOMINIScheduler::adjust_stack(size_t stackSize)
{  // Set main task stack size
  s_top = stackSize;
  return true;
}


// Add task last to run queue
uint32_t REVOMINIScheduler::fill_task(task_t &tp){
    memset(&tp,0,sizeof(tp));

    tp.next = &s_main;  // linked list
    tp.prev = s_main.prev;
    s_main.prev->next = &tp;
    s_main.prev = &tp;
    
    tp.id = ++task_n; // counter
//    tp.active = true;
    tp.ttw = 1;       // delay after 1st enter
//    tp.max_delay=0;   // max execution time

#ifdef MTASK_PROF
    tp.start=_micros(); 
#endif

    tp.guard = STACK_GUARD;

    return (uint32_t)&tp; // только финт с возвратом из функции снимает проклятье "локальный адрес"
}

#if 0 // однажды назначенные задачи никто не отменяет

REVOMINIScheduler::task_t* REVOMINIScheduler::get_empty_task(){
    task_t* ptr = &s_main;

    do {
        if(ptr->handler == NULL)  return ptr;
        
        ptr = ptr->next;
    } while(ptr != &s_main);

    return NULL;
}

void REVOMINIScheduler::stop_task(void *h){
    if(h) {
        task_t *tp = (task_t *)h ;
    
        tp->handle = 0;
    }
}

#endif

void * REVOMINIScheduler::init_task(Handler handler, const uint8_t* stack)
{
    task_t task;
    uint32_t ret=fill_task(task);  // Add task last in run queue (main task)
    task.stack = stack;
    task.handle = handler; // save handler to task to enable to change it later

    stack_bottom = (caddr_t)stack; // remember for memory allocator


    // Create context for new task, caller will return
    if (setjmp(task.context)) {
        // we comes via longjmp - the task itself
        while (1) {
            if(task.handle) revo_call_handler(task.handle, 0); 

            yield();        // in case that function not uses delay();
        }
    }
    // caller returns
//  return &task; GCC optimizes out so returns 0
    return (void *)ret;
}

// start C function as task
void * NOINLINE REVOMINIScheduler::_start_task(Handler handle, size_t stackSize)
{
    // Check called from main task and valid task loop function
    if (!is_main_task() ) return NULL;
    if ( !handle ) return NULL;

    // Adjust stack size with size of task context
    stackSize += sizeof(task_t);
    void * ret;

    disable_stack_check = true;

    { // isolate stack[]

        // Allocate stack(s) and check if main stack top should be set
        size_t frame = RAMEND - (size_t) &frame;
        volatile uint8_t stack[s_top - frame]; // should be volatile else it will be optimized out
        if (s_main.stack == NULL) s_main.stack = (const uint8_t*)stack; // remember on first call stack of main task

        // Check that the task can be allocated without already used CCM
        if ((s_top + stackSize) > (STACK_MAX       -     (&_eccm-&_sccm))) return NULL;

        // Adjust stack top for next task allocation
        s_top += stackSize;

        // Initiate task with stack top
        ret=init_task(handle, (const uint8_t*)(stack - stackSize));
    }
    disable_stack_check = false;
    return ret;
}

// task should run periodically, period in uS
void REVOMINIScheduler::set_task_period(void *h, uint32_t period){
    task_t *task = (task_t *)h;
    
    task->period = period;
    task->start  = _micros();
}

// task wants to run only with this semaphore
void REVOMINIScheduler::set_task_semaphore(void *h, REVOMINI::Semaphore *sem){
    task_t *task = (task_t *)h;
    
    task->sem = sem;
}


void REVOMINIScheduler::yield(uint16_t ttw) // time to wait 
{
    uint8_t ntask = task_n;
    if(ntask==0        || // no tasks
      (ttw && ttw < 5) || // don't mess into delays less than 5uS - 840 steps
      in_interrupt() ) { // don't switch privileged context

        __WFE();
        return;
    }

    disable_stack_check = true;

    { // isolate 'me' - task that calls yield()
        task_t *me = s_running;
    
        if(me->sem) me->sem->give();

// if yield() called with a time, then task don't want to run all this time and exclude it from time sliceing
        uint32_t t =  _micros();
        me->t_yield = t;
        me->ttw     = ttw; // remember that task want to wait

        uint32_t dt =  t - me->start;       // time in task
        if(dt >= me->in_isr) dt -= me->in_isr;  // minus time in interrupts
        else dt=0;
        if(dt > me->max_delay) me->max_delay = dt; // and remember maximum

#ifdef MTASK_PROF
        if(dt > me->max_time) {
            me->max_time = dt; // maximum to show
            me->maxt_addr = ((uint32_t *)(me->context))[8]; // LR of context - PC of process
        }
        me->time+=dt;                           // calculate sum
    
        uint32_t ticks = stopwatch_getticks();
#endif
        if (setjmp(me->context)) {
            // we come here via longjmp - context switch is over
#ifdef MTASK_PROF
            yield_time += stopwatch_getticks() - s_running->ticks; // time of longjmp
            yield_count++;                  // count each context switch
#endif
            disable_stack_check = false;
            return;
        }
        // begin of context switch
#ifdef MTASK_PROF
        yield_time += stopwatch_getticks()-ticks; // time of setjmp
#endif


        while(true) { // find task to switch to
            s_running = s_running->next; // Next task in run queue will continue
            if(s_running == 0) {
                s_running = &s_main; // in case of error
            }
            if(s_running == me) {  // 'me' is the task that calls yield(), so full loop - there is no job.
                __WFE(); //  Timer6 makes events each uS to not spoil microsecond delays
            }
    
            if(!s_running->handle) continue; // skip finished tasks
            
            uint32_t now= _micros();
            // если для задачи установлен период выполнения - проверим
            if(s_running->period){
                    // time from last run  less  than period
                if( (now-s_running->start)  <   s_running->period) continue;
            } else { // проверим задачи без строгого периода на допустимость
            
                // task has a ttw  and time since that moment still less than ttw - skip task
                if(s_running->ttw && (now-s_running->t_yield) < s_running->ttw) continue;

                   //     main task always    task max execution time more than we have
                if(ttw && s_running->id!=0 && s_running->max_delay > ttw) { 
                    s_running->max_delay --; //  понемногу уменьшаем дабы совсем не выключить
                    continue;
                }
            }
            
            if(s_running->sem) {// if task requires a semaphore - try to take
                if(!s_running->sem->take_nonblocking()) continue;
            }
                        
            // вроде бы выбрали задачу для переключения. 
            //   проверим отсутствие переполнения стека. Если это основной процесс то всегда, если это дочерние процессы то только если 
            //   новый ниже текущего. сама структура дескриптора процесса лежит в стеке процесса, поэтому можно сравнивать сами дескрипторы

            if(s_running->id!=0) {// we always can switch to main task
                if(me->id==0 || (uint32_t)s_running < (uint32_t)me){
                    uint32_t sp; 

                    asm volatile ("mov %0, sp\n\t"  : "=rm" (sp) );
            
                    // если нынешний указктель стека ниже конца дескриптора процесса то мы имеем переполнение стека, ААА все пропало!
                    if(sp < (uint32_t)s_running + sizeof(task_t) ) { 
                        // TODO исключить задачу из планирования, в предположении что дескриптор разрушен нужно обойти список по кругу
                        AP_HAL::panic("PANIC: stack overflow in process %d\n", me->id);
                    }
                }
        
                // проверим сохранность дескриптора
                if(s_running->guard != STACK_GUARD){
                    // TODO исключить задачу из планирования
                    AP_HAL::panic("PANIC: stack guard spoiled in process %d (from %d)\n",s_running->id, me->id);
                }
            }
            
            break; // we found task to run
        }
    }


    { // isolate 'me' again
        task_t *me = s_running; // switch to

        me->ttw=0; // time to wait is over
        me->start = _micros(); // task startup time
        me->in_isr=0; // reset ISR time

#ifdef MTASK_PROF
        me->ticks = stopwatch_getticks();
#endif
        longjmp(me->context, true);
        // never comes here
    }
}


/**
   * Return current task stack size.
   * @return bytes
 */
size_t REVOMINIScheduler::task_stack(){
  unsigned char marker;
  return (&marker - s_running->stack);
}





/* how to configure and schedule a PendSV exception
from http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0395b/CIHJHFJD.html
*/
#define CORTEXM4_SCB_SHPR3 ((uint32_t *)0xE000ED20)

// register IO completion routine
uint8_t REVOMINIScheduler::register_io_completion(Handler handler){
    if(num_io_completion == 0) { // initialize subsystem on 1st call
        // The PendSV exception is always enabled so set PRIMASK 
        // to prevent it from occurring while being configured 
        noInterrupts();

        // starts on 0x18, we need 3rd byte of register at 0x20 - so 0x22
        SCB->SHP[14 /* ISR number  */ - 4 /* SHP starts from 4 */ ] = 0xff;

        // Ensure the effect of the priority change occurs before 
        // clearing PRIMASK to ensure that future PendSV exceptions 
        // are taken at the new priority 
        asm volatile("dsb \n");  //DataSynchronizationBarrier();
        asm volatile("isb \n");  //InstructionSynchronizationBarrier();

        interrupts();
    }

    if(num_io_completion < MAX_IO_COMPLETION){
        io_completion[num_io_completion].handler=handler;
        io_completion[num_io_completion].sem=NULL;
        return ++num_io_completion;
    }
    return 0;
}

void REVOMINIScheduler::PendSV_Handler(){ // isr at lowest priority to do all IO completion routines
    bool do_it = false;
    
    do {
        do_it = false;
        for(uint8_t i=0; i<num_io_completion; i++){
            IO_Completion &io = io_completion[i];
            
            if(io.request) {
                io.request=false; // ASAP - it can be set again in interrupt
                if(io.handler){
                    if(io.sem && !io.sem->take_nonblocking()) { // semaphore active? take!
                        reschedule_proc(io.handler);// can't get semaphore, will try next time
                        continue;
                    }

                    do_it=true;
#ifdef SHED_PROF
                    uint32_t t = _micros();
#endif
                    revo_call_handler(io.handler,i); // unified way to call handlers

                    if(io.sem) io.sem->give(); // give back ASAP

#ifdef SHED_PROF
                    t = _micros() - t;
                    io.time += t;
#endif
                }
            }
        }
    } while(do_it);

}

void PendSV_Handler(){
    REVOMINIScheduler::PendSV_Handler();
}



////////////////////////////////////
/*
union Revo_handler { // кровь кишки ассемблер :) преобразование функторов в унифицированный вид
    voidFuncPtr vp;
    AP_HAL::MemberProc mp;          это С а не С++ поэтому мы не можем объявить поддержку функторов явно, и вынуждены передавать
    uint64_t h; // treat as handle             <-- как 64-битное число
    uint32_t w[2]; // words, to check. если функтор то старшее - адрес флеша, младшее - адрес в RAM. 
                                       Если ссылка на функцию то младшее - адрес флеша, старше 0
};
*/


void revo_call_handler(uint64_t hh, uint32_t arg){
    Revo_handler h = { .h = hh };

    if(ADDRESS_IN_FLASH(h.w[0])){
//        (h.vp)(arg);
        (h.isr)(arg);
    } else if(ADDRESS_IN_FLASH(h.w[1])) {
//        (h.mp)(arg);
        (h.mp)();
    }
}

void hal_yield(uint16_t ttw){ REVOMINIScheduler::yield(ttw); }
void hal_delay(uint16_t t){   REVOMINIScheduler::_delay(t); }
void hal_delay_microseconds(uint16_t t){ REVOMINIScheduler::_delay_microseconds(t);}
uint32_t hal_micros() { return REVOMINIScheduler::_micros(); }
