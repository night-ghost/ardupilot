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



#include "SdFatFs.h"

#if defined(BOARD_SDCARD_CS_PIN) || defined(BOARD_DATAFLASH_FATFS)

uint8_t SdFatFs::init(Sd2Card *card) {

    _card=card;

    _SDPath[0] = '0';
    _SDPath[1] = ':';
    _SDPath[2] = '/';
    _SDPath[3] = 0;

    /*##-2- Register the file system object to the FatFs module ##############*/
    if(f_mount(&_SDFatFs, (TCHAR const*)_SDPath, 1) == FR_OK) {
	/* FatFs Initialization done */
	return 1;
    }
    
#if defined(BOARD_DATAFLASH_FATFS) // in DataFlash

//    printf("Formatting DataFlash to FAT..."); - no printf without HAL
    if(f_mkfs((TCHAR const*)_SDPath, 1 /* unpartitioned */, BOARD_DATAFLASH_ERASE_SIZE/FAT_SECTOR_SIZE /* cluster in sectors */) == FR_OK &&
       f_mount(&_SDFatFs, (TCHAR const*)_SDPath, 1) == FR_OK) {

//        printf(" OK!\n");
    	/* FatFs Initialization done */
        return 1;

    }

//    printf(" Error!\n");
#endif
    return 0;
}

uint8_t SdFatFs::fatType(void)
{
	switch (_SDFatFs.fs_type)
	{
	case FS_FAT12:
		return 12;
	case FS_FAT16:
		return 16;
	case FS_FAT32:
		return 32;
	default:
		return 0;
	}
}

#endif

