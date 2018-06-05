Teeny1S flight controller

board connection:

this board REQUIRES external Compass & Baro via I2C bus. 
To get (soft) I2C bus you should remove transistor on BUZZER output and short its input and output pins. Will be:
SCL - LED_STRIP
SDA = BUZZ-

GPS should be pre-configured to 115200, 10/s, and connected by its TX to board's DSM/IBUS pin (UART1 RX)






