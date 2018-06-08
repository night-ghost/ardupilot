#!/bin/sh
/usr/local/stlink/st-flash  --reset write $1 0x0800C000 


