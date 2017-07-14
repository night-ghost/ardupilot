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

uint8_t SdFatFs::init(void) {
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

    if(f_mkfs((TCHAR const*)_SDPath, 1 /* unpartitioned */, 8) == FR_OK &&
       f_mount(&_SDFatFs, (TCHAR const*)_SDPath, 1) == FR_OK) {
    	/* FatFs Initialization done */
        return 1;

    }

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

