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
/*
  This uses 2*16k pages of FLASH ROM to emulate an EEPROM
  This storage is retained after power down, and survives reloading of firmware via bootloader
  All multi-byte accesses are reduced to single byte access so that can span EEPROM block boundaries
  
  http://www.st.com/content/ccc/resource/technical/document/application_note/ec/dd/8e/a8/39/49/4f/e5/DM00036065.pdf/files/DM00036065.pdf/jcr:content/translations/en.DM00036065.pdf
 
  problems of such design
  http://ithare.com/journaled-flash-storage-emulating-eeprom-over-flash-acid-transactions-and-more-part-ii-existing-implementations-by-atmel-silabs-ti-stm-and-microchip/

"partial write" problem fixed by requiring that highest bit of address should be 0

 */
#include <AP_HAL/AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_REVOMINI

#pragma GCC optimize ("O2")

#include <string.h>
#include "Storage.h"
#include "EEPROM.h"

using namespace REVOMINI;

extern const AP_HAL::HAL& hal;

// The EEPROM class uses 2x16k FLASH ROM pages to emulate up to 8k of EEPROM.



// This is the size of each FLASH ROM page
const uint32_t pageSize  = 0x4000; // real page size

// This defines the base addresses of the 2 FLASH ROM pages that will be used to emulate EEPROM
// These are the 2 16k pages in the FLASH ROM address space on the STM32F4 used by HAL
// This will effectively provide a total of 8kb of emulated EEPROM storage
const uint32_t pageBase0 = 0x08008000; // Page2
const uint32_t pageBase1 = 0x0800c000; // Page3

// it is possible to move EEPROM area to sectors 1&2 to free sector 3 for code (firmware from 0x0800c000)
// or use 3 sectors for EEPROM as wear leveling

static EEPROMClass eeprom;

REVOMINIStorage::REVOMINIStorage()
{}

void REVOMINIStorage::init()
{
    eeprom.init(pageBase1, pageBase0, pageSize);
}

void REVOMINIStorage::format_eeprom(void) {  
    eeprom.format(); 
}

static void error_parse(uint16_t status){
    switch(status) {
    case EEPROM_NO_VALID_PAGE: // несмотря на неоднократные попытки, EEPROM не работает, а должен
        AP_HAL::panic("EEPROM Error: no valid page\r\n");
        break;

    case EEPROM_OUT_SIZE:
        AP_HAL::panic("EEPROM Error: full\r\n");
        break;
        
    case EEPROM_BAD_FLASH: // 
        AP_HAL::panic("EEPROM Error: page not empty after erase\r\n");
        break;

    case EEPROM_WRITE_FAILED:
        AP_HAL::panic("EEPROM Error: write failed\r\n");
        break;

    case EEPROM_BAD_ADDRESS: // just not found
    case EEPROM_NOT_INIT:    // can't be
    default:
        break; // all OK
    }
}

uint8_t REVOMINIStorage::read_byte(uint16_t loc){

    // 'bytes' are packed 2 per word
    // Read existing dataword and use upper or lower byte

    uint16_t data;
    error_parse( eeprom.read(loc >> 1, &data) );

    if (loc & 1)
	return data >> 8; // Odd, upper byte
    else
	return data & 0xff; // Even lower byte
}

uint16_t REVOMINIStorage::read_word(uint16_t loc){
    uint16_t value;
    if(loc & 1) { // from odd address
        read_block(&value, loc, sizeof(value));
    } else { // from even address - as word
        error_parse( eeprom.read(loc >> 1, &value));
    }
    return value;
}


void REVOMINIStorage::read_block(void* dst, uint16_t loc, size_t n) {
    // Treat as a block of bytes
    uint8_t *ptr_b=(uint8_t *)dst;
    
    if(loc & 1){
        *ptr_b++ = read_byte(loc++);
        n--;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
    uint16_t *ptr_w=(uint16_t *)ptr_b;
#pragma GCC diagnostic pop
    
    while(n>=2){
        error_parse( eeprom.read(loc >> 1, ptr_w++) );
        loc+=2;
        n-=2;
    }
    
    if(n){
        ptr_b=(uint8_t *)ptr_w;
        *ptr_b = read_byte(loc);
    }    
}

void REVOMINIStorage::write_byte(uint16_t loc, uint8_t value)
{
    // 'bytes' are packed 2 per word
    // Read existing data word and change upper or lower byte
    uint16_t data;
    error_parse(eeprom.read(loc >> 1, &data));

    if (loc & 1)
	data = (data & 0x00ff) | (value << 8); // Odd, upper byte
    else
	data = (data & 0xff00) | value;        // Even, lower byte
    error_parse(eeprom.write(loc >> 1, data));
}

void REVOMINIStorage::write_word(uint16_t loc, uint16_t value)
{
    if(loc & 1)
        write_block(loc, &value, sizeof(value));        // по нечетному адресу как обычно
    else
        error_parse(eeprom.write(loc >> 1, value));     // по четному сразу словом
}



void REVOMINIStorage::write_block(uint16_t loc, const void* src, size_t n)
{
    uint8_t *ptr_b = (uint8_t *)src;     // Treat as a block of bytes
    if(loc & 1){
        write_byte(loc++, *ptr_b++);      // odd byte
        n--;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
    uint16_t *ptr_w = (uint16_t *)ptr_b;     // Treat as a block of words
#pragma GCC diagnostic pop
    while(n>=2){
        error_parse(eeprom.write(loc >> 1, *ptr_w++));
        loc+=2;
        n-=2;
    }

    if(n){ // the last one
        ptr_b=(uint8_t *)ptr_w;
        write_byte(loc, *ptr_b);      // odd byte
    }
}

#endif
