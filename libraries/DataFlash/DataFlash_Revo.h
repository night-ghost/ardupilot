/* ************************************************************ */
/* DataFlash_Revo Log library                                 */
/* ************************************************************ */
#pragma once

#if CONFIG_HAL_BOARD == HAL_BOARD_REVOMINI

#include <AP_HAL/AP_HAL.h>
#include "DataFlash_Backend.h"
#include <AP_HAL_REVOMINI/AP_HAL_REVOMINI.h>
#include <AP_HAL_REVOMINI/GPIO.h>


// flash size
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


class DataFlash_Revo : public DataFlash_Backend
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

//[ from died Dataflash_Block

    struct PageHeader {
        uint16_t FileNumber;
        uint16_t FilePage;
    };

    // DataFlash Log variables...
    uint8_t df_BufferNum;
    uint8_t df_Read_BufferNum;
    uint16_t df_BufferIdx;
    uint16_t df_Read_BufferIdx;
    uint16_t df_PageAdr;
    uint16_t df_Read_PageAdr;
    uint16_t df_FileNumber;
    uint16_t df_FilePage;

    // offset from adding FMT messages to log data
    bool adding_fmt_headers;

    // erase handling
    bool NeedErase(void);

    // internal high level functions
    void StartRead(uint16_t PageAdr);
    uint16_t find_last_page(void);
    uint16_t find_last_page_of_log(uint16_t log_number);
    bool check_wrapped(void);
    uint16_t GetPage(void);
    uint16_t GetWritePage(void);
    void StartWrite(uint16_t PageAdr);
    void FinishWrite(void);
    
    // Read methods
    bool ReadBlock(void *pBuffer, uint16_t size);

    // file numbers
    void SetFileNumber(uint16_t FileNumber);
    uint16_t GetFilePage();
    uint16_t GetFileNumber();

    void _print_log_formats(AP_HAL::BetterStream *port);

protected:
    uint8_t df_manufacturer;
    uint16_t df_device;

    // page handling
    uint16_t df_PageSize;
    uint16_t df_NumPages;

    bool WritesOK() const override;

//]


public:
    DataFlash_Revo(DataFlash_Class &front, DFMessageWriter_DFLogStart *writer) :
        DataFlash_Backend(front, writer) { }
        
    void        Init() override;
    void        ReadManufacturerID();
    bool        CardInserted(void) const { return true; }
    
    uint8_t     ReadStatus();

    bool logging_enabled() const { return true; }
    bool logging_failed() const  { return false; };
    
    void stop_logging(void) { log_write_started = false; }
    
//[ from died Dataflash_Block

    // erase handling
    void EraseAll();

    bool NeedPrep(void);
    void Prep();

    /* Write a block of data at current offset */
    bool WritePrioritisedBlock(const void *pBuffer, uint16_t size, bool is_critical);

    // high level interface
    uint16_t find_last_log() override { return 0; };
    void get_log_boundaries(uint16_t log_num, uint16_t & start_page, uint16_t & end_page) {};
    void get_log_info(uint16_t log_num, uint32_t &size, uint32_t &time_utc);
    int16_t get_log_data_raw(uint16_t log_num, uint16_t page, uint32_t offset, uint16_t len, uint8_t *data);
    int16_t get_log_data(uint16_t log_num, uint16_t page, uint32_t offset, uint16_t len, uint8_t *data);
    uint16_t get_num_logs() override;
    uint16_t start_new_log(void);
    void LogReadProcess(const uint16_t list_entry,
                        uint16_t start_page, uint16_t end_page,
                        print_mode_fn print_mode,
                        AP_HAL::BetterStream *port);
    void DumpPageInfo(AP_HAL::BetterStream *port);
    void ShowDeviceInfo(AP_HAL::BetterStream *port);
    void ListAvailableLogs(AP_HAL::BetterStream *port);

    uint32_t bufferspace_available();
};

#endif // CONFIG_HAL_BOARD == HAL_BOARD_Revo
