#ifndef __EEPROM_H
#define __EEPROM_H

#include "wirish.h"
#include "stm32f4xx_flash.h"




/*
    realization from ST uses full state matrix which is 4*4 on 2-block case and highly grows with adding new blocks
    
    so we should takeinto account only real available cases of block states.
    
реализация от ST использует полную матрицу возможностей, имеющую размер 4*4 для 2ч блоков, и очень быстро растушую с ростом их количества
поэтому будем рассматривать лишь реально возможные варианты состояния.

Все повреждения - это отключения питания невовремя, мы обрабатываем только 1 блок одновременно, поэтому поврежденным может оказаться только 1.

возможные случаи:
1. отключение питания при записи
2. отключение питания при переносе страницы
3. отключение питания при стирании страницы
4. отключение питания при записи метки страницы.


ошибки при операции 1 приводят к недозаписи адреса, и тогда останутся установлены старшие биты, означающие повреждение ячейки
ошибки при операции 2 это недоперенесенная страница, поэтому если есть годная и переносимая с тем же номером то повторяем перенос
ошибки при операции 3 это недостертая страница и страница переноса без годной. Стираем нестертую и метим перенос как годную
ошибки при операции 4 это недоустановленная метка. Повторяет предыдущий случай.

по старту сканируем флеш и составляем битовые маски для годных страниц и страниц переноса, затем разбираемся со страницами переноса. 
Второй перенос не должен начинаться до завершения первого поэтому страница переноса может быть только одна

также составляем массив индексов страниц, дабы скан постоянно не делать.

чтение: просмотр страниц по обратному порядку индексов (с конца)
запись: просмотр страниц по порядку в поиске значения, которое может быть перезаписано, при невозможности перенос страницы.

перенос: с первой страницы, с контролем наличия в остальных и  подсчетом перенесенных элементов. Если их количество < макс количества 
       то записываем в эту страницу, трем вторую и готово, 
       если нет то трем исходную (1) метим страницу как годную, (2) переносим следующую.
       
       при переносе увеличиваем номер последовательности.  в нормальном случае все страницы имеют один и тот же номер.
       наличие годных страниц с меньшим номером говорит о незавершенности переноса, надо доделывать
       
       отключение питания в точке (1) - повторный перенос начиная с этого номера и вверх
       отключение питания в точке (2) - определение точки останова переноса по номерам последовательности страниц

*/


#define MAX_NUM_PAGES 15


//#define EEPROM_PAGE_SIZE (uint16_t)0x4000 /* Page size = 16kbyte*/
//#define EEPROM_START_ADDRESS 	((uint32_t)(0x8008000))

/* Pages 0 and 1 base and end addresses */
//#define EEPROM_PAGE0_BASE		((uint32_t)(EEPROM_START_ADDRESS))
//#define EEPROM_PAGE1_BASE		((uint32_t)(EEPROM_START_ADDRESS + EEPROM_PAGE_SIZE))

/* Page status definitions */
#define PAGE_STATE_MASK			((uint16_t)0xFF00) // статус страницы
#define PAGE_NUMBER_MASK		((uint16_t)0x000F) // порядковый номер страницы
#define PAGE_SEQ_MASK	        	((uint16_t)0x00F0) // номер последовательности страницы
#define EEPROM_ERASED			((uint16_t)0xFF00)	// статус PAGE is empty 
#define EEPROM_RECEIVE_DATA		((uint16_t)0xEE00)	// статус PAGE is marked to receive data 
#define EEPROM_VALID_PAGE		((uint16_t)0xAA00)	// статус PAGE containing valid data 
#define EEPROM_INVALID_PAGE		((uint16_t)0x0000)	// статус PAGE to erase

#define STATE_RECEIVE_DATA		((uint8_t)0xEE)	// статус PAGE is marked to receive data 
#define STATE_VALID_PAGE		((uint8_t)0xAA)	// статус PAGE containing valid data 
#define STATE_INVALID_PAGE		((uint8_t)0x00)	// статус PAGE to erase

#define ADDRESS_MASK 0x3fff          // valid address always below it - 16K of EEPROM max
#define FLAGS_MASK   (~ADDRESS_MASK) // if this bits are set then we have partially written slot

/* Page full define */
enum {
	EEPROM_OK            = 0x00,
	EEPROM_OUT_SIZE      = 0x81,
	EEPROM_BAD_ADDRESS   = 0x82,
	EEPROM_BAD_FLASH     = 0x83,
	EEPROM_NOT_INIT      = 0x84,
	EEPROM_WRITE_FAILED  = 0x96,
	EEPROM_NO_VALID_PAGE = 0xAB
};

#define EEPROM_DEFAULT_DATA		0xFFFF

#define FLASH_CR_ERRIE ((uint32_t)0x02000000) // not in stm32f4xx.h somehow

class EEPROMClass
{
public:
	EEPROMClass(void);

	uint16_t init(uint32_t, uint32_t *); // size and array of adresses, ends by 0

        /**
          * @brief  Erases PAGE0 and PAGE1 and writes EEPROM_VALID_PAGE / 0 header to PAGE0
          * @param  PAGE0 and PAGE1 base addresses
          * @retval _status of the last operation (Flash write or erase) done during EEPROM formating
          */
	uint16_t format(void);

        /**
          * @brief  Returns the erase counter for current page
          * @param  Data: Global variable contains the read variable value
          * @retval Success or error status:
          *                     - EEPROM_OK: if erases counter return.
          *                     - EEPROM_NO_VALID_PAGE: if no valid page was found.
          */
	uint16_t erases(uint16_t *);
        /**
          * @brief      Returns the last stored variable data, if found,
          *                     which correspond to the passed virtual address
          * @param  Address: Variable virtual address
          * @param  Data: Pointer to data variable
          * @retval Success or error status:
          *           - EEPROM_OK: if variable was found
          *           - EEPROM_BAD_ADDRESS: if the variable was not found
          *           - EEPROM_NO_VALID_PAGE: if no valid page was found.
          */
	uint16_t read (uint16_t address, uint16_t *data);
	/**
          * @brief      Returns the last stored variable data, if found,
          *                     which correspond to the passed virtual address
          * @param  Address: Variable virtual address
          * @retval Data for variable or EEPROM_DEFAULT_DATA, if any errors
         */
        inline uint16_t read (uint16_t address) { 
            uint16_t data;
            read(address, &data);
            return data;
        }
        /**
          * @brief  Writes/upadtes variable data in EEPROM.
          * @param  VirtAddress: Variable virtual address
          * @param  Data: 16 bit data to be written
          * @retval Success or error status:
          *                     - FLASH_COMPLETE: on success
          *                     - EEPROM_BAD_ADDRESS: if address = 0xFFFF
          *                     - EEPROM_PAGE_FULL: if valid page is full
          *                     - EEPROM_NO_VALID_PAGE: if no valid page was found
          *                     - EEPROM_OUT_SIZE: if no empty EEPROM variables
          *                     - Flash error code: on write Flash error
          */
	uint16_t write(uint16_t address, uint16_t data);
	/**
         * @brief  Return number of variable
         * @retval Number of variables
        */
	uint16_t count(uint16_t *data);
	inline uint16_t maxpagecount(void) {   return (PageSize / 4)-1; }
	inline uint16_t maxcount(void) {   return (PageSize / 4)-1 * num_pages; }

private:
        uint8_t num_pages; // number of usable pages
	uint32_t PageBase[MAX_NUM_PAGES]; 
	uint32_t PageSize;
	
        typedef struct PAGE {
            uint8_t id; // physical placement - index in PageBase
            uint8_t age;
            uint8_t state; // state of page - valid, target, empty
            uint16_t erased;
        } Page;

        Page page_list[MAX_NUM_PAGES]; 

	uint16_t _status;

	uint16_t _init(void);
	uint16_t _format(void);

	FLASH_Status _ErasePage(uint32_t);
        FLASH_Status _ErasePageByAddress(uint32_t Page_Address);

	uint16_t _CheckPage(uint32_t, uint16_t);
	uint16_t _CheckErasePage(uint32_t, uint16_t);
	uint16_t _Format(void);
	uint32_t _FindValidPage(void);
	uint16_t _GetVariablesCount(uint32_t, uint16_t);
	uint16_t _PageTransfer(uint32_t, uint32_t, uint16_t);
	uint16_t _VerifyPageFullWriteVariable(uint16_t, uint16_t);
};


extern EEPROMClass EEPROM;

#endif	/* __EEPROM_H */
