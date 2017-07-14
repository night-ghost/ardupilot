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
    df_NumPages   = DF_NUM_PAGES - 1;

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

//	pread(flash_fd, buffer[BufferNum], DF_PAGE_SIZE, (PageAdr-1)*DF_PAGE_SIZE);

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
    //	pwrite(flash_fd, buffer[BufferNum], DF_PAGE_SIZE, (PageAdr-1)*(uint32_t)DF_PAGE_SIZE);
    
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
