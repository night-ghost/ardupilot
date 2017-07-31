#ArduPilot Project port for Revolution/RevoMini boards#

* near a half of code is fully rewritten
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
* added support for baro MS5611 on external I2C
* micros() call uses 32-bit hardware timer instead of systick_micros()
* added translation layer between system PWM_MODES and board's PWM_MODES
* added initial support for DMA
* added support for all timers
* supported reboot to DFU mode (via "reboot to PX4 bootloader" in MP)
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
* added failsafe on receiver's hangup - if no one channel changes in 60 seconds
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
* ...
* a lot of minor enhancements

Incompatibility!!!

Since this controller is intended primarily for very small aircraft, the following unnecessary functions are disabled by default:
* Terrain following - there is no SD card
* Optical Flow
* ADSB support
* Precision landing
* Push Button
* Rangefinder support
* Sprayer support
* EPM gripper support
* Mount control support
* CLI support

If some of this is needed it can be enabled later

Also, this HAL now is not compatible with LibMapple/ArduinoSTM32 ("wirish" folder) - all imported files are altered.

Warning!!!
EEPROM emulation in Flash cause periodic program hunging on time of sector erase! So never allow auto-save parameters 
like MOT_THST_HOVER - MOT_HOVER_LEARN should NOT be 2!

If you like this project and want to support further development - you can do it! [![Donate](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](htt
ps://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=SLUC8B3U7E7PS)USD
  [![Donate](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=HXRA6EXZJ489C)
 EUR

[Russian thread](http://www.ykoctpa.ru/groups/eye-in-a-sky/forum/topic/ardupilot-na-platax-openpilot-revolution-revomini/)

***********************************************

# ArduPilot Project

[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/ArduPilot/ardupilot?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

[![Build SemaphoreCI](https://semaphoreci.com/api/v1/projects/4d28a40d-b6a6-4bfb-9780-95d92aabb178/667563/badge.svg)](https://semaphoreci.com/diydrones/ardupilot)

[![Build Travis](https://travis-ci.org/ArduPilot/ardupilot.svg?branch=master)](https://travis-ci.org/ArduPilot/ardupilot)

[![Coverity Scan Build Status](https://scan.coverity.com/projects/5331/badge.svg)](https://scan.coverity.com/projects/diydrones-ardupilot)

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

- [Andrew Tridgell](https://github.com/tridge)
  - ***Vehicle***: Plane, AntennaTracker
  - ***Board***: APM1, APM2, Pixhawk, Pixhawk2, PixRacer
- [Randy Mackay](https://github.com/rmackay9)
  - ***Vehicle***: Copter, AntennaTracker
- [Robert Lefebvre](https://github.com/R-Lefebvre)
  - ***Vehicle***: TradHeli
- [Grant Morphett](https://github.com/gmorph):
  - ***Vehicle***: Rover
- [Tom Pittenger](https://github.com/magicrub)
  - ***Vehicle***: Plane
- [Paul Riseborough](https://github.com/priseborough)
  - ***Subsystem***: AP_NavEKF2
  - ***Subsystem***: AP_NavEKF3
- [Lucas De Marchi](https://github.com/lucasdemarchi)
  - ***Subsystem***: Linux
- [Peter Barker](https://github.com/peterbarker)
  - ***Subsystem***: DataFlash
  - ***Subsystem***: Tools
- [Michael du Breuil](https://github.com/WickedShell)
  - ***Subsystem***: SMBus Batteries
  - ***Subsystem***: GPS
- [Francisco Ferreira](https://github.com/oxinarf)
  - ***Bug Master***
- [Matthias Badaire](https://github.com/badzz)
  - ***Subsystem***: FRSky
- [Eugene Shamaev](https://github.com/EShamaev)
  - ***Subsystem***: CAN bus
  - ***Subsystem***: UAVCAN
- [Víctor Mayoral Vilches](https://github.com/vmayoral)
  - ***Board***: PXF, Erle-Brain 2, PXFmini
- [Mirko Denecke](https://github.com/mirkix)
  - ***Board***: BBBmini, BeagleBone Blue
- [Georgii Staroselskii](https://github.com/staroselskii)
  - ***Board***: NavIO
- [Emile Castelnuovo](https://github.com/emilecastelnuovo)
  - ***Board***: VRBrain
- [Julien BERAUD](https://github.com/jberaud)
  - ***Board***: Bebop & Bebop 2
- [Pritam Ghanghas](https://github.com/pritamghanghas)
  - ***Board***: Raspilot
- [Matt Lawrence](https://github.com/Pedals2Paddles)
  - ***Vehicle***: 3DR Solo & Solo based vehicles
- [Gustavo José de Sousa](https://github.com/guludo)
  - ***Subsystem***: Build system
- [Craig Elder](https://github.com/CraigElder)
  - ***Administration***: ArduPilot Technical Community Manager
- [Jacob Walser](https://github.com/jaxxzer)
  - ***Vehicle***: Sub
