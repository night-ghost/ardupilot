#!/bin/sh

#production binary for bootloader
dfu-util -a 0 --dfuse-address 0x08010000:unprotect:force -D ../../../../../ArduCopter/f4light_MatekF405_CTR.bin -R



