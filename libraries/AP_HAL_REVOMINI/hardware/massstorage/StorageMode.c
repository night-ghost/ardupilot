#include "StorageMode.h"


extern USB_OTG_CORE_HANDLE           USB_OTG_dev;


const int8_t STORAGE_Inquirydata[] = 
{
  0x00, 0x80, 0x02, 0x02,
  (USBD_STD_INQUIRY_LENGTH - 5),
  0x00, 0x00, 0x00,
  'R', 'E', 'V', 'O', ' ', '@', 'N', 'G', // Manufacturer : 8 bytes
  'S', 'D', ' ', 'C', 'a', 'r', 'd', ' ', // Product      : 16 Bytes
  'R', 'e', 'a', 'd', 'e', 'r', ' ', ' ', //
  '1', '.', '0' ,'0',                     // Version      : 4 Bytes
};

int8_t STORAGE_Init(uint8_t lun)
{
    return 0;
}


int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
    if(lun>0) return 1;
    
    *block_size = MAL_massBlockSize[lun];
    *block_num = MAL_massBlockCount[lun];
    return 0;
}

int8_t STORAGE_IsReady(uint8_t lun)
{
    return usb_mass_mal_get_status(lun); 
}

int8_t STORAGE_IsWriteProtected(uint8_t lun)
{
	return 1;
}

int8_t STORAGE_Read(
	uint8_t lun,        // logical unit number
	uint8_t *buf,       // Pointer to the buffer to save data
	uint32_t blk_addr,  // address of 1st block to be read
	uint16_t blk_len)   // nmber of blocks to be read
{
	//emfat_read(&emfat, buf, blk_addr, blk_len);
        return usb_mass_mal_read_memory(lun, blk_addr, buf, blk_len);
}

int8_t STORAGE_Write(uint8_t lun,
	uint8_t *buf,
	uint32_t blk_addr,
	uint16_t blk_len)
{
//	emfat_write(&emfat, buf, blk_addr, blk_len);
	return usb_mass_mal_write_memory(lun, blk_addr, buf, blk_len);
}

int8_t STORAGE_GetMaxLun(void)
{
  return (STORAGE_LUN_NBR - 1);
}

const USBD_STORAGE_cb_TypeDef STORAGE_fops =
{
  STORAGE_Init,
  STORAGE_GetCapacity,
  STORAGE_IsReady,
  STORAGE_IsWriteProtected,
  STORAGE_Read,
  STORAGE_Write,
  STORAGE_GetMaxLun,
  (int8_t *)STORAGE_Inquirydata,
};

const USBD_STORAGE_cb_TypeDef * const USBD_STORAGE_fops = &STORAGE_fops;

#ifdef USE_USB_OTG_FS
void OTG_FS_WKUP_IRQHandler(void)
{
  if(USB_OTG_dev.cfg.low_power)
  {
    *(uint32_t *)(0xE000ED10) &= 0xFFFFFFF9 ;
    SystemInit();
    USB_OTG_UngateClock(&USB_OTG_dev);
  }
  EXTI_ClearITPendingBit(EXTI_Line18);
}
#endif
