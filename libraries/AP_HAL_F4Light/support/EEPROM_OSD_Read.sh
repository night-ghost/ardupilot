#!/bin/sh

/usr/local/stlink/st-flash  --reset read  $1.bin 0x0800C000  0x4000


