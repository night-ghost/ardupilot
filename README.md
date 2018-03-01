#ArduPilot Project port for almost arbitrary F4 board, starting Revolution/RevoMini boards, and including Omnibus, Cl-Racing F4 and so on#

* almost all code is fully rewritten
* external I2C bus moved out from FlexiPort by Soft_I2C driver so we always has at least 3 UARTs
* added 1 full-functional UART (only for quads) and 1 RX-only UART for DSM satellite receiver on OpLink connector
* added buzzer support
* stack now in CCM memory
* PPM inputs can be used for SBUS, DSM and SUMD protocols
* all hardware description tables are now 'const' and locates in flash
* separated USB and UART drivers
* new SoftwareSerial driver based on ST appnote
* now it uses MPU6000 DRDY output for interrupts
* removed all compiler's warnings
* ported and slightly altered bootloader to support flashing and start firmware automatically at addresses 8010000 and 8020000
  (2 low 16k flash pages are used to emulate EEPROM)           
* EEPROM emulation altered to ensure the reliability of data storage at power failures
* optimized EEPROM usage by changing from 1-byte to 2-byte writes
* added support for all known baros on external I2C
* added translation layer between system PWM_MODES and board's PWM_MODES
* supported reboot to DFU mode (via "reboot to PX4 bootloader" in MP)
* after any Fault or Panic() automatically reboots to DFU mode
* diversity on RC_Input - 2 PPM inputs
* unified exception handling
* added ability to bind Spectrum satellite without managed 3.3 DC-DC (requires short power off)
* added support for Arduino32 reset sequence - negative DTR edge on 1200 baud or '1eaf' packet with high DTR
* fixed hang on dataflash malfunction
* fixed USB characters loss *without* hangup if disconnected
* added failsafe on receiver's hangup - if no one channel changes in 60 seconds
* added HAL parameters support
* changed layout to simplify support of slightly different boards - eg. AirbotF4
* full support for AirbotF4 and AurbotV2 (AKA Ombibus) as separate binaries
* added support for servos on Input port unused pins
* Added handling of FLASH_SR error bits, including automatic clearing of write protection 
* added Arduino-like support of relay on arbitrary pin 
* simplified UART driver, buffering now used even in non-blocking mode
* EEPROM emulation locks flash after each write
* EEPROM error handling
* fixed Ardupilot's stealing of Servos even they marked as "Unused"
* added compilation date & time to log output
* added SBUS input via any USART as it wired on Airbot boards
* added per-board read_me.md files
* fixed Dataflash logs bug from mainstream - now logs are persists between reboots!
* DMA mode for lagre SPI transfers
* USB virtual com-port can be connected to any UART - eg. for OSD or 3DR modems setup
* any UART can be connected to ESC for 4-way interface
* support for logs on SD card for AirbotV2 board
* fixed 2nd Dataflash logs bug from mainstream - now logs are persists between reboots even on boards having chips with 64k sector
* I2C wait time limited to 3ms - no more forever hangs by external compass errors
* FlexiPort can be switced between UART and I2C by parameters
* The RCoutput module has been completely rewritten.
* For the PWM outputs, the error in setting the timer frequency has been compensated.
* added parameter to set PWM mode
* added used memory reporting
* added I2C error reporting
* realized low-power idle state with WFE, TIMER6 used to generate events each 1uS
* added HAL_RC_INPUT parameter to allow to force RC input module
* added used stack reporting
* added generation of .DFU file
* time-consuming operations moved out from interrupt level to IO_Completion level with low priority
* added support for Clock Security System - if HSE fails in air system will use HSI instead
* added boardEmergencyHandler which will be called on any Fault or Panic() before halt - eg. to release parachute
* motor layout switched to per-board basis
* console assignment switched to per-board basis
* HAL switched to new DMA api with completion interrupts
* AirbotV2 is fully supported with SD card and OSD
* added support for reading from SD card via USB - HAL parameter allows to be USB MassStorege
* added check for stack overflow for low priority tasks
* all boards formats internal flash chip as FAT and allows access via USB
* added spi flash autodetection
* added support for TRIM command on FAT-formatted Dataflash
* rewritten SD library to support 'errno' and early distinguish between file and dir
* added reformatting of DataFlash in case of hard filesystem errors, which fixes FatFs bug when there is no free space
 after file overflows and then deleted
* added autodetect for all known types of baro on external I2C bus
* added autodetect for all known types ofcompass on external I2C bus
* added check to I2C_Mgr for same device on same bus - to prevent autodetection like MS5611 (already initialized) as BMP_085
* added time offset HAL parameter
* added time syncronization between board's time and GPS time - so logs now will show real local date&time
* added asyncronous bus reset in case when loockup occures after STOP generation
* full BusReset changed to SoftReset on 1st try
* new SoftI2C driver uses timer and works in interrupts
* added support for SUMD and non-inverted SBUS via PPM pins
* added support for SUMD via UARTs
* MPU not uses FIFO - data readed out via interrupts
* added parameter allowing to defer EEPROM save up to disarm
* optimized preemptive multitask to not switch context if next task is the same as current. Real context switch occures in ~4% of calls to task scheduler
* all work with task list moved out to ISR level so there is no race condition anymore
* all work with semaphores moved out to the same ISR level so serialized by hardware and don't requires disabling interrupts
* added parameter RC_FS to enable all RC failsafe
* added disabling of data cache on flash write, just for case (upstream has this update too)
* added a way to schedule context switch from ISR, eg. at data receive or IO_Complete
* added timeout to SPI flags waiting
* now task having started any IO (DMA or interrupts) goes to pause and resumes in IO_Completion ISR, not eating CPU time in wait loop
* greatly reduced time of reformatting of DataFlash to FAT
* I2C driver fully rewritten again to work via interrupts - no DMA, no polling, and excellent work on noisy lines
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
* SoftSerial driver rewritten to not use PWM dependency. Now it can use any pin with timer for RX and any pin for TX, and there
   can be any number of SoftSerial UARTs
* added per-task stack usage
* SPI driver rewritten: added ISR mode instead of polling, all transfers are monolitic (not divded to send and receive parts), setup for receive now in ISR
* all DataFlash reads and writes now in single SPI transfer
* removed usage of one-byte SPI functions from SD driver
* added support of criticalSections to Scheduler, which protect code from task switch without disabling interrupts
* added CS assert/release delays to SPI device descriptrion table
* added partial MPU support (only to protect from process stack overflow)
* removed -fpermissive from GCC options
* class SD is slightly redesigned, reducing the memory consumption by half (!)
* optimized dma_init_transfer() function: now it twice faster and requires 3 times less memory
* added SD size to bootlog
* SPI via interrupts now works
* added pin names to simplify porting of boards
* added DSM rssi as last channel
* IO tasks excluded from priority boost on yield()
* added awakening of main thread after receiving of data from MPU
* ArduCopter loop at 1KHz! fixed all issues, mean scheduling error is only 10uS - 10 times less than OS tick!
* narowed type for timer_channel
* added support for PWM outputs on N-channels of advanced timers
* added support for inverted buzzer
* added support for passive buzzer
* added support for Devo telemetry protocol
* fixed Soft_I2C timeout
* all waits for SD answers moved to ISR as finite state machine
* a try to support Ardupilot parameters on builtin OSD (untested)

* ...
* a lot of minor enhancements

Incompatibility!!!

Since this controller is intended primarily for very small aircraft, the following unnecessary functions are disabled by default:
* Terrain following - there is no SD card
* Push Button
* Sprayer support
* EPM gripper support
* CLI support

If some of this is needed it can be enabled later

Also, this HAL now is not compatible with LibMapple/ArduinoSTM32 ("wirish" folder) - all imported files are highly altered.

Warning!!!
EEPROM emulation in Flash cause periodic program hunging on time of sector erase! So to allow auto-save parameters 
like MOT_THST_HOVER - MOT_HOVER_LEARN to be 2 you should defer parameter writing (Param HAL_EE_DEFER)

If you like this project and want to support further development - you can do it! 
[![Donate](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=SLUC8B3U7E7PS) USD
[![Donate](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=HXRA6EXZJ489C) EUR

[Russian thread](http://www.ykoctpa.ru/groups/eye-in-a-sky/forum/topic/ardupilot-na-platax-openpilot-revolution-revomini/)

***********************************************

# ArduPilot Project

[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/ArduPilot/ardupilot?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

[![Build SemaphoreCI](https://semaphoreci.com/api/v1/ardupilot/ardupilot/branches/master/badge.svg)](https://semaphoreci.com/ardupilot/ardupilot)

[![Build Travis](https://travis-ci.org/ArduPilot/ardupilot.svg?branch=master)](https://travis-ci.org/ArduPilot/ardupilot)

[![Coverity Scan Build Status](https://scan.coverity.com/projects/5331/badge.svg)](https://scan.coverity.com/projects/ardupilot-ardupilot)

## The ArduPilot project is made up of: ##

- ArduCopter (or APM:Copter) : [code](https://github.com/ArduPilot/ardupilot/tree/master/ArduCopter), [wiki](http://ardupilot.org/copter/index.html)

- ArduPlane (or APM:Plane) : [code](https://github.com/ArduPilot/ardupilot/tree/master/ArduPlane), [wiki](http://ardupilot.org/plane/index.html)

- ArduRover (or APMrover2) : [code](https://github.com/ArduPilot/ardupilot/tree/master/APMrover2), [wiki](http://ardupilot.org/rover/index.html)

- ArduSub (or APM:Sub) : [code](https://github.com/ArduPilot/ardupilot/tree/master/ArduSub), [wiki](http://ardusub.com/)

- Antenna Tracker : [code](https://github.com/ArduPilot/ardupilot/tree/master/AntennaTracker), [wiki](http://ardupilot.org/antennatracker/index.html)

## User Support & Discussion Forums ##

- Support Forum: <http://discuss.ardupilot.org/>

- Community Site: <http://ardupilot.org>

## Developer Information ##

- Github repository: <https://github.com/ArduPilot/ardupilot>

- Main developer wiki: <http://dev.ardupilot.org>

- Developer discussion: <http://discuss.ardupilot.org>

- Developer email group: drones-discuss@googlegroups.com. Deprecated November 2016. Included for historical reference.

## Contributors ##

- [Github statistics](https://github.com/ArduPilot/ardupilot/graphs/contributors)

## How To Get Involved ##

- The ArduPilot project is open source and we encourage participation and code contributions: [guidelines for contributors to the ardupilot codebase](http://dev.ardupilot.org/wiki/guidelines-for-contributors-to-the-apm-codebase)

- We have an active group of Beta Testers especially for ArduCopter to help us find bugs: [release procedures](http://dev.ardupilot.org/wiki/release-procedures)

- Desired Enhancements and Bugs can be posted to the [issues list](https://github.com/ArduPilot/ardupilot/issues).

- Helping other users with log analysis on [http://discuss.ardupilot.org/](http://discuss.ardupilot.org/) is always appreciated:

- There is a group of wiki editors as well in case documentation is your thing: <ardu-wiki-editors@googlegroups.com>

- Developer discussions occur on <drones-discuss@google-groups.com>

## License ##

The ArduPilot project is licensed under the GNU General Public
License, version 3.

- [Overview of license](http://dev.ardupilot.com/wiki/license-gplv3)

- [Full Text](https://github.com/ArduPilot/ardupilot/blob/master/COPYING.txt)

## Maintainers ##

Ardupilot is comprised of several parts, vehicles and boards. The list below
contains the people that regularly contribute to the project and are responsible
for reviewing patches on their specific area. See [CONTRIBUTING.md](.github/CONTRIBUTING.md) for more information.

- [Andrew Tridgell](https://github.com/tridge):
  - ***Vehicle***: Plane, AntennaTracker
  - ***Board***: APM1, APM2, Pixhawk, Pixhawk2, PixRacer
- [Randy Mackay](https://github.com/rmackay9):
  - ***Vehicle***: Copter, AntennaTracker
- [Grant Morphett](https://github.com/gmorph):
  - ***Vehicle***: Rover
- [Tom Pittenger](https://github.com/magicrub):
  - ***Vehicle***: Plane
- [Chris Olson](https://github.com/ChristopherOlson) and [Bill Geyer](https://github.com/bnsgeyer):
  - ***Vehicle***: TradHeli
- [Paul Riseborough](https://github.com/priseborough):
  - ***Subsystem***: AP_NavEKF2
  - ***Subsystem***: AP_NavEKF3
- [Lucas De Marchi](https://github.com/lucasdemarchi):
  - ***Subsystem***: Linux
- [Peter Barker](https://github.com/peterbarker):
  - ***Subsystem***: DataFlash
  - ***Subsystem***: Tools
- [Michael du Breuil](https://github.com/WickedShell):
  - ***Subsystem***: SMBus Batteries
  - ***Subsystem***: GPS
- [Francisco Ferreira](https://github.com/oxinarf):
  - ***Bug Master***
- [Matthias Badaire](https://github.com/badzz):
  - ***Subsystem***: FRSky
- [Eugene Shamaev](https://github.com/EShamaev):
  - ***Subsystem***: CAN bus
  - ***Subsystem***: UAVCAN
- [Víctor Mayoral Vilches](https://github.com/vmayoral):
  - ***Board***: PXF, Erle-Brain 2, PXFmini
- [Mirko Denecke](https://github.com/mirkix):
  - ***Board***: BBBmini, BeagleBone Blue, PocketPilot
- [Georgii Staroselskii](https://github.com/staroselskii):
  - ***Board***: NavIO
- [Emile Castelnuovo](https://github.com/emilecastelnuovo):
  - ***Board***: VRBrain
- [Julien BERAUD](https://github.com/jberaud):
  - ***Board***: Bebop & Bebop 2
- [Matt Lawrence](https://github.com/Pedals2Paddles):
  - ***Vehicle***: 3DR Solo & Solo based vehicles
- [Gustavo José de Sousa](https://github.com/guludo):
  - ***Subsystem***: Build system
- [Craig Elder](https://github.com/CraigElder):
  - ***Administration***: ArduPilot Technical Community Manager
- [Jacob Walser](https://github.com/jaxxzer):
  - ***Vehicle***: Sub
