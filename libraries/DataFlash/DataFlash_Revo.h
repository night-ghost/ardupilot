/* ************************************************************ */
/* DataFlash_Revo Log library                                 */
/* ************************************************************ */
#pragma once

#if CONFIG_HAL_BOARD == HAL_BOARD_REVOMINI

#include <AP_HAL/AP_HAL.h>
#include "DataFlash_Block.h"
#include <AP_HAL_REVOMINI/AP_HAL_REVOMINI.h>
#include <AP_HAL_REVOMINI/GPIO.h>
#include "DataFlash_Block.h"

// flash size
//#define DF_NUM_PAGES 0x1f00
#define DF_NUM_PAGES BOARD_DATAFLASH_PAGES 
#define DF_PAGE_SIZE 256L

#define DF_RESET BOARD_DATAFLASH_CS_PIN // RESET (PB3)

//Micron M25P16 Serial Flash Embedded Memory 16 Mb, 3V
#define JEDEC_WRITE_ENABLE           0x06
#define JEDEC_WRITE_DISABLE          0x04
#define JEDEC_READ_STATUS            0x05
#define JEDEC_WRITE_STATUS           0x01
#define JEDEC_READ_DATA              0x03
#define JEDEC_FAST_READ              0x0b
#define JEDEC_DEVICE_ID              0x9F
#define JEDEC_PAGE_WRITE             0x02

#define JEDEC_BULK_ERASE             0xC7
#define JEDEC_SECTOR_ERASE           0xD8

#define JEDEC_STATUS_BUSY            0x01
#define JEDEC_STATUS_WRITEPROTECT    0x02
#define JEDEC_STATUS_BP0             0x04
#define JEDEC_STATUS_BP1             0x08
#define JEDEC_STATUS_BP2             0x10
#define JEDEC_STATUS_TP              0x20
#define JEDEC_STATUS_SEC             0x40
#define JEDEC_STATUS_SRP0            0x80

#define expect_memorytype            0x20
#define expect_capacity              0x15


using namespace REVOMINI;


class DataFlash_Revo : public DataFlash_Block
{
private:
    //Methods
    void              BufferWrite (uint8_t BufferNum, uint16_t IntPageAdr, uint8_t Data);
    void              BufferToPage (uint8_t BufferNum, uint16_t PageAdr, uint8_t wait);
    void              PageToBuffer(uint8_t BufferNum, uint16_t PageAdr);
    void              WaitReady();
    uint8_t           ReadStatusReg();
    uint16_t          PageSize() { return df_PageSize; }
    void              PageErase (uint16_t PageAdr);
    void              BlockErase (uint16_t BlockAdr);
    void              ChipErase();

    void              Flash_Jedec_WriteEnable();
    void              Flash_Jedec_EraseSector(uint32_t chip_offset);

    // write size bytes of data to a page. The caller must ensure that
    // the data fits within the page, otherwise it will wrap to the
    // start of the page
    // If pHeader is not nullptr then write the header bytes before the data
    void		    BlockWrite(uint8_t BufferNum, uint16_t IntPageAdr, 
				       const void *pHeader, uint8_t hdr_size,
				       const void *pBuffer, uint16_t size);

    // read size bytes of data to a page. The caller must ensure that
    // the data fits within the page, otherwise it will wrap to the
    // start of the page
    bool 		    BlockRead(uint8_t BufferNum, uint16_t IntPageAdr, void *pBuffer, uint16_t size);


    //////////////////
    static AP_HAL::OwnPtr<AP_HAL::SPIDevice> _spi;
    static AP_HAL::Semaphore *_spi_sem;
    static bool log_write_started;

    static bool              _sem_take(uint8_t timeout); // take a semaphore safely

    bool cs_assert(); // Select device 
    void cs_release(); // Deselect device

    uint8_t spi_read(void) { uint8_t b;  _spi->transfer(NULL,0, &b, 1); return b; }
    void    spi_write(uint8_t b) {       _spi->transfer(&b,1, NULL, 0);  }
    
    void spi_write(int data) { spi_write((uint8_t)data); }

    static bool flash_died;

public:
    DataFlash_Revo(DataFlash_Class &front, DFMessageWriter_DFLogStart *writer) :
        DataFlash_Block(front, writer) { }
    void        Init() override;
    void        ReadManufacturerID();
    bool        CardInserted(void) const { return true; }
    
    uint8_t     ReadStatus();

    bool logging_enabled() const { return true; }
    bool logging_failed() const  { return false; };
    
    void stop_logging(void) { log_write_started = false; }
};

#endif // CONFIG_HAL_BOARD == HAL_BOARD_Revo
