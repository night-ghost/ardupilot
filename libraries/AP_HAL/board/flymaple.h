
#define AP_HAL_BOARD_DRIVER AP_HAL_FLYMAPLE
#define HAL_BOARD_NAME "FLYMAPLE"
//#define HAL_CPU_CLASS HAL_CPU_CLASS_75 no AP_AHRS_NavEKF so error in compiling
#define HAL_CPU_CLASS HAL_CPU_CLASS_150
#define HAL_STORAGE_SIZE            4096
#define HAL_STORAGE_SIZE_AVAILABLE  HAL_STORAGE_SIZE
//#define HAL_INS_DEFAULT HAL_INS_FLYMAPLE
#define HAL_INS_DEFAULT HAL_INS_MPU60XX_I2C
#define HAL_INS_MPU60x0_I2C_BUS 1
#define HAL_INS_MPU60x0_I2C_ADDR 0x68

#define HAL_BARO_DEFAULT HAL_BARO_BMP085
#define HAL_COMPASS_DEFAULT HAL_COMPASS_HMC5843
#define HAL_COMPASS_HMC5843_I2C_BUS 1
#define HAL_COMPASS_HMC5843_I2C_ADDR 0x3C

#define HAL_SERIAL0_BAUD_DEFAULT 115200
#define CONFIG_HAL_BOARD_SUBTYPE HAL_BOARD_SUBTYPE_NONE
#define HAL_BARO_BMP085_BUS 1
#define HAL_BARO_BMP085_I2C_ADDR 0x77  //(0xEE >> 1)
#define BMP085_EOC 30 // Arduino's pin with End Of Conversion
