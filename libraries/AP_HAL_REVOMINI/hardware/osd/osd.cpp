

#include <AP_HAL/AP_HAL.h>

#ifdef BOARD_OSD_CS_PIN

#include <utility>

#include "osd_core/compat.h"


using namespace REVOMINI;


#include <hal.h>
#include "ring_buffer.h"

#include <inttypes.h>
#include <AP_HAL/AP_HAL.h>
#include <AP_HAL_REVOMINI/AP_HAL_REVOMINI.h>
#include <AP_HAL_REVOMINI/SPIDevice.h>

#include "osd_core/Defs.h"

#include "osd.h"

#include "osd_eeprom.h"
#include "osd_core/eeprom.h"
#include "osd_core/version.h"


namespace OSDns {

#include "osd_core/OSD_Max7456.h"
OSD osd; //OSD object

#include "osd_core/prototypes.h"
#include "osd_core/Config_Func.h"
#include "osd_core/Config.h"
#include "osd_core/Vars.h"
#include "osd_core/Func.h"
#include "osd_core/protocols.h"
#include "osd_core/misc.h"
#include "osd_core/Panels.h"



static ring_buffer osd_rxrb IN_CCM;
static uint8_t osd_rx_buf[OSD_RX_BUF_SIZE] IN_CCM;

AP_HAL::OwnPtr<REVOMINI::SPIDevice> osd_spi;
AP_HAL::Semaphore                *osd_spi_sem;

static volatile byte vas_vsync=false;

mavlink_system_t mavlink_system = {12,1};  // sysid, compid


extern void heartBeat();
extern void writePanels(unsigned long pt);

void On100ms() {}
void On20ms() {}
void osd_loop();
void vsync_ISR();
void max_do_transfer(const char *buffer, uint16_t len);


static bool osd_need_redraw = false;

void osd_begin(AP_HAL::OwnPtr<REVOMINI::SPIDevice> spi){

    osd_spi = std::move(spi);
    
    osd_spi_sem = osd_spi->get_semaphore();

    const stm32_pin_info &pp = PIN_MAP[BOARD_OSD_CS_PIN];
    gpio_set_mode(pp.gpio_device, pp.gpio_bit, GPIO_OUTPUT_PP);
    gpio_write_bit(pp.gpio_device, pp.gpio_bit, HIGH);


    if(osd_spi_sem->take(HAL_SEMAPHORE_BLOCK_FOREVER) ) {
        osd_spi->set_speed(AP_HAL::Device::SPEED_HIGH);
        osd_spi_sem->give();
    }
//    max7456_on();

    rb_init(&osd_rxrb, OSD_RX_BUF_SIZE, osd_rx_buf);

    readSettings();

    if( sets.CHK1_VERSION != VER || sets.CHK2_VERSION != (VER ^ 0x55)) { // wrong version
        lflags.bad_config=1;
    }

#define REL_1 int(RELEASE_NUM/100)
#define REL_2 int((RELEASE_NUM - REL_1*100 )/10) 
#define REL_3 int((RELEASE_NUM - REL_1*100 - REL_2*10  )) 

    if(sets.FW_VERSION[0]!=(REL_1 + '0') || sets.FW_VERSION[1]!=(REL_2 + '0') || sets.FW_VERSION[2]!=(REL_3 + '0') ){
        sets.FW_VERSION[0]=REL_1 + '0';
        sets.FW_VERSION[1]=REL_2 + '0';
        sets.FW_VERSION[2]=REL_3 + '0';

        eeprom_write_len( sets.FW_VERSION,  EEPROM_offs(sets) + ((byte *)sets.FW_VERSION - (byte *)&sets),  sizeof(sets.FW_VERSION) );
    }

        
    OSD::update();// clear memory

    osd.init();    // Start display
        
    logo();
    
    doScreenSwitch(); // set vars for startup screen
    
#ifdef BOARD_OSD_VSYNC_PIN
    Revo_hal_handler h = { .vp = vsync_ISR };
    
    REVOMINIGPIO::_attach_interrupt(BOARD_OSD_VSYNC_PIN, h.h, RISING, 9);
#endif
}


// TODO: all SPI transfers should use register_completion_callback()

void osd_loop() {
    if(osd_need_redraw){ // если была отложенная передача
        osd_need_redraw=false;
        
        OSD::update();           
    }


    uint32_t pt=millis();     //millis_plus(&pt, 0); much larger

    seconds = pt / 1000;

    if(pt < BOOTTIME || lflags.bad_config){ // startup delay for fonts or EEPROM error
            logo();
            return;
    }

#if defined(MAV_REQUEST) && (defined(USE_MAVLINK) || defined(USE_MAVLINKPX4))
    if(apm_mav_system && !lflags.mav_request_done){ // we got HEARTBEAT packet and still don't send requests
        for(byte n = 3; n >0; n--){
            request_mavlink_rates();//Three times to certify it will be readed
            delay_150();
        }
        lflags.mav_request_done=1;
    }
#endif

    if(lflags.got_data){ // были свежие данные - обработать

        pan_toggle(); // проверить переключение экранов

        if(!lflags.need_redraw) {
            lflags.need_redraw=1;
            vsync_wait=1; // будем ждать прерывания
        }

        lflags.got_data=0; // данные обработаны
    }
    
    if( lflags.need_redraw &&  !vsync_wait) { // сразу после прерывания дабы успеть закончить расчет к следующему
        lflags.need_redraw=0; // экран перерисован

        setHomeVars();   // calculate and set Distance from home and Direction to home

        setFdataVars();  // накопление статистики и рекордов

        writePanels(pt);   // writing enabled panels (check OSD_Panels Tab)

        update_screen = 1; // пришли данные, надо перерисовать экран
    }

    if(pt > timer_20ms){
        long_plus(&timer_20ms, 20);
        On20ms();
        
        if(update_screen && vsync_wait && time_since((uint32_t *)&vsync_time)>50){ // прерывания остановились - с последнего прошло более 50мс
            vsync_wait=0; // хватит ждать

            OSD::update(); // обновим принудительно (и далее будем обновлять каждые 20мс)
            update_screen = 0;
        }
    

    }

    if(pt > timer_100ms){
        long_plus(&timer_100ms, 100);
        On100ms(); 

        lflags.flag_01s = !lflags.flag_01s;

        if(lflags.flag_01s) {

            if(skip_inc) {
                skip_inc++;

                if(skip_inc >=3){
                    count02s++;
                    skip_inc=0; // we go again
                }

            } else
                count02s++;
        }
//      count01s++;
    }

    if(pt > timer_500ms){
        long_plus(&timer_500ms, 500);
        lflags.got_data=1; // каждые полсекунды принудительно

        lflags.flag_05s = 1;

        count05s++;

        lflags.blinker = !lflags.blinker;
        if(lflags.blinker) {
    //        seconds++;
            lflags.one_sec_timer_switch = 1; // for warnings

            if(lflags.got_date) day_seconds++; // if we has GPS time - let it ticks

            if( vas_vsync && vsync_count < 5) { // при частоте кадров их должно быть 25 или 50
                                                    // но есть платы где эта нога не подключена. Китай...
                max7456_err_count++;
                if(max7456_err_count>3) { // 3 seconds bad sync
#ifdef DEBUG   
                    Serial.printf_P(PSTR("restart MAX! vsync_count=%d\n"),vsync_count);
#endif
                    osd.reset();    // restart MAX7456
                }
            } else  max7456_err_count=0;

            vsync_count=0;

            heartBeat();

#ifdef DEBUG
            if(seconds % 30 == 0) {
                extern volatile uint16_t lost_bytes;
                Serial.printf_P(PSTR("loop time = %dms lost bytes=%d\n"),max_dly, lost_bytes);
                Serial.printf_P(PSTR("stack bottom = %x\n"),stack_bottom);
//    serial_hex_dump((byte *)0x100, 2048);    // memory 2k, user's from &flags to stack
            }
#endif
        }
    }
}




void vsync_ISR(){
    vas_vsync=true;
    vsync_wait=0;       // отметить его наличие

    vsync_count++; // считаем кадровые прерывания
    vsync_time=millis(); // и отметим его время

    if(update_screen) { // there is data for screen
        OSD::update();          // do it in interrupt! execution time is ~500uS so without interrupts we will NOT lose serial bytes because:
                                    //   on 115200 bit time=1/speed = ~87uS so byte time= ~870uS. ATmega's datasheet says that 
                                    //   receive buffer has 2 bytes, so we have time near 3*870uS = ~2500uS before character loss
        update_screen = 0;
    }
    
#if defined(DEBUG)
    byte sp;

    if(((uint16_t)&sp)<stack_bottom)
        stack_bottom=((uint16_t)&sp);
#endif
}



// is there any chars in ring buffer?
int16_t osd_available(){
    return rb_full_count(&osd_rxrb);
}


void osd_queue(uint8_t c) {
    /* By default, push bytes around in the ring buffer. */
    rb_push_insert(&osd_rxrb, c);
}


// get char from ring buffer
int16_t osd_getc(){
    return rb_remove(&osd_rxrb);
}


// parse char - no buffers
void osd_putc(uint8_t c){
    extern bool mavlink_one_byte(char c);
    
    if(mavlink_one_byte(c)) lflags.got_data=true;
}


void max7456_off(){
    const stm32_pin_info &pp = PIN_MAP[BOARD_OSD_CS_PIN];
    gpio_write_bit(pp.gpio_device, pp.gpio_bit, HIGH);
    
    osd_spi_sem->give();
}

void max7456_on(){
    if(osd_spi_sem->take(100)) {
        const stm32_pin_info &pp = PIN_MAP[BOARD_OSD_CS_PIN];
        gpio_write_bit(pp.gpio_device, pp.gpio_bit, LOW);

        osd_spi->set_speed(AP_HAL::Device::SPEED_HIGH);
    }
}

void MAX_write(byte addr, byte data){
    osd_spi->transfer(addr);
    osd_spi->transfer(data);
}

byte MAX_read(byte addr){
  osd_spi->transfer(addr);
  return osd_spi->transfer(0xff);
}

byte MAX_rw(byte b){
  return osd_spi->transfer(b);
}

void max_do_transfer(const uint8_t *buffer, uint16_t len){
        osd_spi->set_speed(AP_HAL::Device::SPEED_HIGH);

        MAX_write(MAX7456_DMAH_reg, 0);
        MAX_write(MAX7456_DMAL_reg, 0);
        MAX_write(MAX7456_DMM_reg, 1); // автоинкремент адреса
        // DMA
        osd_spi->transfer(buffer, len, NULL, 0);

        osd_spi->transfer(MAX7456_END_string); // 0xFF - "end of screen" character
        max7456_off();
}

void update_max_buffer(const uint8_t *buffer, uint16_t len){
//    max7456_on();
    if(REVOMINIScheduler::in_interrupt()){
        if(osd_spi_sem->take(0)) {
            const stm32_pin_info &pp = PIN_MAP[BOARD_OSD_CS_PIN];
            gpio_write_bit(pp.gpio_device, pp.gpio_bit, LOW);
    
            max_do_transfer(buffer, len);
        } else {
            osd_need_redraw=true; // can't get semaphore in ISR
        }
    } else {
        max7456_on();
        max_do_transfer(buffer, len);
    }
}



} // namespace


#endif