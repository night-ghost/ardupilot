/*
 * <Description>
 *
 * Copyright (C) 2016, STMicroelectronics - All Rights Reserved
 * Author: YOUR NAME <> for STMicroelectronics.
 *
 * License type: GPLv2
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef Sd2Card_h
#define Sd2Card_h

#include "FatFs/drivers/sd.h"
#include "FatFs/diskio.h"

#include <AP_HAL/AP_HAL.h>


#if defined(BOARD_SDCARD_CS_PIN) || defined(BOARD_DATAFLASH_FATFS)

#include <AP_HAL_REVOMINI/AP_HAL_REVOMINI.h>
#include <AP_HAL_REVOMINI/SPIDevice.h>
#include <AP_HAL_REVOMINI/Scheduler.h>
#include <AP_HAL_REVOMINI/handler.h>

#include "FatFs/diskio.h"

using namespace REVOMINI;

#define   FALSE      ((uint8_t)0x00)
#define   TRUE       ((uint8_t)0x01)

// card types to match Arduino definition
/** Standard capacity V1 SD card */
#define SD_CARD_TYPE_SD1	CT_SD1
/** Standard capacity V2 SD card */
#define SD_CARD_TYPE_SD2	CT_SD2
/** High Capacity SD card */
#define SD_CARD_TYPE_SDHC	CT_BLOCK




extern "C" uint8_t spi_spiSend(uint8_t b);
extern "C" uint8_t spi_spiRec(void);
extern "C" void spi_spiTransfer(const uint8_t *send, uint32_t send_len,  uint8_t *recv, uint32_t recv_len);
extern "C" void spi_chipSelectHigh(void);
extern "C" bool spi_chipSelectLow(bool take_sem);
extern "C" void spi_yield();
extern "C" uint8_t spi_detect();
extern "C" uint32_t get_fattime();


class Sd2Card {
public:

    uint8_t init(AP_HAL::OwnPtr<REVOMINI::SPIDevice> spi);

    /** Return the card type: SD V1, SD V2 or SDHC */
    static uint8_t type(void) { return sd_get_type(); }

    static uint16_t errorCode() { return sd_status(); }
    static uint8_t writeBlock(uint32_t block, uint8_t *buff) { return sd_write(buff, block, 1)==RES_OK; }
    static uint8_t readBlock(uint32_t block, uint8_t *buff){   return sd_read( buff, block, 1)==RES_OK; }

    static uint8_t writeBlock(uint32_t block, uint8_t *buff, uint16_t len) { return sd_write(buff, block, len)==RES_OK; }
    static uint8_t readBlock(uint32_t block, uint8_t *buff, uint16_t len){   return sd_read( buff, block, len)==RES_OK; }
    
    static uint8_t ioctl(uint32_t cmd, uint32_t *buff){   return sd_ioctl(cmd, buff) == RES_OK; }

    static uint32_t sectorCount() { // full number of sectors
        uint32_t sz;
        if(sd_ioctl(GET_SECTOR_COUNT, &sz) == RES_OK)
            return sz;
        return 0;

    }

    static uint32_t blockSize() { // sectors in erase block
        uint32_t sz;
        if(sd_ioctl(GET_BLOCK_SIZE, &sz) == RES_OK)
            return sz; 
#ifdef BOARD_DATAFLASH_ERASE_SIZE
        return BOARD_DATAFLASH_ERASE_SIZE/512;
#else
        return 8;
#endif

    }

    static uint32_t sectorSize() { // sector size in bytes
        uint32_t sz;
        if(sd_ioctl(GET_SECTOR_SIZE, &sz) == RES_OK)
            return sz;
            
        return 512;
    }

private:
    void _timer(void) { sd_timerproc(); }

};

#endif // revomini

#endif  // sd2Card_h
