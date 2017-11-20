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

Revo_IO    REVOMINIScheduler::_io_proc[REVOMINI_SCHEDULER_MAX_IO_PROCS] IN_CCM;
uint8_t    REVOMINIScheduler::_num_io_proc IN_CCM=0;


AP_HAL::Proc REVOMINIScheduler::_delay_cb IN_CCM=NULL;
uint16_t REVOMINIScheduler::_min_delay_cb_ms IN_CCM=0;
void *   REVOMINIScheduler::_delay_cb_handle IN_CCM=0;

uint32_t REVOMINIScheduler::timer5_ovf_cnt IN_CCM=0;

bool REVOMINIScheduler::_initialized IN_CCM=false;

Handler REVOMINIScheduler::on_disarm_handler IN_CCM;
task_t * REVOMINIScheduler::_idle_task IN_CCM;

volatile Handler REVOMINIScheduler::_delay_timer_proc IN_CCM=0;


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
 uint32_t REVOMINIScheduler::tsched_count IN_CCM;
 uint32_t REVOMINIScheduler::tsched_sw_count IN_CCM;
 uint32_t REVOMINIScheduler::tsched_count_y IN_CCM;
 uint32_t REVOMINIScheduler::tsched_sw_count_y IN_CCM;
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
#ifdef MPU_DEBUG
uint32_t REVOMINIScheduler::MPU_overflow_cnt IN_CCM;
uint32_t REVOMINIScheduler::MPU_restart_cnt IN_CCM;
uint32_t REVOMINIScheduler::MPU_count IN_CCM;
uint32_t REVOMINIScheduler::MPU_Time IN_CCM;
#endif
volatile bool REVOMINIScheduler::need_switch_task IN_CCM;

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
        __WFE(); 
        REVOMINIScheduler::yield(0);
    }
}

void REVOMINIScheduler::init()
{
    if(in_interrupt()){ // some interrupt caused restart at ISR level
        AP_HAL::panic("HAL initialization on ISR level=0x%x", (uint8_t)(SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk));
    }
    memset(_io_proc,      0, sizeof(_io_proc) );
    memset(io_completion, 0, sizeof(io_completion) );


    // The PendSV exception is always enabled so set PRIMASK 
    // to prevent it from occurring while being configured 
    noInterrupts();

    NVIC_SetPriority(PendSV_IRQn,  PENDSV_INT_PRIORITY);      // lowest priority so all IRQs can't be switced
    NVIC_SetPriority(SVCall_IRQn,  SVC_INT_PRIORITY);         // priority 14 - the same as Timer7 ISR
    NVIC_SetPriority(SysTick_IRQn, SYSTICK_INT_PRIORITY);     // priority 5 - less thah fast device IO ISRs but higher than USB


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


    {// timeslice timer, not SYSTICK because we need to restart it by hands
        uint32_t period    = (2000000UL / SHED_FREQ) - 1; 
                // dev    period   freq, kHz
        configTimeBase(TIMER7, period, 2000);       //2MHz 0.5us ticks
        Revo_handler h = { .isr = _timer_isr_event };
        timer_attach_interrupt(TIMER7, TIMER_UPDATE_INTERRUPT, h.h , SVC_INT_PRIORITY); // almost lowest priority, higher than Pend_SW to schedule task switch
        TIMER7->regs->CR1 |=  TIMER_CR1_URS;  // interrupt only by overflow, not by update
        timer_resume(TIMER7);
    }

    {// timer5 - 32-bit general timer, unused for other needs
     // so we can read micros32() directly from its counter and micros64() from counter and overflows
        configTimeBase(TIMER5, 0, 1000);       //1MHz 1us ticks
        timer_set_count(TIMER5,(1000000/SHED_FREQ)/2); // to not interfere with TIMER7
        Revo_handler h = { .isr = _timer5_ovf };
        timer_attach_interrupt(TIMER5, TIMER_UPDATE_INTERRUPT, h.h, MPU_INT_PRIORITY); // high priority
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
        configTimeBase(TIMER14, 0, 1000);       //1MHz 1us ticks
        Revo_handler h = { .isr = _tail_timer_event };
        timer_attach_interrupt(TIMER14, TIMER_UPDATE_INTERRUPT, h.h , SVC_INT_PRIORITY); // priority 14 - the same as Timer7 and SVC
        TIMER14->regs->CR1 &= ~(TIMER_CR1_ARPE | TIMER_CR1_URS); // not buffered preload, interrupt by overflow or by UG set
    }


    { // timer to generate interrupt for driver's IO_Completion
                // dev    period   freq, kHz
        configTimeBase(TIMER13, 0, 1000);       //1MHz 1us ticks
        Revo_handler h = { .isr = _ioc_timer_event };
        timer_attach_interrupt(TIMER13, TIMER_UPDATE_INTERRUPT, h.h , IOC_INT_PRIORITY); // priority 12
        TIMER13->regs->CR1 &= ~(TIMER_CR1_ARPE | TIMER_CR1_URS); // not buffered preload, interrupt by overflow or by UG set
    }

    { // timer to execute function after some delay
                // dev    period   freq, kHz
        configTimeBase(TIMER11, 0, 1000);      //1MHz 1us ticks
        Revo_handler h = { .isr = _delay_timer_event };
        timer_attach_interrupt(TIMER11, TIMER_UPDATE_INTERRUPT, h.h , IOC_INT_PRIORITY); // priority as IO_Complete
        TIMER11->regs->CR1 &= ~(TIMER_CR1_ARPE ); // not buffered preload
        TIMER11->regs->CR1 |= TIMER_CR1_URS;      // interrupt only by overflows
    }


    void *task = _start_task((uint32_t)idle_task, 256); // only for one context
    set_task_priority(task, 255); // lowest possible, to fill delay()
    _idle_task=(task_t *)task;
    set_task_active(task); // tasks without period are created paused so run it

}

// it can't be started on init() because should be stopped in later_init()
void REVOMINIScheduler::start_stats_task(){
#ifdef DEBUG_BUILD
// show stats output each 10 seconds
    Revo_handler h = { .vp = _set_10s_flag };
    void *task = _register_timer_task(10000000, h.h, NULL);
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
//    set_task_priority(s_running, s_running->priority + 2); this is useless because task already paused for given time
    _delay_microseconds(us);
//    set_task_priority(s_running, s_running->priority - 2);
}

#define NO_YIELD_TIME 40 // uS

void REVOMINIScheduler::_delay_microseconds(uint16_t us)
{
    uint32_t rtime = stopwatch_getticks(); // get start ticks first

#ifdef SHED_PROF
    uint32_t t = _micros(); 
#endif
    task_t *me=s_running;
    
    uint16_t no_yield_t;    // guard time for main process
//    no_yield_t=us/20;   // 5%
//    if(me->id==0 && no_yield_t<NO_YIELD_TIME) 
        no_yield_t=NO_YIELD_TIME;

    uint32_t dt = us_ticks * us;  // delay time in ticks
    uint32_t ny = us_ticks * no_yield_t; // no-yield time in ticks
    
    uint32_t tw;

    while ((tw = stopwatch_getticks() - rtime) < dt) { // tw - time waiting, in ticks
        if((dt - tw) > ny ) { // No Yeld time - some uS to end of wait 
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
        }
    }

    _in_io_proc = false;
}


void REVOMINIScheduler::_register_io_process(Handler h, Revo_IO_Flags flags)
{
    if(_num_io_proc>=REVOMINI_SCHEDULER_MAX_IO_PROCS) return;

    if(_num_io_proc==0){
        void *task = start_task(_run_io, IO_STACK_SIZE);
        set_task_period(task, 500); 
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
    static uint32_t last_timer_procs=0;

    uint32_t now = _micros();

    if(now - last_timer_procs >= 1000) {
        last_timer_procs = now;
        _run_timer_procs(true);
    }


#ifndef MTASK_PROF
    _switch_task();
#else
    
    if(task_n==0 || need_switch_task) return;        // if there no tasks
    
    next_task = get_next_task();
    tsched_count++;

    if(next_task != s_running) { // if we should switch task
        s_running->sw_type=0;
        tsched_sw_count++;
        plan_context_switch();     // plan context switch
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

extern "C" {
    extern  void *__brkval;
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

            printf("\nSched stats:\n  %% of full time: %5.2f  Efficiency %5.3f max loop time %ld\n", (task_time/10.0)/t /* in percent*/ , shed_eff, max_loop_time);
            printf("delay times: in main %5.2f including in timer %5.2f",         (delay_time/10.0)/t, (delay_int_time/10.0)/t);
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
#endif
    
            }break;
            
        case 2:{
#ifdef MTASK_PROF    
            task_t* ptr = &s_main;

            uint32_t  fc=tsched_count+tsched_count_y+tsched_count_t;
            printf("\nsched time: by timer %5.2f%% sw %5.2f%% in yield %5.2f%% sw %5.2f%% in tails %5.2f%% sw %5.2f%%\n", 100.0*tsched_count/fc, 100.0 * tsched_sw_count/tsched_count, 100.0*tsched_count_y/fc,100.0 * tsched_sw_count_y/tsched_count_y, 100.0*tsched_count_t/fc, 100.0 * tsched_sw_count_t/tsched_count_t);
        
            do {
                printf("task %d (0x%015llx) time: %7.2f%% mean %8.1fuS max %5lduS full %7lduS in %6ld ticks (mean %8.1fuS) %7.2f%%  wait sem. %6lduS\n",  
                          ptr->id, ptr->handle, 100.0 * ptr->time/1000.0 / t, 
                                                              (float)ptr->time / ptr->count, 
                                                                       ptr->max_time, 
                                                                                  ptr->work_time, ptr->quants,(float)ptr->quants_time/ptr->quants, 
                                                                                                                           (float)ptr->quants_time/ptr->work_time*100.0, 
                                                                                                                                            ptr->sem_max_wait);
        
                ptr->max_time=0; // reset times
                ptr->work_time=0;
                ptr->sem_max_wait=0;
                ptr->quants=0;
                ptr->quants_time=0;
                ptr->max_paused=0;
                
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
            printf("\nIO completion %7.3f%%\n",  ioc_time/1000.0/t*100);
            uint64_t iot=0;
            for(uint8_t i=0; i<num_io_completion; i++){
                struct IO_COMPLETION &io = io_completion[i];
                
                if(io.handler) {
                    if(io.count){
                        printf("task %llx time %7.3f%% mean %7.3fuS max %lduS\n", io.handler,  100.0 * io.time / t / 1000, (float)io.time/io.count, io.max_time);
                        io.max_time=0; 
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

void REVOMINIScheduler::_set_10s_flag(){
    flag_10s=true;
    _print_stats();
}
#endif

/*
[    common realization of all Device.PeriodicCallback;
*/

AP_HAL::Device::PeriodicHandle REVOMINIScheduler::_register_timer_task(uint32_t period_us, Handler proc, REVOMINI::Semaphore *sem){

    // all drivers will runs at individual IO tasks
    void *task = _start_task(proc, SMALL_TASK_STACK);
    if(task){
        set_task_priority(task, DRIVER_PRIORITY);
        set_task_semaphore(task, sem);
        set_task_period(task, period_us); // setting of period allows task to run
    }
    return (AP_HAL::Device::PeriodicHandle)task;
}




bool REVOMINIScheduler::adjust_timer_task(AP_HAL::Device::PeriodicHandle h, uint32_t period_us)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align" // yes I know

    task_t *p = (task_t *)h;
#pragma GCC diagnostic pop
    p->period = period_us;
    return true;
}

bool REVOMINIScheduler::unregister_timer_task(AP_HAL::Device::PeriodicHandle h)
{
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wcast-align"
    task_t *p = (task_t *)h;
 #pragma GCC diagnostic pop

    noInterrupts(); // 64-bits should be 
    p->handle=0L;
    interrupts();
    return true;
}
// ]


//[ -------- realization of preemptive multitasking --------

bool REVOMINIScheduler::adjust_stack(size_t stackSize)
{  // Set main task stack size
  s_top = stackSize;
  return true;
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
        noInterrupts();    
        tp->handle = 0;
        interrupts();
    }
}

#endif

// task's executor, which calls user's function having semaphore
void REVOMINIScheduler::do_task(task_t *task) {
    while(1){
        uint32_t t=0;
        if(task->handle && task->active) { // Task Switch occures asyncronously so we should wait until task becomes active again
            task->time_start=_micros();
            if(task->sem) {// if task requires a semaphore - block on it
                if(!task->sem->take(HAL_SEMAPHORE_BLOCK_FOREVER)) {
                    yield(0);   // can't be
                    continue;
                }
            }
            revo_call_handler(task->handle, task->id);  // calls user's function
            if(task->sem){
                task->sem->give(); // give semaphore when task finished
            }
#ifdef MTASK_PROF
            t = _micros()-task->time_start; // execution time
            if(t > task->work_time)  task->work_time=t;
            if(task->t_paused > task->max_paused) {
                task->max_paused = task->t_paused;
            }
            if(task->count_paused>task->max_c_paused) {
                task->max_c_paused = task->count_paused;
            }
            task->count_paused=0;
            task->t_paused=0;
#endif
            task->active=false;                    // turn off active, to know when task is started again. last! or can never give semaphore
            task->curr_prio = task->priority - 6;  // just activated task will have a highest priority for one quant
        }
        yield(0);        // give up quant remainder
    }// endless loop
}

// Create task descriptor and add it last to run queue
uint32_t REVOMINIScheduler::fill_task(task_t &tp){
    memset(&tp,0,sizeof(tp));

    // firstly fill required fields

    tp.priority=100;  // default priority equal to main task
    tp.curr_prio=100; // current priority the same
#ifdef MTASK_PROF
    tp.start=_micros(); 
#endif
    tp.guard = STACK_GUARD;

    // now we can add new task to run queue
    
    noInterrupts(); // we will break linked list so do it in critical section
    tp.next = &s_main;  // insert task to linked list
    tp.prev = s_main.prev;
    s_main.prev->next = &tp;
    s_main.prev = &tp;
    tp.id = ++task_n; // counter - new task is created
    interrupts();       // now TCB is ready to task scheduler, active==0 excludes task from execution so context will be prepared later

    return (uint32_t)&tp;
}

// create task descriptor and context
void * REVOMINIScheduler::init_task(Handler handler, const uint8_t* stack){
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align" // yes I know

    task_t *task = (task_t *)((uint32_t)(stack-sizeof(task_t)) & 0xFFFFFFFCUL); // control block below top of stack, 4-byte alignment
#pragma GCC diagnostic pop

    fill_task(*task);  // Add task as last to run queue (starting main task), task will not be executed because active==0
    task->stack  = stack;

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

    // task is not active so we need not to disable interrupts
    task->handle = handler; // save handler to task to enable to change it later

    return (void *)task;
}



// create a paused task
void * NOINLINE REVOMINIScheduler::_start_task(Handler handle, size_t stackSize)
{
    // Check called from main task
    if (!_in_main_thread() ) return NULL; 
    
    if(in_interrupt()) {
        AP_HAL::panic("start_task called from ISR 0x%x", (uint8_t)(SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk));
    }


    // Adjust stack size with size of task context
    stackSize += sizeof(task_t)+8; // for alignment

    // we don't need to use arrays because we will prepare context
    if (s_main.stack == NULL) {       // first call, initialize all task subsystem
        s_main.stack = (const uint8_t*)RAMEND - s_top; // remember bottom of stack of main task on first call
    }

    const uint8_t *sp=(const uint8_t*)s_main.prev->stack; // top of stack for new task    

    task_t *task=(task_t *) init_task(handle, sp);    // give stack top as parameter, will correct later
    sp-= stackSize;               // calc stack bottom
    task->stack = sp;             // correct to bottom of stack
    stack_bottom = (caddr_t)sp;   // and remember for memory allocator
    s_top += stackSize;           // adjust used size at stack top

    // task->active = true;          // now task is ready to run but let it stays paused
    return (void *)task; // return address of task descriptor
}

// task should run periodically, period in uS. this will be high-priority task
void REVOMINIScheduler::set_task_period(void *h, uint32_t period){
    task_t *task = (task_t *)h;

    task->active = false; // will bi first started after 'period'
    task->time_start  = _micros();
    task->period = period;
}

void REVOMINIScheduler::set_task_priority(void *h, uint8_t prio){
    task_t *task = (task_t *)h;

    task->curr_prio= prio;
    task->priority = prio;
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

    uint32_t timeFromLast=0;
    uint32_t remains = 0;

    uint32_t partial_quant=(uint32_t)-1;
    task_t *want_tail = NULL;

    uint32_t now =  _micros();
    me->t_yield = now;

    { // isolate dt
#if defined(MTASK_PROF) 
        uint32_t dt =  now - me->start;       // time in task
        if(dt >= me->in_isr) dt -= me->in_isr;  // minus time in interrupts
        else                 dt=0;

        me->time+=dt;                           // calculate sum
        me->quants_time+=dt;
#endif


#ifdef MTASK_PROF
        if(dt > me->max_time) {
            me->max_time = dt; // maximum to show
//            me->maxt_addr = ret;
        }

 #ifdef SHED_DEBUG
        {
            revo_sched_log &lp = logbuf[sched_log_ptr];
            lp.end = now;
            lp.task_id=me->id;
            lp.ttw = me->ttw;
            lp.in_isr = me->in_isr;
            lp.sw_type=me->sw_type;
            sched_log_ptr = next_log_ptr(sched_log_ptr);
            ZeroIt(logbuf[sched_log_ptr]); // clear next
        }
 #endif
#endif
    }

    task_t *ptr = me; // starting from current task

    while(true) { // lets try to find task to switch to
        ptr = ptr->next; // Next task in run queue will continue

#if 0
        if(!(ADDRESS_IN_RAM(ptr) || ADDRESS_IN_CCM(ptr)) ){
            AP_HAL::panic("PANIC: s_rinning spoiled in process %d\n", me->id);
        }
#endif
            
        if(!ptr->handle) goto skip_task; // skip finished tasks
        
//        if(ptr->f_yield) {
//            ptr->f_yield=false; // only once
//            goto skip_task;     // this allows to execute low-priority tasks 
//        }
        
        if(ptr->sem_wait) { // task want a semaphore
            if(ptr->sem_wait->is_taken()) { // task blocked on semaphore
                task_t *own =(task_t *)ptr->sem_wait->get_owner();
                if(own != ptr) { // owner is another task?
                    uint32_t dt = now - ptr->sem_start_wait;   // time since start waiting
                    if(ptr->sem_time == HAL_SEMAPHORE_BLOCK_FOREVER || dt < ptr->sem_time) {
                        if(ptr->curr_prio>1) ptr->curr_prio--;      // increase priority as task waiting for a semaphore
                        if(own->curr_prio > ptr->curr_prio) {
                            own->curr_prio=ptr->curr_prio;
                        }
                        goto skip_task; 
                    }
                }
            } 
            ptr->sem_wait=NULL; // clear semaphore after release
#ifdef MTASK_PROF
            uint32_t st=now-ptr->sem_start_wait;
            if(st>ptr->sem_max_wait) ptr->sem_max_wait=st; // time of semaphore waiting
#endif            
        }
            
        
        if(!ptr->active){ // non-active, is it periodic?
            if(ptr->period){ 
                if(_timer_suspended) goto skip_task; //       timed tasks can't be started
                
                timeFromLast = now - ptr->time_start; // time from last run
                if( timeFromLast < ptr->period) {     //   is less than task's period?
                    remains = ptr->period - timeFromLast;
                    if(remains>10) {
                        if(remains<partial_quant) {
                            partial_quant=remains; // minimal time remains to next task
                            want_tail = ptr;
                        }
                        goto skip_task; 
                    }// else execute task slightly before
                }
            } else { // non-active non-periodic tasks with manual activation
                goto skip_task; // should be skipped
            }
            ptr->active=true; // selected task to run, even if it lose quant by priority

        } else { // обычный тайм слайс

            if(ptr->ttw){// task wants to wait 
                timeFromLast = now - ptr->t_yield;     // time since that moment
                if(timeFromLast < ptr->ttw){           // still less than ttw ?
                    remains = ptr->ttw - timeFromLast; // remaining time to wait
                    if(remains>4) { // context switch time
                        if(remains<partial_quant) {
                            partial_quant=remains;
                            want_tail = ptr;
                        }
                        goto skip_task; 
                    }// else execute task slightly before
                }
            } 
        }

        if(ptr->curr_prio <= task->curr_prio){ // select the most priority task, round-robin for equal priorities
            // task loose tick
            if(task->priority != 255) { // not for idle task
                if(task->curr_prio>1)  task->curr_prio--;      // increase priority if task loose tick
// в результате роста приоритета ожидающей задачи мы не останавливаем низкоприоритетные задачи полностью, а лишь замедляем их
// выполнение, заставляя пропустить число тиков, равное разности приоритетов. В результате низкоприоритетная задача выполняется на меньшей скорости,
// которую можно настраивать изменением разницы приоритетов
            }
            task = ptr; // winner
        }
skip_task:
    // we should do this check after EACH task so can't use "continue" which skips ALL loop. 
    // And we can't move this to begin of loop because then interrupted task does not participate in the comparison of priorities
        if(ptr == me) { 
            break;   // 'me' is the task that works now, so full loop - now we have most-priority task so let it run!
        }
    }

 #ifdef SHED_DEBUG
    revo_sched_log &lp = logbuf[sched_log_ptr];
    lp.start = now;
    lp.task_id=task->id;
    lp.prio = task->curr_prio;
    lp.active = task->active;
    lp.time_start = task->time_start;
    lp.quant = partial_quant;
    lp.want_tail = want_tail;
    ZeroIt(logbuf[next_log_ptr(sched_log_ptr)]); // clear next
 #endif

    task->curr_prio=task->priority; // reset current priority to default value
    task->ttw=0;        // time to wait is over
#if defined(MTASK_PROF) 
    task->start = now;  // task startup time
    task->in_isr=0; // reset ISR time
    task->count++;     // full count
    task->quants++;    // one-start count
#endif

    // выбрали задачу для переключения. 

    // проверим сохранность дескриптора
    if(task->guard != STACK_GUARD){
        // TODO исключить задачу из планирования
        AP_HAL::panic("PANIC: stack guard spoiled in process %d (from %d)\n", task->id, me->id);
    }

    if(want_tail) { // we have a task that want to be started next in the middle of tick
        if(partial_quant < TIMER_PERIOD-10) { // if time less than tick
            timer_set_count(TIMER14, 0);
            timer_set_reload(TIMER14, partial_quant+2); // +2 to guarantee
            timer_resume(TIMER14);
        }
    }

    return task;
}


void REVOMINIScheduler::context_switch_isr(){
    timer_generate_update(TIMER14); 
}


/*
    interrupt to reduce timeslice quant
*/
void REVOMINIScheduler::_tail_timer_event(uint32_t v /*TIM_TypeDef *tim */){
    timer_pause(TIMER14);
    timer_generate_update(TIMER7); // tick is over
#ifndef MTASK_PROF
    _switch_task();
#else

    if(need_switch_task || task_n==0) return; // already scheduled context switch

    next_task = get_next_task();

    tsched_count_t++;

    if(next_task != s_running) { // if we should switch task
        s_running->sw_type=1;
        tsched_sw_count_t++;
        plan_context_switch();    // plan context switch
    }
#endif
}


void REVOMINIScheduler::yield(uint16_t ttw) // time to wait 
{
    if(task_n==0 || in_interrupt()) { // SVC causes HardFault if in interrupt so just nothing to do
 #ifdef USE_WFE
        if(ttw) {__WFE(); }
 #endif
        return;
    }

// if yield() called with a time, then task don't want to run all this time so exclude it from time sliceing
/*
    asm volatile( "mov r0,%0 \n"  // time to sleep in R0
                  "svc 0     \n"::"r"(ttw)); // do scheduler in interrupt
*/
    if(ttw) { // ttw cleared on sleep exit so always 0 if not set specially 
        s_running->ttw=ttw;   // если переключение задачи происходит между записью и вызовом svc, мы всего лишь добавляем лишний тик
    }
    asm volatile("svc 0");
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
        io_completion[num_io_completion].request=false;
        return ++num_io_completion;
    }
    return 0;
}

void REVOMINIScheduler::_ioc_timer_event(uint32_t v){ // isr at low priority to do all IO completion routines
    bool do_it = false;

#ifdef SHED_PROF
    uint32_t full_t=_micros();
#endif

    do {
        do_it = false;
        for(uint8_t i=0; i<num_io_completion; i++){
            IO_Completion &io = io_completion[i];
            
            if(io.request) {
                io.request=false; // ASAP - it can be set again in interrupt. 
                // we don't need to disable interrupts because all drivers has own queue and can survive a skipping of one request
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
                    if(t>io.max_time) io.max_time=t;
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
    REVOMINIScheduler::need_switch_task=false;
    __do_context_switch();
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
    //    * Stack contains:    * r0, r1, r2, r3, r12, r14, the return address and xPSR      
    unsigned int svc_number = ((char *)svc_args[6])[-2];    

    bool ret;
    switch(svc_number)    {        
    case 0:            // Handle SVC 00 - yield()
//        s_running->ttw=svc_args[0]; // we can do it in yield() itself
        if(s_running->priority!=255){
            if(s_running->ttw){ // the task voluntarily gave up its quant and wants delay, so that at the end of the delay it will have the high priority
                s_running->curr_prio = s_running->priority - 6;
            } else {
                s_running->curr_prio = s_running->priority + 2; // to guarantee that quant will not return if there is equal priority tasks
                // popular I2C wait time is 300us so let skip 2 quants
            }
        }
        switch_task();
        break;        

    case 1:{            // Handle SVC 01 - semaphore give(semaphore) returns bool
            Semaphore * sem = (REVOMINI::Semaphore *)svc_args[0];
            bool v=sem->is_waiting();            
            svc_args[0] = sem->svc_give();
            if(v) switch_task(); // switch context to waiting task if any
        }
        break;

    case 2:{            // Handle SVC 02 - semaphore take(semaphore, time) returns bool
            Semaphore * sem = (REVOMINI::Semaphore *)svc_args[0];
            uint32_t timeout_ms = svc_args[1];
            svc_args[0] = ret = sem->svc_take(timeout_ms);
            if(!ret)  {// if failed - switch context to pause waiting task
                task_t *own = (task_t *)sem->get_owner();
                task_t *curr_task = s_running;
                curr_task->sem_wait = sem;             // semaphore
                curr_task->sem_start_wait = _micros(); // time when waiting starts
                if(timeout_ms == HAL_SEMAPHORE_BLOCK_FOREVER){
                    curr_task->sem_time = timeout_ms;
                } else {
                    curr_task->sem_time = timeout_ms*1000;      // time to wait semaphore
                }
                //Increase the priority of the semaphore's owner up to the priority of the current task
                if(own->priority >= curr_task->priority) own->curr_prio = curr_task->priority-1;
                switch_task(); 
            }
        }
        break;

    case 3:{            // Handle SVC 03 - semaphore take_nonblocking(semaphore) returns bool
            Semaphore * sem = (REVOMINI::Semaphore *)svc_args[0];
            svc_args[0] = ret = sem->svc_take_nonblocking();
            if(!ret)  { // if failed - switch context to give tick to semaphore's owner
                task_t *own = (task_t *)sem->get_owner();
                task_t *curr_task = s_running;
                //Increase the priority of the semaphore's owner up to the priority of the current task
                if(own->priority >= curr_task->priority) own->curr_prio = curr_task->priority-1;

                switch_task(); 
            }
        }
        break;
    
    
    //case 4: // whats more we can do via SVC?

    default:                // Unknown SVC - just ignore
        break;    
    }
}

// prepare task switch and plan it if needed. This function called only on ISR level 14
void REVOMINIScheduler::switch_task(){
    timer_pause(TIMER14);          // we will recalculate scheduling
    timer_generate_update(TIMER7); // tick is over
    _switch_task();
}

void REVOMINIScheduler::_switch_task(){
    if(need_switch_task || task_n==0) return; // already scheduled context switch

    next_task = get_next_task(); // 2.5uS mean full time

#ifdef MTASK_PROF
    tsched_count_y++;
#endif

    if(next_task != s_running) { // if we should switch task
#ifdef MTASK_PROF
        s_running->sw_type=2;
        tsched_sw_count_y++;
#endif
        plan_context_switch();   // plan context switch
    }
#ifdef MTASK_PROF
      else if(next_task == _idle_task){ // the same task
        tsched_count_y--; // don't count loops in idle task
    }
#endif
}


void REVOMINIScheduler::_delay_timer_event(uint32_t v /*TIM_TypeDef *tim */){
    timer_pause(TIMER11);
    timer_set_count(TIMER11, 0);

    Handler h = _delay_timer_proc;
    _delay_timer_proc=0;
    revo_call_handler(h,0);
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
        (h.mpa)(arg);
    }
}

void hal_yield(uint16_t ttw){ REVOMINIScheduler::yield(ttw); }
void hal_delay(uint16_t t){   REVOMINIScheduler::_delay(t); }
void hal_delay_microseconds(uint16_t t){ REVOMINIScheduler::_delay_microseconds(t);}
void hal_delay_us_ny(uint16_t t){ REVOMINIScheduler::_delay_us_ny(t);}

uint32_t hal_micros() { return REVOMINIScheduler::_micros(); }
void hal_isr_time(uint32_t t) { s_running->in_isr += t; }


// task management for USB
void hal_set_task_active(void * h) { REVOMINIScheduler::set_task_active(h); } 
void hal_context_switch_isr() { REVOMINIScheduler::context_switch_isr(); }
void hal_set_task_priority(void * h, uint8_t prio) {REVOMINIScheduler::set_task_priority(h, prio); }
void * hal_register_task(voidFuncPtr task, uint32_t stack) {
    Revo_handler r = { .vp=task };
    return REVOMINIScheduler::_start_task(r.h, stack);
}

//void hal_panic(const char *errormsg, ...) { AP_HAL::pa}