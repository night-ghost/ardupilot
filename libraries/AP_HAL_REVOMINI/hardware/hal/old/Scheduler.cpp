#pragma GCC optimize ("O2")


#include "Scheduler.h"

#include <stdio.h>
#include <AP_HAL_REVOMINI/AP_HAL_REVOMINI.h>


#include "Semaphores.h"
#include "I2CDevice.h"

#include <timer.h>


#include <AP_Notify/AP_Notify.h>
#include <AP_Math/AP_Math.h>

#include "RCInput.h"
#include <systick.h>
#include "GPIO.h"
#include <usb.h>


using namespace REVOMINI;
extern const AP_HAL::HAL& hal;


AP_HAL::Proc  REVOMINIScheduler::_failsafe  IN_CCM= NULL;
volatile bool REVOMINIScheduler::_timer_suspended IN_CCM= false;
volatile bool REVOMINIScheduler::_in_timer_proc  IN_CCM= false;

#ifndef PREEMPTIVE
revo_timer REVOMINIScheduler::_timers[REVOMINI_SCHEDULER_MAX_SHEDULED_PROCS] IN_CCM;
uint8_t    REVOMINIScheduler::_num_timers  IN_CCM= 0;
revo_tick  REVOMINIScheduler::_ticks[REVOMINI_SCHEDULER_MAX_SHEDULED_PROCS] IN_CCM;
uint8_t    REVOMINIScheduler::_num_ticks  IN_CCM= 0;
volatile bool REVOMINIScheduler::_timer_event_missed  IN_CCM= false;
bool REVOMINIScheduler::new_api_flag IN_CCM=0;

#endif

Revo_IO    REVOMINIScheduler::_io_proc[REVOMINI_SCHEDULER_MAX_IO_PROCS] IN_CCM;
uint8_t    REVOMINIScheduler::_num_io_proc IN_CCM=0;


AP_HAL::Proc REVOMINIScheduler::_delay_cb IN_CCM=NULL;
uint16_t REVOMINIScheduler::_min_delay_cb_ms IN_CCM=0;
void *   REVOMINIScheduler::_delay_cb_handle IN_CCM=0;

uint32_t REVOMINIScheduler::timer5_ovf_cnt IN_CCM=0;

bool REVOMINIScheduler::_initialized IN_CCM=false;

Handler REVOMINIScheduler::on_disarm_handler IN_CCM;
task_t * REVOMINIScheduler::_idle_task IN_CCM;


static void loc_ret(){}

#define STACK_GUARD 0x60a4d51aL

// Reference running task
task_t* s_running IN_CCM;
task_t* next_task IN_CCM;
// Main task and run queue
task_t REVOMINIScheduler::s_main = { 0 }; // NOT in CCM to can't be corrupted by stack
uint16_t REVOMINIScheduler::task_n=0;


struct REVOMINIScheduler::IO_COMPLETION REVOMINIScheduler::io_completion[MAX_IO_COMPLETION] IN_CCM;

uint8_t REVOMINIScheduler::num_io_completion  IN_CCM= 0;


// Initial top stack for task allocation
size_t REVOMINIScheduler::s_top IN_CCM; //  = MAIN_STACK_SIZE; - CCM not initialized!


#ifdef SHED_PROF
uint64_t REVOMINIScheduler::shed_time = 0;
bool     REVOMINIScheduler::flag_10s = false;
uint64_t REVOMINIScheduler::task_time IN_CCM = 0;
uint64_t REVOMINIScheduler::delay_time IN_CCM = 0;
uint64_t REVOMINIScheduler::delay_int_time IN_CCM = 0;
uint32_t REVOMINIScheduler::max_loop_time IN_CCM =0;
uint64_t REVOMINIScheduler::ioc_time IN_CCM =0;
uint64_t REVOMINIScheduler::sleep_time IN_CCM =0;
uint32_t REVOMINIScheduler::max_delay_err=0;

uint32_t REVOMINIScheduler::tick_micros IN_CCM;    // max exec time
uint32_t REVOMINIScheduler::tick_count IN_CCM;     // number of calls
uint64_t REVOMINIScheduler::tick_fulltime IN_CCM;  // full consumed time to calc mean
#endif


#ifdef MTASK_PROF
 uint32_t REVOMINIScheduler::max_wfe_time IN_CCM =0;
 uint64_t REVOMINIScheduler::tsched_time IN_CCM;
 uint32_t REVOMINIScheduler::tsched_count IN_CCM;
 uint32_t REVOMINIScheduler::tsched_sw_count IN_CCM;
 uint64_t REVOMINIScheduler::tsched_time_y IN_CCM;
 uint32_t REVOMINIScheduler::tsched_count_y IN_CCM;
 uint32_t REVOMINIScheduler::tsched_sw_count_y IN_CCM;
 uint64_t REVOMINIScheduler::tsched_time_t IN_CCM;
 uint32_t REVOMINIScheduler::tsched_count_t IN_CCM;
 uint32_t REVOMINIScheduler::tsched_sw_count_t IN_CCM;
 #ifdef SHED_DEBUG
  revo_sched_log REVOMINIScheduler::logbuf[SHED_DEBUG_SIZE] IN_CCM;
  uint16_t REVOMINIScheduler::sched_log_ptr;
 #endif

uint32_t REVOMINIScheduler::lowest_stack = (uint32_t)-1;
uint32_t REVOMINIScheduler::main_stack   = (uint32_t)-1;
uint32_t REVOMINIScheduler::max_stack_pc IN_CCM ;
#endif

bool REVOMINIScheduler::disable_stack_check=false;

bool REVOMINIScheduler::_in_io_proc IN_CCM =0;
uint32_t REVOMINIScheduler::MPU_overflow_cnt IN_CCM;
uint32_t REVOMINIScheduler::MPU_restart_cnt IN_CCM;
uint32_t REVOMINIScheduler::MPU_count IN_CCM;
uint32_t REVOMINIScheduler::MPU_Time IN_CCM;

#ifdef PREEMPTIVE
    volatile bool REVOMINIScheduler::need_io_completion IN_CCM;
    volatile bool REVOMINIScheduler::need_switch_task IN_CCM;
#endif

REVOMINIScheduler::REVOMINIScheduler()
{

    s_running = &s_main;         //  CCM don't initialized! - Reference running task
    next_task = &s_main;         
    s_top = MAIN_STACK_SIZE;     // Initial top stack for task allocation

// init main task
    memset(&s_main, 0, sizeof(s_main));

    Revo_handler h = { .vp=loc_ret }; // fake handler to not 0

    s_main.next = &s_main; // linked list
    s_main.prev = &s_main;
    s_main.priority = 100; // base priority
    s_main.active = true;  // not paused
    s_main.handle = h.h;        // to not 0
    s_main.guard = STACK_GUARD; // to check corruption of TCB by stack overflow

}


// to do when nothing to do 
static void idle_task(){
    while(1){
        __WFE(); // reset event flag
        __WFE(); // the sleep itself
        REVOMINIScheduler::yield(0);
    }
}

void REVOMINIScheduler::init()
{
    if(in_interrupt()){ // some interrupt caused restart at ISR level
        AP_HAL::panic("HAL initialization on ISR level=0x%x", (uint8_t)(SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk));
    }
#ifndef PREEMPTIVE
    memset(_timers,       0, sizeof(_timers) );
    memset(_ticks,        0, sizeof(_ticks) );
#endif
    memset(_io_proc,      0, sizeof(_io_proc) );
    memset(io_completion, 0, sizeof(io_completion) );


    // The PendSV exception is always enabled so set PRIMASK 
    // to prevent it from occurring while being configured 
    noInterrupts();

    NVIC_SetPriority(PendSV_IRQn, 15);         // lowest priority so all IRQs can't be switced
    NVIC_SetPriority(SVCall_IRQn, 14);         // priority 14 - the same as Timer7 ISR
    NVIC_SetPriority(SysTick_IRQn, 3);         // priority 3 - less thah fast device IO ISRs but higher than USB


    // Ensure the effect of the priority change occurs before 
    // clearing PRIMASK to ensure that future PendSV exceptions 
    // are taken at the new priority 
    asm volatile("dsb \n");  //DataSynchronizationBarrier();
    asm volatile("isb \n");  //InstructionSynchronizationBarrier();

    interrupts();


    CLEAR_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPDEEP_Msk)); //we don't need deep sleep
    SET_BIT(  SCB->SCR, ((uint32_t)SCB_SCR_SEVONPEND_Msk)); //we need Event on each interrupt

//*[ DEBUG
    SCnSCB->ACTLR |= SCnSCB_ACTLR_DISDEFWBUF_Msk; // disable imprecise exceptions
//]*/

    timer_foreach(timer_reset); // timer_reset(dev) moved out from configTimeBase so reset by hands
    {
        uint32_t period    = (2000000UL / SHED_FREQ) - 1; 
                // dev    period   freq, kHz
        configTimeBase(TIMER7, period, 2000);       //2MHz 0.5us ticks
        Revo_handler h = { .isr = _timer_isr_event };
        timer_attach_interrupt(TIMER7, TIMER_UPDATE_INTERRUPT, h.h , 0xE); // almost lowest priority, higher than Pend_SW to schedule task switch
        TIMER7->regs->CR1 |=  TIMER_CR1_URS;  // interrupt only by overflow, not by update
        timer_resume(TIMER7);
    }

    {// timer5 - 32-bit general timer, unused for other needs
     // so we can read micros32() directly from its counter and micros64() from counter and overflows
        configTimeBase(TIMER5, 0, 1000);       //1MHz 1us ticks
        timer_set_count(TIMER5,(1000000/SHED_FREQ)/2); // to not interfere with TIMER7
        Revo_handler h = { .isr = _timer5_ovf };
        timer_attach_interrupt(TIMER5, TIMER_UPDATE_INTERRUPT, h.h, 2); // high priority
        timer_resume(TIMER5);
    }

    {     // only Timer6 from spare timers has personal NVIC line - TIM6_DAC_IRQn
        uint32_t freq = configTimeBase(TIMER6, 0, 20000);     // 20MHz - we here don't know real freq so can't set period
        timer_set_reload(TIMER6, freq / 1000000);             // period to generate 1uS requests
        timer_enable_irq(TIMER6, TIMER_UPDATE_INTERRUPT);     // enable interrupt requests from timer but not enable them in NVIC - will be events
        timer_resume(TIMER6);
    }

    { // timer to generate more precise delays via quant termination
                // dev    period   freq, kHz
        configTimeBase(TIMER11, 0, 1000);       //1MHz 1us ticks
        Revo_handler h = { .isr = _tail_timer_event };
        timer_attach_interrupt(TIMER11, TIMER_UPDATE_INTERRUPT, h.h , 0xE); // priority 14 - the same as Timer7 and SVC
        TIMER11->regs->CR1 &= ~TIMER_CR1_ARPE; // not buffered preload
        TIMER11->regs->CR1 |=  TIMER_CR1_URS;  // interrupt only by overflow
    }


#ifdef PREEMPTIVE
    void *task = _start_task((uint32_t)idle_task, 256); // only for one context
    set_task_priority(task, 255); // lowest possible, to fill delay()
    _idle_task=(task_t *)task;
#endif
    
#ifdef DEBUG_BUILD
// show stats output each 10 seconds
    task = register_timer_task(10000000, FUNCTOR_BIND_MEMBER(&REVOMINIScheduler::_set_10s_flag, bool), NULL);
    set_task_priority(task, 100); // like main task has
#endif


}

void REVOMINIScheduler::_delay(uint16_t ms)
{
    uint32_t start = _micros();
#ifdef SHED_PROF
    uint32_t t=start;
#endif
    
    while (ms > 0) {
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

#define NO_YIELD_TIME 10

void REVOMINIScheduler::_delay_microseconds(uint16_t us)
{
    uint32_t rtime = stopwatch_getticks(); // get start ticks first

#ifdef SHED_PROF
    uint32_t t = _micros(); 
#endif
    uint16_t no_yield_t=us/20; // 5%
    task_t *me=s_running;
    
    // guard time for main process
    if(me->id==0 && no_yield_t<NO_YIELD_TIME) no_yield_t=NO_YIELD_TIME;

    uint32_t dt = us_ticks * us;  // delay time in ticks
    uint32_t ny = us_ticks * no_yield_t; // no-yield time in ticks
    
    uint32_t tw;

    while ((tw = stopwatch_getticks() - rtime) < dt) { // tw - time waiting, in ticks
        if((dt - tw) > ny ) { // No Yeld time - 3uS to end of wait 
            yield((dt - tw) / us_ticks); // in micros
        }
    }    

#ifdef SHED_PROF
    uint32_t r_us=_micros()-t; // real time
    
    if(_in_timer_proc)
        delay_int_time +=r_us;
    else
        delay_time     +=r_us;

    if(me->id !=0) return; // check only for main task

    uint32_t err = labs(us - r_us);
    if(err <= max_delay_err) {
        return;
    }
    max_delay_err = err;
    
    if(err>1000) {
        printf("\n delay() error=%ld too big!\n",err);
    }
    
#endif

}

void REVOMINIScheduler::_delay_us_ny(uint16_t us){ // precise no yield delay
    uint32_t rtime = stopwatch_getticks(); // get start ticks first

    uint32_t dt = us_ticks * us;  // delay time in ticks
    
    while ((stopwatch_getticks() - rtime) < dt) {
        // __WFE(); 
    }    

#ifdef SHED_PROF
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


void REVOMINIScheduler::_run_io(void)
{
    if (_in_io_proc) {
        return;
    }
    _in_io_proc = true;

    // now call the IO based drivers
    for (int i = 0; i < _num_io_proc; i++) {
        if (_io_proc[i].h) {
            revo_call_handler(_io_proc[i].h,0);
            if(_io_proc[i].flags == IO_ONCE){
                _io_proc[i].h = 0;
            }
#ifndef PREEMPTIVE
            yield(0); // не все сразу!
#endif
        }
    }

    _in_io_proc = false;
}


void REVOMINIScheduler::_register_io_process(Handler h, Revo_IO_Flags flags)
{
    if(_num_io_proc>=REVOMINI_SCHEDULER_MAX_IO_PROCS) return;

    if(_num_io_proc==0){
        void *task = start_task(_run_io, IO_STACK_SIZE);
#ifdef PREEMPTIVE
        set_task_period(task, 100); 
#else
        set_task_ttw(task, 100); // 500uS between calls to this task - no more 2kHz
#endif
        set_task_priority(task, IO_PRIORITY); 
    }

    uint8_t i;
    for(i=0; i<_num_io_proc; i++){ // find free slots
        if(_io_proc[i].h == 0) {  // found
            _io_proc[i].h = h; 
            _io_proc[i].flags = flags;
            return;
        }
    }

    i=_num_io_proc++;
    _io_proc[i].h = h;
    _io_proc[i].flags = flags;
}



void REVOMINIScheduler::resume_timer_procs()
{
    _timer_suspended = false;
#ifndef PREEMPTIVE
    if (_timer_event_missed == true) {
        _run_timer_procs(false);        // TODO here code executes on main thread, not in interrupt level!
        _timer_event_missed = false;
    }
#endif
}

#ifdef MTASK_PROF
void REVOMINIScheduler::check_stack(uint32_t sp) { // check for stack usage
    
//    uint32_t * stack = (uint32_t *)sp;

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
    
    if(ADDRESS_IN_CCM(sp)){
        if(_in_main_thread()){
            if(sp<main_stack) { main_stack=sp; }
        }else {
            if(sp<lowest_stack){ lowest_stack=sp; }
        }
    }
}
#endif


void REVOMINIScheduler::_run_timer_procs(bool called_from_isr) {

    if (_in_timer_proc) {
        return;
    }
    _in_timer_proc = true;

#ifndef PREEMPTIVE
    if (!_timer_suspended) {
        _run_timers(); 
    } else if (called_from_isr) {
        _timer_event_missed = true;
    }
#endif

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

void REVOMINIScheduler::_timer_isr_event(uint32_t v  /* TIM_TypeDef *tim */) {
#ifdef MTASK_PROF

    uint32_t sp; 
    // Get stack pointer
    asm volatile ("MRS     %0, PSP\n\t"  : "=rm" (sp));

    check_stack(sp);
#endif

    _run_timer_procs(true);
    
#ifdef PREEMPTIVE
    if(task_n && !need_switch_task) {        // if there are created tasks
 #ifdef MTASK_PROF
        uint32_t t=stopwatch_getticks();
 #endif
        next_task = get_next_task();

 #ifdef MTASK_PROF
        tsched_count++;
        tsched_time+=stopwatch_getticks() - t;
 #endif

        if(next_task != s_running) { // if we should switch task
            tsched_sw_count++;
            do_io_completion(0);     // plan context switch
        }
    }
#endif
}

void REVOMINIScheduler::_timer5_ovf(uint32_t v /* TIM_TypeDef *tim */) {
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
#ifndef I_KNOW_WHAT_I_DO
    if (_initialized) {
        AP_HAL::panic("PANIC: scheduler::system_initialized called more than once");
    }
#endif
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


void REVOMINIScheduler::stats_proc(void){
//    _print_stats(); only for debug

}

#ifdef DEBUG_BUILD
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

            printf("\nSched stats:\n  %% of full time: %5.2f  Efficiency %5.3f max loop time %ld\n", (task_time/10.0)/t /* in percent*/ , shed_eff, max_loop_time);
            printf("delay times: in main %5.2f including in semaphore %5.2f  in timer %5.2f",         (delay_time/10.0)/t, (Semaphore::sem_time/10.0)/t,  (delay_int_time/10.0)/t);
            max_loop_time=0;

#ifdef ISR_PROF
            printf("\nISR time %5.2f max %5.2f", (isr_time/10.0/(float)us_ticks)/t, max_isr_time/(float)us_ticks );
            max_isr_time=0;
#endif
            printf("\nmax delay() error= %ld wfe time = %ld\n", max_delay_err, max_wfe_time ); max_delay_err=0; max_wfe_time=0;
#ifdef MPU_DEBUG
            printf("MPU overflows: %ld restarts %ld max samples %ld time %ld\n", MPU_overflow_cnt, MPU_restart_cnt, MPU_count, MPU_Time); MPU_overflow_cnt=0; MPU_restart_cnt=0; MPU_count=0; MPU_Time=0;
#endif

            printf("PPM max buffer size: %d\n", REVOMINIRCInput::max_num_pulses); REVOMINIRCInput::max_num_pulses=0;
#endif
                        
        } break;

        case 1:{
#ifdef SHED_PROF 
#ifndef PREEMPTIVE
            printf("\nTask times:\n");

            for(int i=0; i< _num_timers; i++) {
                if(_timers[i].proc){    // task not cancelled?
                    printf("shed task 0x%llX tim %8.1f int %5.3f%% tot %6.4f%% mean time %5.1f max time %ld\n", _timers[i].proc, _timers[i].fulltime/1000.0, _timers[i].fulltime*100.0 / task_time, (_timers[i].fulltime / 10.0) / t, (float)_timers[i].fulltime/_timers[i].count, _timers[i].micros );
                    _timers[i].micros = 0; // reset max time
                }
            }
            printf("tick tasks tim %8.1f int %5.3f%% tot %6.4f%% mean time %5.1f max time %ld\n",  tick_fulltime/1000.0, tick_fulltime*100.0 / task_time, (tick_fulltime / 10.0) / t, (float)tick_fulltime/tick_count, tick_micros ); tick_micros=0;
#endif
#endif
    
            }break;
            
        case 2:{
#ifdef MTASK_PROF    
            task_t* ptr = &s_main;

            uint32_t  fc=tsched_count+tsched_count_y+tsched_count_t;
            printf("\nsched time: by timer %5.2f (%5.2f%%) sw %5.2f%% in yield %5.2f (%5.2f%%) sw %5.2f%% in tails %5.2f (%5.2f%%) sw %5.2f%% total %5.2f%%\n", ((float)tsched_time/tsched_count)/us_ticks, 100.0*tsched_count/fc, 100.0 * tsched_sw_count/tsched_count, ((float)tsched_time_y/tsched_count_y)/us_ticks, 100.0*tsched_count_y/fc,100.0 * tsched_sw_count_y/tsched_count_y,((float)tsched_time_t/tsched_count_t)/us_ticks, 100.0*tsched_count_t/fc,100.0 * tsched_sw_count_t/tsched_count_t, 100.0*(tsched_time+tsched_time_y+tsched_time_t)/t/us_ticks );
        
            do {
                printf("task %d (0x%llx) times: full %8.1fms (%7.2f%%) mean %8.1fuS max %lduS full %lduS\n",  ptr->id, ptr->handle, ptr->time/1000.0, 100.0 * ptr->time/1000.0 / t, (float)ptr->time / ptr->count, ptr->max_time, ptr->work_time);
        
                ptr->max_time=0; // reset times
                ptr->work_time=0;
                
                ptr = ptr->next;
            } while(ptr != &s_main);
#endif
            }break;

        case 3: {
            uint8_t n = REVOI2CDevice::get_dev_count();
            printf("\nI2C stats\n");
    
            for(uint8_t i=0; i<n; i++){
                REVOI2CDevice * d = REVOI2CDevice::get_device(i);
                if(d){
                    printf("bus %d addr %x errors %ld last error=%d\n",d->get_bus(), d->get_addr(), d->get_error_count(), d->get_last_error());   
                }
            }
            }break;

        case 4: {
            uint32_t heap_ptr = (uint32_t)__brkval; // here should be upper bound of sbrk()
            uint32_t bottom=(uint32_t)&_sdata;
            
            // 48K after boot 72K while logging on
            printf("\nMemory used: %ldk:\n",(heap_ptr-bottom)/1024);
            printf("Free stack: %ldk:\n",(lowest_stack - (uint32_t)&_eccm)/1024);
            printf("Main stack use: %ldk\n",((uint32_t)&_sccm + 0x10000 /* 64K CCM */ - main_stack)/1024);

            } break;
        
        case 5: {
            printf("\nIO completion time=%9.1fms (%7.3f%%)\n", ioc_time/1000.0,  ioc_time/1000.0/t*100);
            uint64_t iot=0;
            for(uint8_t i=0; i<num_io_completion; i++){
                struct IO_COMPLETION &io = io_completion[i];
                
                if(io.handler) {
                    if(io.count){
                        printf("task %llx time %9.1fms (%7.3f%%) mean %7.3fuS\n", io.handler,  io.time/1000.0, 100.0 * io.time / t / 1000, (float)io.time/io.count);
                        
                        iot+=io.time;
                    }
                }    
            }
            if(ioc_time)
                printf("IO completion effectiveness=%7.3f%%\n",  100.0 * iot/ioc_time);
        
            }break;
            
        case 6:
        default:
            cnt=0;
            break;
        }
    }
}

bool REVOMINIScheduler::_set_10s_flag(){
    flag_10s=true;
    _print_stats();
    return true;
}
#endif

/*
[    common realization of all Device.PeriodicCallback;
*/

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"


AP_HAL::Device::PeriodicHandle REVOMINIScheduler::_register_timer_task(uint32_t period_us, Handler proc, REVOMINI::Semaphore *sem, revo_cb_type mode){

#ifndef PREEMPTIVE
    if(new_api_flag){ // new IO_Completion api allows to not wait in interrupt so can be scheduled in timers interrupt
        new_api_flag=false;
    } else 
#endif
    {
        // slow tasks will runs at individual IO tasks
        void *task = _start_task(proc, SMALL_TASK_STACK);
        if(task){
            set_task_period(task, period_us);
            set_task_priority(task, DRIVER_PRIORITY);
            set_task_semaphore(task, sem);
        }
        return (AP_HAL::Device::PeriodicHandle)task;
    }

#ifndef PREEMPTIVE
    uint8_t i;
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
        rt.sem2 = NULL; // no check semaphore
        rt.mode = mode;
 #ifdef SHED_PROF
        rt.count = 0;
        rt.micros = 0;
        rt.fulltime = 0;
 #endif
        noInterrupts();    // 64-bits should be 
        rt.proc = proc;    //     last one, not interferes - guard is over
        interrupts();
        return (AP_HAL::Device::PeriodicHandle)&rt;
    }

    return NULL;
#endif
}




bool REVOMINIScheduler::adjust_timer_task(AP_HAL::Device::PeriodicHandle h, uint32_t period_us)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align" // yes I know

#ifdef PREEMPTIVE
    task_t *p = (task_t *)h;
#else

    revo_timer *p = (revo_timer *)h;
#endif
#pragma GCC diagnostic pop
    p->period = period_us;
    
    return true;
}
bool REVOMINIScheduler::unregister_timer_task(AP_HAL::Device::PeriodicHandle h)
{
#ifdef PREEMPTIVE
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wcast-align"
    task_t *p = (task_t *)h;
 #pragma GCC diagnostic pop

    noInterrupts(); // 64-bits should be 
    p->handle=0L;
    interrupts();
#else
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wcast-align"
    revo_timer *p = (revo_timer *)h;
 #pragma GCC diagnostic pop

    noInterrupts(); // 64-bits should be 
    p->proc=0L;
    interrupts();
#endif
    return true;
}

#ifndef PREEMPTIVE
void REVOMINIScheduler::set_checked_semaphore(AP_HAL::Device::PeriodicHandle h, REVOMINI::Semaphore *sem){
    revo_timer *p = (revo_timer *)h;
    p->sem2 = sem;
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

void REVOMINIScheduler::do_at_next_tick(Handler proc, REVOMINI::Semaphore *sem){
    uint8_t i;
    for (i = 0; i < _num_ticks; i++) {
        if ( _ticks[i].proc == 0L /* free slot */ ) goto store;        
        else if( _ticks[i].proc ==proc) return;  // already is
    }

    if (_num_ticks < REVOMINI_SCHEDULER_MAX_SHEDULED_PROCS) {
        /* this write to _ticks[] can be outside the critical section
         * because that memory won't be used until _num_ticks is
         * incremented or where proc is NULL. */

        i = _num_ticks;

        _ticks[i].proc = 0L; // clear proc - this entry will be skipped
        _num_ticks++; // now nulled proc guards us
store:        
        revo_tick &rt=_ticks[i];
        rt.sem  = sem;
        noInterrupts();    // 64-bits should be 
        rt.proc = proc;    //     last one, not interferes - guard is over
        interrupts();
//        return (AP_HAL::Device::PeriodicHandle)&rt;
    }
}

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
    uint8_t i;
    for(i = 0; i<_num_timers; i++){
        revo_timer &tim = _timers[i];
        
        if(tim.proc){    // task not cancelled?
/*
    у нас время в 32-разрядном счетчике, который может переполниться
    now - tim.last_run - время с момента прошлого запуска, с учетом всех переполнений
    
*/
            if( (now - tim.last_run) > tim.period) { // time to run?
                uint8_t ret=1;  // OK by default
                if(tim.sem2){
                    if(!tim.sem2->take_nonblocking()) continue; // semaphore busy? go out, will try next tick
                    tim.sem2->give();   // give back ASAP
                }

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
                
                if(ret) {  // ok?
                    tim.last_run    += tim.period; // прошлое время запуска - по надобности а не по факту
                } else {// reschedule
                }

            }
        }
    }
    for (i = 0; i < _num_ticks; i++) {
        revo_tick &ti = _ticks[i];
        if ( ti.proc ) {
            if(ti.sem) {
                if(!ti.sem->take_nonblocking())  // semaphore active? take!
                    continue; // can't take - semaphore busy, go out
                ti.sem->give(); //  give back ASAP! task will take it itself. we in interrupt so no one can catch it
            }
            Handler h = ti.proc;
            ti.proc=0;           // clear before call
 #ifdef SHED_PROF
            uint32_t t = _micros();
 #endif          
            revo_call_handler(h,0);
 #ifdef SHED_PROF
            t = _micros() - t;               // work time

            if(tick_micros < t)
                tick_micros   =  t;      // max time
            tick_count     += 1;          // number of calls
            tick_fulltime  += t;          // full time, mean time = full / count
            job_t += t;                  // time of all jobs
 #endif        

        }
    }





    full_t = _micros() - full_t;         // full time of scheduler
 #if defined(MTASK_PROF) 
// исключить время работы прерывания из времени задачи, которую оно прервало
    s_running->in_isr += full_t;
 #endif

 #ifdef SHED_PROF
    uint32_t shed_t = full_t - job_t;   // net time

    if(full_t>max_loop_time)
        max_loop_time=full_t;

    task_time += job_t; // full time in tasks
    shed_time += shed_t;
    
 #endif                

}
 #pragma GCC diagnostic pop

#endif

// ]


//[ -------- realization of cooperative/preemptive multitasking --------

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
    tp.priority=108;  // main task has 100 so by default such tasks will use 1/8 of CPU
    tp.active = true; // all tasks starts as active
#ifdef MTASK_PROF
    tp.start=_micros(); 
#endif

    tp.guard = STACK_GUARD;

    return (uint32_t)&tp; // только финт с возвратом из функции снимает проклятье "локальный адрес"
}

#if 0 // однажды назначенные задачи никто не отменяет

task_t* REVOMINIScheduler::get_empty_task(){
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


#ifdef PREEMPTIVE
void REVOMINIScheduler::do_task(task_t *task) {
    while(1){
        uint32_t t=0;
        if(task->handle && task->active) { // Task Switch occures asyncronously so we should wait until task becomes active again
            task->time_start=_micros();
            if(task->sem && !task->in_ioc) {// if task requires a semaphore - block on it
                if(!task->sem->take(HAL_SEMAPHORE_BLOCK_FOREVER)) {
                    yield(0);   // can't be
                    continue;
                }
            }
            revo_call_handler(task->handle, 0); 
            if(task->sem && !task->in_ioc) task->sem->give(); // give semaphore when task finished
            task->active=false;     // turn off active, to know when task is started again
            task->curr_prio = task->priority - 6; // just activated task will have a highest priority for one quant

            task->has_semaphore=0; // task is over so glitch
            t = _micros()-task->time_start; // execution time
            if(t > task->work_time)  task->work_time=t;
        }
        yield(0);        // give up quant remainder
    }// endless loop
}

void * REVOMINIScheduler::init_task(Handler handler, const uint8_t* stack){
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align" // yes I know

    task_t *task = (task_t *)((uint32_t)(stack-sizeof(task_t)) & 0xFFFFFFFCUL); // control block below top of stack, 4-byte alignment
#pragma GCC diagnostic pop

    uint32_t ret=fill_task(*task);  // Add task as last to run queue (main task)
    task->stack  = stack;
    task->handle = handler; // save handler to task to enable to change it later

    /*
     * ARM Architecture Procedure Call Standard [AAPCS] requires 8-byte stack alignment.
     * This means that we must get top of stack aligned _after_ context "pushing", at
     * interrupt entry.
     */
    uint32_t *sp =(uint32_t *) (((uint32_t)task - 4) & 0xFFFFFFF8UL); // below TCB

// HW frame
    *(--sp)  = 0x01000000UL;          // xPSR
    //           61000000
    *(--sp)  = ((uint32_t)do_task);   // PC Entry Point - task executor
    *(--sp)  = ((uint32_t)do_task)|1; // LR the same, with thumb bit set
    sp -= 4;                          // emulate "push R12,R3,R2,R1"
    *(--sp)  = (uint32_t)task;        // emulate "push r0"
// SW frame, context saved as  "STMDB     R0!, {R4-R11, LR}"
    *(--sp)  = 0xFFFFFFFDUL;          // emulate "push lr" =exc_return: Return to Thread mode, floating-point context inactive, execution uses PSP after return.
    asm volatile ("STMDB     %0!, {R4-R11}\n\t"  : "+rm" (sp) ); // push real registers - they can be global register variables
//    sp -= 8;                        // emulate "push R4-R11"
    task->sp=(uint8_t *)sp;           // set stack pointer of task

    return (void *)ret;
}


#else // cooperative multitask


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
        uint32_t t;
        while (1) {
            if(task.handle) {
                if(task.sem && !task.in_ioc) {// if task requires a semaphore - try to take
                    if(!task.sem->take_nonblocking()) {
                        yield(0);
                        continue;
                    }
                }
                task.time_start=_micros();
                revo_call_handler(task.handle, 0); // call the task 
                if(task.sem && !task.in_ioc) task.sem->give(); // give semaphore when task finished
                task.active=false;
                task.has_semaphore=0; // task is over so glitch

                t = _micros()-task.time_start; // execution time
                if(task.def_ttw && task.def_ttw > t) t = task.def_ttw - t; // time to wait
                else                                 t = 0;
            } else t=0;
            yield(t);        // in case that function not uses delay();
        }// endless loop
    }
    // caller returns
//  return &task; GCC optimizes out so returns 0
    return (void *)ret;
}
#endif


void * NOINLINE REVOMINIScheduler::_start_task(Handler handle, size_t stackSize)
{
    // Check called from main task and with valid task loop function
    if (!_in_main_thread() ) return NULL;

    // Adjust stack size with size of task context
    stackSize += sizeof(task_t)+8; // for alignment
    void * ret;

    disable_stack_check = true;

    { // isolate stack[]

#ifdef PREEMPTIVE
    // we don't need to use arrays because we will prepare context
        if (s_main.stack == NULL) {       // first call, initialize all task subsystem
            s_main.stack = (const uint8_t*)RAMEND - s_top; // remember bottom of stack of main task on first call
        }

        const uint8_t *sp=(const uint8_t*)s_main.prev->stack; // top of stack for new task    

        ret=init_task(handle, sp); // give stack top as parameter, will correct later
        task_t *task=(task_t *)ret;
        sp-= stackSize;               // calc stack bottom
        task->stack = sp;             // correct to bottom of stack
        stack_bottom = (caddr_t)sp; // and remember for memory allocator
        s_top += stackSize;

#else
        // Allocate stack(s) and check if main stack top should be set
        size_t frame = RAMEND - (size_t) &frame; // already used stack

// allocate stack for previous  task so this memory already used!
        volatile uint8_t stack[s_top - frame]; // should be volatile else it will be optimized out
        
        if (s_main.stack == NULL) { // first call, initialize all task subsystem
            s_main.stack = (const uint8_t*)stack; // remember on first call stack of main task
        }

        // Check that the task can be allocated without already used CCM
        if ((s_top + stackSize) > (STACK_MAX       -     (&_eccm-&_sccm))) {
            disable_stack_check = false;
            return NULL;
        }

        // Adjust stack top for next task allocation


        const uint8_t *sp=(const uint8_t*)(stack - stackSize); // stack for new task    
// done at boot      memset((void)sp, 0x55, sizeof(stackSize)); // for stack usage debugging fill the stack for new task
        stack_bottom = (caddr_t)sp; // remember for memory allocator
        // Initiate task with stack top
        ret=init_task(handle, sp);    // give bottom of stack as parameter
#endif
    }
    disable_stack_check = false;
    return ret; // return address of task descriptor
}

// task should run periodically, period in uS. this will be high-priority task
void REVOMINIScheduler::set_task_period(void *h, uint32_t period){
    task_t *task = (task_t *)h;

    task->period = period;
    task->active = false; // will bi first started after 'period'
    task->start  = _micros();
}

// default TTW
void REVOMINIScheduler::set_task_ttw(void *h, uint32_t period){
#if !defined(PREEMPTIVE)
    task_t *task = (task_t *)h;
    
    task->def_ttw = period;
#endif
}

void REVOMINIScheduler::set_task_priority(void *h, uint8_t prio){
    task_t *task = (task_t *)h;

    task->priority = prio;
    task->curr_prio= prio;
}



// task wants to run only with this semaphore owned
void REVOMINIScheduler::set_task_semaphore(void *h, REVOMINI::Semaphore *sem){
    task_t *task = (task_t *)h;
    
    task->sem = sem;
}

#ifdef SHED_DEBUG
static uint16_t next_log_ptr(uint16_t sched_log_ptr){
    uint16_t lp = sched_log_ptr+ 1;
    if(lp >= SHED_DEBUG_SIZE) lp=0;
    return lp;
}
#endif




// this function called only from Level 14  ISRs so there is no need to be reentrant
task_t *REVOMINIScheduler::get_next_task(){
    task_t *me = s_running; // current task
    task_t *task=_idle_task; // task to switch to, idle_task by default
    uint32_t loop_count=0;
    uint32_t timeFromLast=0;
    uint32_t remains = 0;

    uint32_t partial_quant=(uint32_t)-1;
    volatile task_t *want_tail=NULL;


#if 0
    if(me->has_semaphore) {
        me->sem_count++;
        if(me->curr_prio>10)   me->curr_prio -= me->has_semaphore;      // increase priority if task owns semaphore
    }
#endif

#ifdef SHED_DEBUG
    uint32_t ttw_skip_count=0;
#endif

    uint32_t t =  _micros();
    me->t_yield = t;

    { // isolate dt
#if !defined(PREEMPTIVE) || defined(MTASK_PROF) 
        uint32_t dt =  t - me->start;       // time in task
        if(dt >= me->in_isr) dt -= me->in_isr;  // minus time in interrupts
        else                 dt=0;

        me->time+=dt;                           // calculate sum
#endif
#if !defined(PREEMPTIVE)
        if(dt > me->max_delay) me->max_delay = dt; // and remember maximum to not schedule long tasks in small slices
#endif


#ifdef MTASK_PROF
        if(dt > me->max_time) {
            me->max_time = dt; // maximum to show
//            me->maxt_addr = ret;
        }

 #ifdef SHED_DEBUG
        {
            revo_sched_log &lp = logbuf[sched_log_ptr];
            lp.end = t;
            lp.task_id=me->id;
            lp.ttw = me->ttw;
            sched_log_ptr = next_log_ptr(sched_log_ptr);
            ZeroIt(logbuf[sched_log_ptr]); // clear next
        }
        ttw_skip_count=0;
 #endif
#endif
    }
    uint32_t now;

    task_t *ptr = me; // starting from current task

    while(true) { // lets try to find task to switch to
        ptr = ptr->next; // Next task in run queue will continue

#if 0
        if(!(ADDRESS_IN_RAM(ptr) || ADDRESS_IN_CCM(ptr)) ){
            AP_HAL::panic("PANIC: s_rinning spoiled in process %d\n", me->id);
        }
#endif
        now= _micros(); // renew each loop

            
        if(!ptr->handle) goto skip_task; // skip finished tasks
        
        if(ptr->sem_wait) { // task want a semaphore
            if(ptr->sem_wait->is_taken()) { // task blocked on semaphore
                if(ptr->sem_wait->get_owner() != ptr) { // owner is another task?
                    if(ptr->sem_time != HAL_SEMAPHORE_BLOCK_FOREVER) { // is time to wait limited?
                        uint32_t dt = _micros() - ptr->sem_start_wait;   // time since start waiting
                        if(dt < ptr->sem_time) {
                            if(ptr->curr_prio>1) ptr->curr_prio--;      // increase priority as task waiting for a semaphore
                            goto skip_task; 
                        } else {
                            ptr->sem_wait=NULL; // time to wait is over
                        }
                    } else goto skip_task;  // endless waiting
                } else  {
                    ptr->sem_wait=NULL; // task tries to get a semaphore that already owns, something wrong
                }
            } else {
                ptr->sem_wait=NULL; // clear semaphore after release
            }
        }
            
        // если для задачи установлен период выполнения и она в самом начале - проверим 
        if(!ptr->active){
            if(ptr->period){
                if(_timer_suspended) goto skip_task; //       timed tasks can't be started
                
                timeFromLast = now - ptr->time_start; // time from last run  less  than task's period
                if( timeFromLast < ptr->period) {
                    remains = ptr->period - timeFromLast;
                    if(remains<partial_quant) {
                        partial_quant=remains; // minimal time remains to next task
                        want_tail = ptr;
                    }
                    goto skip_task; 
                }
            }else { // non-active non-periodic tasks
                goto skip_task; // should be skipped
            }
            ptr->active=true; // selected task to run, even if it lose quant by priority

        } else { // задачи без строгого периода, обычный тайм слайс

            if(ptr->ttw){// task wants to wait 

                timeFromLast = now - ptr->t_yield; // time since that moment
                if(timeFromLast < ptr->ttw){               // still less than ttw ?
                    remains = ptr->ttw - timeFromLast; // remaining time to wait
                    if(remains<partial_quant) {
                        partial_quant=remains;
                        want_tail = ptr;
                    }
                    goto skip_task; 
                }
            }
                
#ifndef PREEMPTIVE                
            //     main task always    not periodic           task max execution time more than we have - but not for periodic drivers which have high priority
            if(ttw && ptr->id!=0 && !ptr->period && ptr->max_delay > ttw ) { 
                ptr->max_delay --; //  понемногу уменьшаем дабы совсем не выключить
 #ifdef SHED_DEBUG
                ttw_skip_count++;
 #endif

                goto skip_task;
            }
            if(loop_count==0) { // first loop we tries to select only high-priority tasks
#define HIGH_TIMESLICE_TIME  1 // не вызывать приориитетную задачу, вызвавшую yield() меньше этого времени назад                    
#define MID_TIMESLICE_TIME  5 // не вызывать основную задачу, вызвавшую yield() меньше этого времени назад                    
#define LOW_TIMESLICE_TIME 10 // не вызывать фоновую задачу, вызвавшую yield() меньше этого времени назад                    

                dt=LOW_TIMESLICE_TIME;
                if(ptr->period)  dt=HIGH_TIMESLICE_TIME;
                if(ptr->id==0)   dt=MID_TIMESLICE_TIME;
                                

                if(now - ptr->t_yield < dt) goto skip_task; // some time between calls to the same task if there are anothers
            }
                // on 2nd loop we selects any available task
#endif
        }

        if(ptr->curr_prio <= task->curr_prio){ // select the most priority task, round-robin for equal priorities
            // task loose tick
            if(task->priority != 255) { // not for idle task
                if(task->curr_prio>1)
                    task->curr_prio--;      // increase priority if task loose tick
// в результате роста приоритета ожидающей задачи мы не останавливаем низкоприоритетные задачи полностью, а лишь замедляем их
// выполнение, заставляя пропустить число тиков, равное разности приоритетов. В результате низкоприоритетная задача выполняется на меньшей скорости,
// которую можно настраивать изменением разницы приоритетов
            }
            task = ptr; // winner
        }
skip_task:
    // we should do this check after EACH task so can't use "continue" which skips ALL loop. And we can't move this to begin of loop because then interrupted task does not participate in the comparison of priorities
        if(ptr == me) {  // 'me' is the task that calls yield(), so full loop - now we have most-priority task
            break; // so let it run!
        }

    }

 #ifdef SHED_DEBUG
    revo_sched_log &lp = logbuf[sched_log_ptr];
    lp.start = now;
    lp.task_id=task->id;
    lp.loop_count=loop_count;
    lp.prio = task->curr_prio;
//    lp.ttw_skip_count=ttw_skip_count;
    lp.active = task->active;
    lp.time_start = task->time_start;
    lp.timeFromLast=timeFromLast;
    lp.remains = remains;
    lp.quant = partial_quant;
    ZeroIt(logbuf[next_log_ptr(sched_log_ptr)]); // clear next
 #endif

    task->curr_prio=task->priority; // reset current priority to default value
    task->ttw=0;        // time to wait is over
    task->start = now;  // task startup time
#if !defined(PREEMPTIVE) || defined(MTASK_PROF) 
    task->in_isr=0; // reset ISR time
#endif
    // выбрали задачу для переключения. 
    //   проверим отсутствие переполнения стека. Если это основной процесс то всегда, если это дочерние процессы то только если 
    //   новый ниже текущего. сама структура дескриптора процесса лежит в стеке процесса, поэтому можно сравнивать сами дескрипторы

    if(task->id!=0) {// we always can switch to main task
#if 0// let stack_guard works

        if(me->id==0 || (uint32_t)task < (uint32_t)me){
            uint32_t sp; 

            asm volatile ("mov %0, sp\n\t"  : "=rm" (sp) );
            
            // если нынешний указктель стека ниже конца дескриптора процесса то мы имеем переполнение стека, ААА все пропало!
            if(sp < (uint32_t)task + sizeof(task_t) ) { 
                // TODO исключить задачу из планирования, в предположении что дескриптор разрушен нужно обойти список по кругу
                AP_HAL::panic("PANIC: stack overflow in process %d\n", me->id);
            }
        }
#endif        
        // проверим сохранность дескриптора
        if(task->guard != STACK_GUARD){
            // TODO исключить задачу из планирования
            AP_HAL::panic("PANIC: stack guard spoiled in process %d (from %d)\n", task->id, me->id);
        }
    }

            
#ifdef MTASK_PROF
    task->count++;

 #if !defined(PREEMPTIVE)
    uint32_t err=0, rt=0;
    if(task->period) {
        rt = task->time_start+task->period;     // required time to start - a period from last
    } else if(task->ttw) {
        rt = task->t_yield+task->ttw;           // required time to start
    }
    if(rt && now>rt) err = now - rt; // scheduling error

    if(err>task->sched_error) task->sched_error=err;
 #endif
#endif

    if(want_tail) { // we have a task that want to be started next
        if(partial_quant < TIMER_PERIOD*5) { 
            timer_set_count(TIMER11, 0);
            timer_set_reload(TIMER11, partial_quant+1);
            timer_resume(TIMER11);
        }
    }

    return task;
}

/*
    interrupt to reduce timeslice quant
*/
void REVOMINIScheduler::_tail_timer_event(uint32_t v /*TIM_TypeDef *tim */){
    timer_pause(TIMER11);
#ifndef MTASK_PROF
    switch_task();
#else
    timer_generate_update(TIMER7); // tick is over

    if(need_switch_task || task_n==0) return; // already scheduled context switch

    uint32_t t=stopwatch_getticks();
    next_task = get_next_task();

    tsched_count_t++;
    tsched_time_t+=stopwatch_getticks() - t;

    if(next_task != s_running) { // if we should switch task
        tsched_sw_count_t++;
        do_io_completion(0);    // plan  context switch
    }
#endif
}


void REVOMINIScheduler::yield(uint16_t ttw) // time to wait 
{
#ifdef PREEMPTIVE
    if(task_n==0) {
 #ifdef USE_WFE
        if(ttw) {__WFE(); }
 #endif
        return;
    }

    if(in_interrupt()) return; // SVC causes HardFault if in interrupt so just nothing to do

// if yield() called with a time, then task don't want to run all this time so exclude it from time sliceing
/*
    asm volatile( "mov r0,%0 \n"  // time to sleep in R0
                  "svc 0     \n"::"r"(ttw)); // do scheduler in interrupt
*/
    s_running->ttw=ttw;   // если переключение задачи происходит между записью и вызовом svc, мы всего лишь добавляем лишний тик
    asm volatile("svc 0");

#else

    task_t * me=s_running;
 #ifdef MTASK_PROF
    uint32_t ret;
    asm volatile ("mov %0, lr\n\t"  : "=rm" (ret) );

 #endif
    if(mw->has_semaphore) return; // don't switch out from task that owns a semaphore

    if(task_n==0        || // no tasks
      ( mw->id==0 && ttw && ttw < 10) || // don't mess into delays less than 5uS from main task - 840 steps
      in_interrupt() ) { // don't switch privileged context
 #ifdef USE_WFE
        if(ttw) __WFE();
 #endif
        return;
    }

    me->ttw=ttw; // remember time task wants to wait

    task_t *next;
    
    
    // select task to run next -- let do it in ISR
    asm volatile (     //s_running=get_next_task(); 
        "svc 4        \n"  
        "mov %0, r0   \n"  
        : "=rm" (next) );
    

    if(me == next) return; 

    if (setjmp(me->context)) {
        // we come here via longjmp - context switch is over
        return;
    }
    
    s_running=next; // store task to run next
    
 // all OK, switch context to founded task
    longjmp(s_running->context, true);
    // never comes here, returns after setjmp() above instead
#endif
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

// register IO completion routine
uint8_t REVOMINIScheduler::register_io_completion(Handler handler){

    if(num_io_completion < MAX_IO_COMPLETION){
        io_completion[num_io_completion].handler=handler;
        io_completion[num_io_completion].sem=NULL;
        return ++num_io_completion;
    }
    return 0;
}

void REVOMINIScheduler::PendSV_Handler(){ // isr at lowest priority to do all IO completion routines
    bool do_it = false;

#ifdef SHED_PROF
    uint32_t full_t=_micros();
#endif

    do {
        do_it = false;
        for(uint8_t i=0; i<num_io_completion; i++){
            IO_Completion &io = io_completion[i];
            
            if(io.request) {
                io.request=false; // ASAP - it can be set again in interrupt
                if(io.handler){
                    do_it=true;

#ifdef SHED_PROF
                    uint32_t t = _micros();
#endif
                    revo_call_handler(io.handler,i); // unified way to call handlers
#ifdef SHED_PROF
                    t = _micros() - t;
                    io.time += t;
                    io.count++;
#endif
                }
            }
        }
    } while(do_it);

#ifdef SHED_PROF
    full_t=_micros() - full_t;
    ioc_time += full_t;
#endif

#ifdef MTASK_PROF
// исключить время работы прерывания из времени задачи, которую оно прервало
    s_running->in_isr += full_t;
#endif

}

#pragma GCC optimize ("O2") // should ALWAYS be -O2 for tail recursion optimization in PendSV_Handler


void PendSV_Handler(){

#ifdef PREEMPTIVE
    if( REVOMINIScheduler::need_io_completion) {
        REVOMINIScheduler::need_io_completion=false;
        REVOMINIScheduler::PendSV_Handler();            // do all IO_completion jobs
    }

    if(!REVOMINIScheduler::need_switch_task) return;
    REVOMINIScheduler::need_switch_task=false;


    __do_context_switch();
#else
    REVOMINIScheduler::PendSV_Handler();
#endif
}

void SVC_Handler(){
    uint32_t * svc_args;
    
    asm volatile (
        "TST lr, #4     \n"  
        "ite eq         \n"
        "MRSEQ %0, MSP  \n"  
        "MRSNE %0, PSP  \n"  
        : "=rm" (svc_args) );

    REVOMINIScheduler::SVC_Handler(svc_args);
    
}

// svc executes on same priority as Timer7 ISR so there is no need to prevent interrupts
void REVOMINIScheduler::SVC_Handler(uint32_t * svc_args){
    unsigned int svc_number;    
    //    * Stack contains:    * r0, r1, r2, r3, r12, r14, the return address and xPSR      
    svc_number = ((char *)svc_args[6])[-2];    
//    uint32_t r0=svc_args[0];          First argument (r0) is svc_args[0]
    bool ret;
    switch(svc_number)    {        
    case 0:            // Handle SVC 00 - yield()
//        s_running->ttw=svc_args[0]; // we can do it in yield() itself
        if(svc_args[0]){ // the task voluntarily gave up its quant and wants delay, so that at the end of the delay it will have the highest priority
            s_running->curr_prio = s_running->priority - 6;
        }
        switch_task();
        break;        

    case 1:{            // Handle SVC 01 - semaphore give(semaphore) returns bool
            Semaphore * sem = (REVOMINI::Semaphore *)svc_args[0];
            svc_args[0] = ret = sem->svc_give();
            switch_task(); // switch context to waiting task
        }
        break;

    case 2:{            // Handle SVC 02 - semaphore take(semaphore, time) returns bool
            Semaphore * sem = (REVOMINI::Semaphore *)svc_args[0];
            svc_args[0] = ret = sem->svc_take(svc_args[1]);
            if(!ret)  switch_task(); // if failed switch context to pause waiting task
        }
        break;

    case 3:{            // Handle SVC 03 - semaphore take_nonblocking(semaphore) returns bool
            Semaphore * sem = (REVOMINI::Semaphore *)svc_args[0];
            svc_args[0] = ret = sem->svc_take_nonblocking();
            if(!ret)  switch_task(); // if failed switch context to give tick to semaphore's owner
        }
        break;
    
    case 4: {           // Handle SVC 04 - get next task to run, to prevent any issues
            svc_args[0] = (uint32_t)get_next_task();
        }
        break;
    
    //case 5: // whats more we can do via SVC?

    default:                // Unknown SVC - just ignore
        break;    
    }
}

// prepare task switch and plan it if needed. This function called only on ISR level 14
void REVOMINIScheduler::switch_task(){
    timer_pause(TIMER11);          // we will recalculate scheduling
    timer_generate_update(TIMER7); // tick is over

    if(need_switch_task || task_n==0) return; // already scheduled context switch

#ifdef MTASK_PROF
    uint32_t t=stopwatch_getticks();
#endif
    next_task = get_next_task();

#ifdef MTASK_PROF
    tsched_count_y++;
    tsched_time_y+=stopwatch_getticks() - t;
#endif

    if(next_task != s_running) { // if we should switch task
        tsched_sw_count_y++;
        do_io_completion(0);   // plan context switch
    }

}

////////////////////////////////////
/*
union Revo_handler { // кровь кишки ассемблер :) преобразование функторов в унифицированный вид для вызова из С
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
void hal_delay_us_ny(uint16_t t){ REVOMINIScheduler::_delay_us_ny(t);}

uint32_t hal_micros() { return REVOMINIScheduler::_micros(); }

