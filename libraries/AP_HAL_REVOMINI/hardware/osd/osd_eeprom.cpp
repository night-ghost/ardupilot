#include "osd_eeprom.h"

#include <AP_HAL/AP_HAL.h>

extern const AP_HAL::HAL& hal;

#include <utility>
#include <util.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

uint32_t OSD_EEPROM::ee_ptr=0;

void OSD_EEPROM::init(){

}

uint8_t OSD_EEPROM::read(uint16_t addr){
    
    for(uint8_t i=PAGE_SIZE/EEPROM_SIZE; i!=0;){
        // look most recent value from last stage
        i--;
        uint32_t ea = EEPROM_PAGE + i*EEPROM_SIZE + addr;
        uint8_t val = read_8(ea);
        if(val != 0xFF) {
            ee_ptr=ea;  // remember last read address
            return val;     // first non-FF is a value
        }
    }
    ee_ptr = EEPROM_PAGE + addr;
    return 0xff; // got to begin and still FF - really FF
}

void OSD_EEPROM::write(uint16_t addr, uint8_t val){
    uint8_t cv = read(addr);
    if(cv == val) return; // already is

    FLASH_Unlock();
    
    if( /* we can write - there is no '0' where we need '1' */ (~cv & val)==0 ){
        write_8(ee_ptr, val);    // just overwrite last value
        goto done;
    }

    if(val != 0xFF){ // the only way to write FF is to clear all
        for(uint8_t i=0; i<PAGE_SIZE/EEPROM_SIZE; i++){
            // look 0xFF 
            uint32_t ea = EEPROM_PAGE + i*EEPROM_SIZE + addr;
            cv = read_8(ea);
            if(cv == 0xFF) { // empty 
                write_8(ea, val);
                goto done;
            }
        }
    }

    // no empty slots - so need to erase page
    { // isolate "data"
// 1st copy all data to RAM
//         uint8_t data[EEPROM_SIZE]; //this executes in task so stack is very limited!
        uint8_t *data = (uint8_t *)malloc(EEPROM_SIZE);
        if(data==NULL) goto done; // no memory
    
        for(uint16_t i=0;i<EEPROM_SIZE; i++){
            data[i] = read(i);
        }
    
        data[addr] = val; // write value
    
// 2nd - erase page. power loss here cause data loss! In execution time CPU is frozen!
        hal.console->printf("\nEEprom_OSD erase page %d\n ", (uint16_t)((EEPROM_PAGE & 0x00ffffff) / 0x4000) ); // clear high byte of address and count 16K blocks
        erasePageByAddress(EEPROM_PAGE); 

// 3rd write data back to the beginning of Flash page
        for(uint16_t i=0;i<EEPROM_SIZE; i++){
            write_8(EEPROM_PAGE+i, data[i]);
        }
    
        free(data);
    }
done:
    FLASH_Lock_check();
}

