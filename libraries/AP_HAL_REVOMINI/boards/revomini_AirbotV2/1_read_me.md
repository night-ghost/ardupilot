board connection:

just see board's documentation.

default connection:
Telemetry to UART1 (Serial3 in MP) 
GPS  to      UART6 (Serial2) 
Built-in OSD is     Serial5

this board REQUIRES external Compass via I2C bus

Built-in OSD can be configured via files in root directory of SD card:

* eeprom.osd for configuration,  and
* font.mcm for fonts (this file will be deleted after flashing)

also supported connection to built-in OSD with CT from my MinimOSD (https://github.com/night-ghost/minimosd-extra)
* set HAL_CONNECT_COM parameter to 5, then reboot / power cycle
* USB will be connected to OSD after reboot, supported load/store/fonts in MAVLink mode

