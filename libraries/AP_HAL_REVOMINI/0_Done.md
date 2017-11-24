* near a half of code is fully rewritten
* added check for all input parameters - no more HardFaults if setted wrong pin number
* external I2C bus moved out from FlexiPort by Soft_I2C driver so we always has at least 3 UARTs
* added 1 full-functional UART (only for quads) and 1 RX-only UART for DSM satellite receiver on OpLink connector
* Unlike many other boards, fully implemented registerPeriodicCallback & Co calls
* implemented register_io_process via simple cooperative multitasking
* added buzzer support
* stack now in CCM memory
* PPM and PWM inputs works via Timer's driver handlers
* added DSM and SBUS parsing on PPM input
* high-frequency (8kHz) advanced scheduler, common for all needs, capable to use semaphores with (optional) performance statistics
* all hardware description tables are now 'const' and locates in flash
* more reliable reset for I2C bus on hangups
* all drivers support set_retries()
* all delays - even microseconds - are very presize by using hardware clock counter (DWT_CYCCNT) in free-running mode
* separated USB and UART drivers
* new SoftwareSerial driver based on ST appnote
* now it uses MPU6000 DRDY output
* removed all compiler's warnings
* ported and slightly altered bootloader to support flashing and start firmware automatically at addresses 8010000 and 8020000 
  (2 low 16k flash pages are used to emulate EEPROM)
* EEPROM emulation altered to ensure the reliability of data storage at power failures
* optimized EEPROM usage by changing from 1-byte to 2-byte writes
* all internal calls use static private methods
* removed unused files from "wirish" folder
* micros() call uses 32-bit hardware timer instead of systick_micros()
* added parameters support for HAL
* OneShot supported
* added translation layer between system PWM modes and borad PWM modes
* added descriptors for all internal hardware
* added support to reboot to DFU mode (via "reboot to PX4 bootloader" in MP)
* after any Fault or Panic() automatically reboots to DFU mode
* diversity on RC_Input
* reverted idiotic mainline change in Periodic interface
* all used drivers altered to use "reschedule me" HAL feature
* added usage for Compass' DataReady pin
* unified exception handling 
* added ability to bind Spectrum satellite without managed 3.3 DC-DC (requires short power off)
* added support for Arduino32 reset sequence - negative DTR edge on 1200 baud or '1eaf' packet with high DTR
* fixed hang on dataflash malfunction
* fixed USB characters loss *without* hangup if disconnected
* added failsafe on receiver's hangup - if no channel changes in 60 seconds
* added HAL parameters support
* changed to simplify support of slightly different boards - eg. AirbotF4
* full support for AirbotF4 (separate binaries)
* added support for servos on Input port unused pins
* Added handling of FLASH_SR error bits, including automatic clearing of write protection
* added Arduino-like support of relay on arbitrary pin
* simplified UART driver, buffering now used even in non-blocking mode
* EEPROM emulation locks flash after each write
* EEPROM error handling
* fixed Ardupilot's stealing of Servos even they marked as "Unused"
* added compilation date & time to log output
* added SBUS input via USART1 as on Airbot boards
* added per-board read_me.md files
* fixed Dataflash logs bug from mainstream - now logs are persists between reboots!
* DMA mode for lagre SPI transfers
* USB virtual com-port can be connected to any UART - eg. for OSD or 3DR modems setup
* any UART can be connected to ESC for 4-way interface
* support for logs on SD card for AirbotV2 board
* fixed 2nd Dataflash logs bug from mainstream - now logs are persists between reboots even on boards having chips with 64k sector
* I2C wait time limited to 0.3s - no more forever hangs by external compass errors
* FlexiPort can be switced between UART and I2C by parameters
* The RCoutput module has been completely rewritten.
* For the PWM outputs, the error in setting the timer frequency has been compensated.
* Fixed bug with OneShot
* added parameter to set PWM mode
* added used memory reporting
* added I2C error reporting
* realized low-power idle state with WFE, TIMER6 used to generate events each 1uS
* added HAL_RC_INPUT parameter to allow to force RC input module
* added used stack reporting
* added generation of .DFU file
* time-consuming operations moved out from interrupt level to IO_Completion level with lowest possible priority
* added support for Clock Security System - if HSE fails in air system will use HSI instead
* added boardEmergencyHandler which will be called on any Fault or Panic() before halt - eg. to release parachute
* motor layout switched to per-board basis
* console assignment switched to per-board basis
* HAL switched to new DMA api with completion interrupts
* AirbotV2 is fully supported with SD card and OSD (OSD not tested, just compiles)
* added support for reading from SD card via USB - HAL parameter allows to be USB MassStorege
* fixed BUG in scheduler which periodically causes HardFault
* added check for stack overflow for low priority tasks
* all boards formats internal flash chip as FAT and allows access via USB
* added spi flash autodetection
* added support for TRIM command on FAT-formatted Dataflash
* fixed bug in hardware I2C driver which spoils writes to I2C2
* fixed bug in log write with different meanings of flags in FatFs and Posix
* rewritten SD library to support 'errno' and early distinguish between file and dir
* compass processing (4027uS) and baro processing(1271uS)  moved out from interrupt level to low-priority io level, because its
 execution time spoils loop time (500Hz = 2000uS for all)
* added reformatting of DataFlash in case of hard filesystem errors, which fixes FatFs bug when there is no free space 
 after file overflows and then deleted
* added autodetect for all known types of baro on external I2C bus
* added autodetect for all known types ofcompass on external I2C bus
* added check to I2C_Mgr for same device on same bus - to prevent autodetection like MS5611 (already initialized) as BMP_085
* added time offset HAL parameter
* added time syncronization between board's time and GPS time - so logs now will show real local date&time
* i2c driver is fully rewritten, added parsing of bus error flags - ARLO & BERR
* errors at STOP don't cause data loss or time errors - bus reset scheduled as once io_task
* added parsing of TIMEOUT bus flag
* added asyncronous bus reset in case when loockup occures after STOP generation
* full BusReset changed to SoftReset on 1st try
* new SoftI2C driver uses timer and works in interrupts
* added support for SUMD and non-inverted SBUS via PPM pins
* added support for SUMD via UARTs
* MPU not uses FIFO - data readed out via interrupts
* added parametr allowing to defer EEPROM save up to disarm
* optimized multitask to not switch context if next task is the same as current. Real context switch occures in ~4% of calls to task scheduler
* all work with task list moved out to ISR level so there is no race condition anymore
* all work with semaphores moved out to the same ISR level so serialized by hardware and don't requires disabling interrupts
* added parameter RC_FS to enable all RC failsafe
* added disabling of data cache on flash write, just for case (upstream has this update too)
* added a way to schedule context switch from ISR, eg. at data receive or IO_Complete
* added timeout to SPI flags waiting
* now task having started any IO (DMA or interrupts) goes to pause and resumes in IO_Completion ISR, not eating CPU time in wait loop
* optimized I2C wait times to work on noisy lines
* greatly reduced time of reformatting of DataFlash to FAT
* I2C driver fully rewritten again to work via interrupts - no DMA, no polling
* compass and baro gives bus semaphore ASAP to allow bus operations when calculations does
* buzzer support
* full status on only 2 leds - GPS sats count, failsafe, compass calibration, autotune etc
* support for SBUS on any UART
* PWM_IN is rewritten to use HAL drivers, as result its size decreased four times (!)
* working PPM on AirbotV2/V3
* added ability to connect buzzer to arbitrary pin (parameter BUZZ_PIN)
* added priority to SPI DMA transfers
* overclocking support
* OSD is working
* 'boards' folder moved from 'wirish' to HAL directory, to help to find them
* added translation of decoded serial data from PPMn input port to fake UARTs
* reduced to ~1.5uS time from interrupt to resuming task that was waiting that interrupt
* unified NVIC handling
* fixed bug in parsing of .osd file
* fixed bug in scheduler that can cause task freeze
* fixed bug in OSD_Uart that cause hang if port not listened
* ...
* a lot of minor enhancements


Warning!!!
EEPROM emulation in Flash cause periodic program hunging on time of sector erase! So to allow auto-save parameters
like MOT_THST_HOVER - MOT_HOVER_LEARN to be 2 you should defer parameter writing (Param HAL_EE_DEFER)

--------
Старый расклад времени:
25.7 опрос датчиков, из них 7.7% ожидание
остается 74.3%, из них 65% ожидание
собственно на расчет с частотой 400 раз в секунду уходит около 10% всего времени

Новый расклад времени:
task 0 (0x0000000080ACE09) time:   12.18% mean     77.3uS max   131uS full       0uS wait sem.    122uS main task
task 1 (0x0000000080AD735) time:   78.50% mean      4.2uS max    33uS full       0uS wait sem.      0uS idle
task 2 (0x80AE3E920004CD0) time:    0.28% mean     13.9uS max    38uS full      38uS wait sem.      0uS analog IO
task 3 (0x8049C0120007E50) time:    1.90% mean     44.8uS max   116uS full     617uS wait sem.      8uS baro
task 4 (0x0000000080ADACD) time:    1.88% mean      9.5uS max    35uS full    1003uS wait sem.      0uS IO tasks
task 5 (0x804CB7F2000CF08) time:    1.74% mean     51.3uS max   117uS full     763uS wait sem.    189uS compass
task 6 (0x80546152000CFA0) time:    3.40% mean     33.5uS max    45uS full     155uS wait sem.     10uS MPU
task 7 (0x0000000080AE225) time:    0.01% mean     85.6uS max   112uS full    1928uS wait sem.      0uS stats
занято 21.5% , из них 7.5% опрос датчиков и 12.2 это основной процесс

Шедулер:
by timer 12.05% sw  5.88%  таймерный планировщик составляет 12% от всех вызовов, при этом контекст переключается в 6% случаев, итог - 0.72% 
in yield 79.97% sw 97.13%  добровольное завершение кванта с вызовом планировщика это 80%, вероятность переключения 97%, итог - 77.6%
in tails  7.98% sw 71.75%  переключение по таймеру до истечения кванта - 8%, вероятность 72%, итог - 5.8%
вывод: кооперативная многозадачность на 77% :)

new I2C driver stats:
sched time: by timer 10.53% sw 12.38% in yield 82.38% sw 99.62% in tails  7.09% sw 65.64%
task 0 (0x0000000080ACA3D) time:   12.23% mean     78.4uS max   121uS full       0uS wait sem.     28uS
task 1 (0x0000000080AD369) time:   81.32% mean      4.3uS max    33uS full       0uS wait sem.      0uS
task 2 (0x80AE02120004C9C) time:    0.28% mean     13.9uS max    37uS full      40uS wait sem.      0uS
task 3 (0x8049CE920007E38) time:    0.38% mean     12.0uS max    40uS full     900uS wait sem.    557uS
task 4 (0x0000000080AD705) time:    1.94% mean      9.7uS max    35uS full     444uS wait sem.      0uS
task 5 (0x804CD492000CF20) time:    0.34% mean     14.0uS max    43uS full     856uS wait sem.    378uS
task 6 (0x80547D92000CFA0) time:    3.38% mean     33.2uS max    49uS full     168uS wait sem.      8uS
task 7 (0x0000000080ADE5D) time:    0.01% mean     84.5uS max   107uS full    7044uS wait sem.      0uS

OSD task uses 0.5% of CPU

Timer usage:

1 
2 RC-Output
3 RC-Output
4 soft_i2c0, PPM_IN on AirbotV2
5 micros()
6 event generation for WFE
7 scheduler
8 PPM_IN
9 soft_i2c1
10 soft_i2c2
11 
12 PPM_IN
13 driver's io_completion
14 schedule tail timer


stacks:
main 0x1000FC00 revoMini uses up to f600 so uses only 0x600 bytes
