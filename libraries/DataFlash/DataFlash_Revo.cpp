/*
  hacked up DataFlash library for Desktop support
*/


#include <AP_HAL/AP_HAL.h>


#if CONFIG_HAL_BOARD == HAL_BOARD_REVOMINI && defined(BOARD_DATAFLASH_CS_PIN) && !defined(BOARD_DATAFLASH_FATFS)

#include "DataFlash_Revo.h"


#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <assert.h>

#include <AP_HAL_REVOMINI/Scheduler.h>
#include <AP_HAL_REVOMINI/GPIO.h>

#pragma GCC diagnostic ignored "-Wunused-result"


extern const AP_HAL::HAL& hal;

static uint8_t buffer[2][DF_PAGE_SIZE];


AP_HAL::OwnPtr<AP_HAL::SPIDevice> DataFlash_Revo::_spi = nullptr;
AP_HAL::Semaphore                *DataFlash_Revo::_spi_sem = nullptr;
bool                              DataFlash_Revo::log_write_started=false;
bool                              DataFlash_Revo::flash_died=false;


// the last page holds the log format in first 4 bytes. Please change
// this if (and only if!) the low level format changes
#define DF_LOGGING_FORMAT    0x28122013L

uint32_t DataFlash_Revo::bufferspace_available()
{
    // because DataFlash_Block devices are ring buffers, we *always*
    // have room...
    return df_NumPages * df_PageSize;
}

// *** DATAFLASH PUBLIC FUNCTIONS ***
void DataFlash_Revo::StartWrite(uint16_t PageAdr)
{
    df_BufferIdx  = 0;
    df_BufferNum  = 0;
    df_PageAdr    = PageAdr;
    WaitReady();
}

void DataFlash_Revo::FinishWrite(void)
{
    // Write Buffer to flash, NO WAIT
    BufferToPage(df_BufferNum, df_PageAdr, 0);      
    df_PageAdr++;
    // If we reach the end of the memory, start from the beginning    
    if (df_PageAdr > df_NumPages)
        df_PageAdr = 1;

    // switch buffer
    df_BufferNum ^= 1;
    df_BufferIdx = 0;
}

bool DataFlash_Revo::WritesOK() const
{
    if (!DataFlash_Backend::WritesOK()) {
        return false;
    }
    if (!CardInserted()) {
        return false;
    }
    if (!log_write_started) {
        return false;
    }
    return true;
}

bool DataFlash_Revo::WritePrioritisedBlock(const void *pBuffer, uint16_t size,
    bool is_critical)
{
    // is_critical is ignored - we're a ring buffer and never run out
    // of space.  possibly if we do more complicated bandwidth
    // limiting we can reservice bandwidth based on is_critical
    if (!WritesOK()) {
        return false;
    }

    if (! WriteBlockCheckStartupMessages()) {
        return false;
    }

    while (size > 0) {
        uint16_t n = df_PageSize - df_BufferIdx;
        if (n > size) {
            n = size;
        }

        if (df_BufferIdx == 0) {
            // if we are at the start of a page we need to insert a
            // page header
            if (n > df_PageSize - sizeof(struct PageHeader)) {
                n = df_PageSize - sizeof(struct PageHeader);
            }
            struct PageHeader ph = { df_FileNumber, df_FilePage };
            BlockWrite(df_BufferNum, df_BufferIdx, &ph, sizeof(ph), pBuffer, n);
            df_BufferIdx += n + sizeof(ph);
        } else {
            BlockWrite(df_BufferNum, df_BufferIdx, nullptr, 0, pBuffer, n);
            df_BufferIdx += n;
        }

        size -= n;
        pBuffer = (const void *)(n + (uintptr_t)pBuffer);

        if (df_BufferIdx == df_PageSize) {
            FinishWrite();
            df_FilePage++;
        }
    }

    return true;
}


// Get the last page written to
uint16_t DataFlash_Revo::GetWritePage()
{
    return df_PageAdr;
}

// Get the last page read
uint16_t DataFlash_Revo::GetPage()
{
    return df_Read_PageAdr;
}

void DataFlash_Revo::StartRead(uint16_t PageAdr)
{
    df_Read_BufferNum = 0;
    df_Read_PageAdr   = PageAdr;

    // disable writing while reading
    log_write_started = false;

    WaitReady();

    // copy flash page to buffer
    PageToBuffer(df_Read_BufferNum, df_Read_PageAdr);

    // We are starting a new page - read FileNumber and FilePage
    struct PageHeader ph;
    BlockRead(df_Read_BufferNum, 0, &ph, sizeof(ph));
    df_FileNumber = ph.FileNumber;
    df_FilePage   = ph.FilePage;
    df_Read_BufferIdx = sizeof(ph);
}

bool DataFlash_Revo::ReadBlock(void *pBuffer, uint16_t size)
{
    while (size > 0) {
        uint16_t n = df_PageSize - df_Read_BufferIdx;
        if (n > size) {
            n = size;
        }

        WaitReady();

        if (!BlockRead(df_Read_BufferNum, df_Read_BufferIdx, pBuffer, n)) {
            return false;
        }
        size -= n;
        pBuffer = (void *)(n + (uintptr_t)pBuffer);
        
        df_Read_BufferIdx += n;

        if (df_Read_BufferIdx == df_PageSize) {
            df_Read_PageAdr++;
            if (df_Read_PageAdr > df_NumPages) {
                df_Read_PageAdr = 1;
            }
            PageToBuffer(df_Read_BufferNum, df_Read_PageAdr);

            // We are starting a new page - read FileNumber and FilePage
            struct PageHeader ph;
            if (!BlockRead(df_Read_BufferNum, 0, &ph, sizeof(ph))) {
                return false;
            }
            df_FileNumber = ph.FileNumber;
            df_FilePage   = ph.FilePage;

            df_Read_BufferIdx = sizeof(ph);
        }
    }
    return true;
}

void DataFlash_Revo::SetFileNumber(uint16_t FileNumber)
{
    df_FileNumber = FileNumber;
    df_FilePage = 1;
}

uint16_t DataFlash_Revo::GetFileNumber()
{
    return df_FileNumber;
}

uint16_t DataFlash_Revo::GetFilePage()
{
    return df_FilePage;
}

void DataFlash_Revo::EraseAll()
{
    log_write_started = false;

    ChipErase();

    // write the logging format in the last page
    hal.scheduler->delay(100);
    StartWrite(df_NumPages+1);
    uint32_t version = DF_LOGGING_FORMAT;
    log_write_started = true;
    BlockWrite(df_BufferNum, 0, nullptr, 0, &version, sizeof(version));

    log_write_started = false;
    FinishWrite();
    hal.scheduler->delay(100);

//[ just to test
    StartRead(df_NumPages+1); // read last page after erase to check it

    StartRead(1);
//]
}

bool DataFlash_Revo::NeedPrep(void)
{
    return NeedErase();
}

void DataFlash_Revo::Prep()
{
    if (hal.util->get_soft_armed()) {
        // do not want to do any filesystem operations while we are e.g. flying
        return;
    }
    if (NeedErase()) {
        EraseAll();
    }
}

/*
 *  we need to erase if the logging format has changed
 */
bool DataFlash_Revo::NeedErase(void)
{
    uint32_t version = 0;
    StartRead(df_NumPages+1); // last page

    BlockRead(df_Read_BufferNum, 0, &version, sizeof(version));
    StartRead(1);
    if(version == DF_LOGGING_FORMAT) return false;
    
    hal.console->printf("Need to erase: version is %lx required %lx\n", version, DF_LOGGING_FORMAT);
        
    return true;
}

/**
  get raw data from a log
 */
int16_t DataFlash_Revo::get_log_data_raw(uint16_t log_num, uint16_t page, uint32_t offset, uint16_t len, uint8_t *data)
{
    uint16_t data_page_size = df_PageSize - sizeof(struct PageHeader);

    if (offset >= data_page_size) {
        page += offset / data_page_size;
        offset = offset % data_page_size;
        if (page > df_NumPages) {
            // pages are one based, not zero
            page = 1 + page - df_NumPages;
        }
    }
    if (log_write_started || df_Read_PageAdr != page) {
        StartRead(page);
    }

    df_Read_BufferIdx = offset + sizeof(struct PageHeader);
    if (!ReadBlock(data, len)) {
        return -1;
    }

    return (int16_t)len;
}

/**
  get data from a log, accounting for adding FMT headers
 */
int16_t DataFlash_Revo::get_log_data(uint16_t log_num, uint16_t page, uint32_t offset, uint16_t len, uint8_t *data)
{
    if (offset == 0) {
        uint8_t header[3];
        get_log_data_raw(log_num, page, 0, 3, header);
        adding_fmt_headers = (header[0] != HEAD_BYTE1 || header[1] != HEAD_BYTE2 || header[2] != LOG_FORMAT_MSG);
    }
    uint16_t ret = 0;

    if (adding_fmt_headers) {
        // the log doesn't start with a FMT message, we need to add
        // them
        const uint16_t fmt_header_size = num_types() * sizeof(struct log_Format);
        while (offset < fmt_header_size && len > 0) {
            struct log_Format pkt;
            uint8_t t = offset / sizeof(pkt);
            uint8_t ofs = offset % sizeof(pkt);
            Log_Fill_Format(structure(t), pkt);
            uint8_t n = sizeof(pkt) - ofs;
            if (n > len) {
                n = len;
            }
            memcpy(data, ofs + (uint8_t *)&pkt, n);
            data += n;
            offset += n;
            len -= n;
            ret += n;
        }
        offset -= fmt_header_size;
    }

    if (len > 0) {
        ret += get_log_data_raw(log_num, page, offset, len, data);
    }

    return ret;
}


// Public Methods //////////////////////////////////////////////////////////////
void DataFlash_Revo::Init()
{

    df_NumPages=0;
    
    REVOMINIGPIO::_pinMode(DF_RESET,OUTPUT);
    // Reset the chip
    REVOMINIGPIO::_write(DF_RESET,0);
    REVOMINIScheduler::_delay(1);
    REVOMINIGPIO::_write(DF_RESET,1);

    _spi = hal.spi->get_device(HAL_DATAFLASH_NAME);

    if (!_spi) {
        AP_HAL::panic("PANIC: DataFlash SPIDeviceDriver not found");
        return; /* never reached */
    }

    _spi_sem = _spi->get_semaphore();
    if (!_spi_sem) {
        AP_HAL::panic("PANIC: DataFlash SPIDeviceDriver semaphore is null");
        return; /* never reached */
    }

    

    _spi_sem->take(10);
    _spi->set_speed(AP_HAL::Device::SPEED_LOW);

    DataFlash_Backend::Init();
    
    _spi_sem->give();


    ReadManufacturerID();

    flash_died=false;

    log_write_started = true;

    df_PageSize = DF_PAGE_SIZE;

    // reserve last page for config information
    df_NumPages   = BOARD_DATAFLASH_PAGES - 1;

}

void DataFlash_Revo::WaitReady() { 
    if(flash_died) return;
    
    uint32_t t = AP_HAL::millis();
    while(ReadStatus()!=0){
    
        REVOMINIScheduler::yield(20); // пока ждем пусть другие работают
        
        if(AP_HAL::millis() - t > 4000) {
            flash_died = true;
            return;
        }
    }
}

//  try to take a semaphore safely from both in a timer and outside
bool DataFlash_Revo::_sem_take(uint8_t timeout)
{

    if(!_spi_sem || flash_died) return false;

    if (REVOMINIScheduler::_in_timerprocess()) {
        return _spi_sem->take_nonblocking();
    }
    return _spi_sem->take(timeout);
}

bool DataFlash_Revo::cs_assert(){
    if (!_sem_take(50))
        return false;

    _spi->set_speed(AP_HAL::Device::SPEED_HIGH);

    REVOMINIGPIO::_write(DF_RESET,0);
    return true;
}

void DataFlash_Revo::cs_release(){
    REVOMINIGPIO::_write(DF_RESET,1);

    _spi_sem->give();
}


// This function is mainly to test the device
void DataFlash_Revo::ReadManufacturerID()
{
    // activate dataflash command decoder
    if (!cs_assert()) return;

    // Read manufacturer and ID command...
    spi_write(JEDEC_DEVICE_ID); //

    df_manufacturer = spi_read();
    df_device = spi_read(); //memorytype
    df_device = (df_device << 8) | spi_read(); //capacity
    spi_read();

    // release SPI bus for use by other sensors
    cs_release();
}


// Read the status register
uint8_t DataFlash_Revo::ReadStatusReg()
{
    uint8_t tmp;

    // activate dataflash command decoder
    if (!cs_assert()) return JEDEC_STATUS_BUSY;

    // Read status command
    spi_write(JEDEC_READ_STATUS);
    tmp = spi_read(); // We only want to extract the READY/BUSY bit

    // release SPI bus for use by other sensors
    cs_release();
    
    return tmp;
}

uint8_t DataFlash_Revo::ReadStatus()
{
  // We only want to extract the READY/BUSY bit
    int32_t status = ReadStatusReg();
    if (status < 0)
            return -1;
    return status & JEDEC_STATUS_BUSY;
}


void DataFlash_Revo::PageToBuffer(unsigned char BufferNum, uint16_t pageNum)
{

    uint32_t PageAdr = pageNum * DF_PAGE_SIZE;

    if (!cs_assert()) return;

    uint8_t cmd[4];
    cmd[0] = JEDEC_READ_DATA;
    cmd[1] = (PageAdr >> 16) & 0xff;
    cmd[2] = (PageAdr >>  8) & 0xff;
    cmd[3] = (PageAdr >>  0) & 0xff;

    _spi->transfer(cmd, sizeof(cmd), NULL, 0);

    _spi->transfer(NULL,0, buffer[BufferNum], DF_PAGE_SIZE);
    
    cs_release();
}

void DataFlash_Revo::BufferToPage (unsigned char BufferNum, uint16_t pageNum, unsigned char wait)
{    
    uint32_t PageAdr = pageNum * DF_PAGE_SIZE;

    Flash_Jedec_WriteEnable();
    
    if (!cs_assert()) return;

    uint8_t cmd[4];
    cmd[0] = JEDEC_PAGE_WRITE;
    cmd[1] = (PageAdr >> 16) & 0xff;
    cmd[2] = (PageAdr >>  8) & 0xff;
    cmd[3] = (PageAdr >>  0) & 0xff;

    _spi->transfer(cmd, sizeof(cmd),NULL, 0);

    _spi->transfer(buffer[BufferNum], DF_PAGE_SIZE, NULL, 0);

    cs_release();

    if(wait)   WaitReady();

}

void DataFlash_Revo::BufferWrite (unsigned char BufferNum, uint16_t IntPageAdr, unsigned char Data)
{
	buffer[BufferNum][IntPageAdr] = (uint8_t)Data;
}

void DataFlash_Revo::BlockWrite(uint8_t BufferNum, uint16_t IntPageAdr, 
                                const void *pHeader, uint8_t hdr_size,
                                const void *pBuffer, uint16_t size)
{
    if (hdr_size) {
        memcpy(&buffer[BufferNum][IntPageAdr],
               pHeader,
               hdr_size);
    }
    memcpy(&buffer[BufferNum][IntPageAdr+hdr_size],
           pBuffer,
           size);
}

// read size bytes of data to a page. The caller must ensure that
// the data fits within the page, otherwise it will wrap to the
// start of the page
bool DataFlash_Revo::BlockRead(uint8_t BufferNum, uint16_t IntPageAdr, void *pBuffer, uint16_t size)
{
    memcpy(pBuffer, &buffer[BufferNum][IntPageAdr], size);
    return true;
}

/*

* 2 097 152 bytes (8 bits each) 
* 32 sectors (512 Kbits, 65536 bytes each)
* 8192 pages (256 bytes each).

*/

void DataFlash_Revo::PageErase (uint16_t pageNum)
{

    uint32_t PageAdr = pageNum * DF_PAGE_SIZE;

    uint8_t cmd[4];
    cmd[0] = JEDEC_SECTOR_ERASE;
    cmd[1] = (PageAdr >> 16) & 0xff;
    cmd[2] = (PageAdr >>  8) & 0xff;
    cmd[3] = (PageAdr >>  0) & 0xff;

    Flash_Jedec_WriteEnable();
    
    if (!cs_assert()) return;

    _spi->transfer(cmd, sizeof(cmd), NULL, 0);
        
    cs_release();
}

void DataFlash_Revo::BlockErase (uint16_t BlockAdr)
{

    for(uint8_t i=0; i<8; i++) {
        PageErase( BlockAdr * 8 + i );
    }

}


void DataFlash_Revo::ChipErase()
{

    uint8_t cmd[1];
    cmd[0] = JEDEC_BULK_ERASE;

    Flash_Jedec_WriteEnable();
    
    if (!cs_assert()) return;

    _spi->transfer(cmd, sizeof(cmd), NULL, 0);
        
    cs_release();
}


void DataFlash_Revo::Flash_Jedec_WriteEnable(void)
{
    // activate dataflash command decoder
    if (!cs_assert()) return;

    spi_write(JEDEC_WRITE_ENABLE);

    cs_release();
}


#endif
