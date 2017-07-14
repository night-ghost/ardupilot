to add your own board on any STM32F4XX you should comltete thease steps:


 *  give a name for your board - eg. yourboard_F4
 *  make directory  "yourboard_F4" in folder wirish/boards
 *  copy all files from revomini_MP32V1F4 to this folder
 *  open board.c and adjust PIN_MAP[] array to your CPU usage
 *  change in "board.h" pin definitions. see pin number in PIN_MAP array
 *  check settings in board.h
 *  copy "board_REVOMINI.mk" in folder /mk to "yourboard.mk"
 *  inside "yourboard.mk" change constant BOARD  ?= revomini_MP32V1F4 to  BOARD  ?= yourboard_F4
 
 