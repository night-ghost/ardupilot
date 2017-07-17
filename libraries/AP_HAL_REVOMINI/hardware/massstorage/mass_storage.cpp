#include <AP_HAL_REVOMINI/AP_HAL_REVOMINI_Namespace.h>

#include "mass_storage.h"
#include "../sd/SD.h"
#include "../sd/Sd2Card.h"
#include <usb.h>

#include "usbMassStorage.h"

#ifdef USB_MASSSTORAGE

uint32_t MAL_massBlockCount[STORAGE_LUN_NBR];
uint32_t MAL_massBlockSize[STORAGE_LUN_NBR];

static uint32_t rebootFileBlock=0;



extern USB_OTG_CORE_HANDLE           USB_OTG_dev;

void MassStorage::setup() const {
    usb_attr_t usb_attr;
    
    usb_open(); 

    usb_default_attr(&usb_attr);
    usb_attr.preempt_prio = 13; // very low to allow to run timers
    usb_attr.sub_prio = 13;
    usb_attr.use_present_pin = 1;
    usb_attr.present_port = PIN_MAP[BOARD_USB_SENSE].gpio_device;
    usb_attr.present_pin =  PIN_MAP[BOARD_USB_SENSE].gpio_bit;

    usb_setParams(&usb_attr);



    SdFatFs *fs = SD.getVolume();
    uint32_t bpc = fs->blocksPerCluster();

#ifdef BOARD_DATAFLASH_ERASE_SIZE // FAT in dataflash

    MAL_massBlockSize[0] = bpc * 512; // full cluster to exclude RMW operations let host buffers
    MAL_massBlockCount[0] = fs->clusterCount();   // in clusters

#else // FAT on SD card

    MAL_massBlockSize[0] = 512; 
    MAL_massBlockCount[0] = bpc * fs->clusterCount();   // in blocks
    
    static const char nproc[]="0:/proc";
    // open the /PROC directory    
    File proc = SD.open(nproc);
    bool f_ok = false;
            
    if (proc) {
        proc.close();
        f_ok=true;
    } else {
        if(SD.mkdir(nproc)) 
            f_ok=true;                
    }
    if(f_ok){
        File rebootFile = SD.open("0:/proc/reboot", O_WRITE | O_CREAT | O_TRUNC);
        // create /PROC/REBOOT file and write a '0' to it.
        if (rebootFile) {
            rebootFile.write('0');
            rebootFileBlock = rebootFile.firstCluster() * bpc;
            rebootFile.close();
        }
    }
#endif        
            
    SD.sync(); // consistent state

    USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_MSC_desc, &USBD_MSC_cb, &USR_cb);
}


// The following methods are used by the USBMassStorage driver to read and write to the SDCard.  
 
extern "C" {

uint16_t usb_mass_mal_get_status(uint8_t lun){
    return Sd2Card::errorCode();
}


int8_t usb_mass_mal_write_memory(uint8_t lun, uint32_t lba, uint8_t *writebuff, uint16_t transferLength) {

  if (lba == rebootFileBlock) {
    REVOMINI::REVOMINIScheduler::_reboot(false);
    return USB_MASS_MAL_SUCCESS;
  }
  
  if (lun != 0) {
    return USB_MASS_MAL_FAIL;
  }
  if (Sd2Card::writeBlock(lba, writebuff, transferLength)==RES_OK) {
    return USB_MASS_MAL_SUCCESS;
  }
  return USB_MASS_MAL_FAIL;
}

int8_t usb_mass_mal_read_memory(uint8_t lun, uint32_t lba, uint8_t *readbuff, uint16_t transferLength) {
  if (lun != 0) {
    return USB_MASS_MAL_FAIL;
  }
  if (Sd2Card::readBlock(lba, readbuff, transferLength)==RES_OK) {
    return USB_MASS_MAL_SUCCESS;
  }
  return USB_MASS_MAL_FAIL;
}

extern void usb_mass_mal_USBdisconnect(); // in HAL

} // extern C

#endif
