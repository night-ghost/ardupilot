# Try "make help" first

#TOOLCHAIN = NATIVE
TOOLCHAIN = ARM

ARM_CXX     :=  arm-none-eabi-g++
ARM_CC      :=  arm-none-eabi-gcc
ARM_AS      :=  arm-none-eabi-gcc
ARM_AR      :=  arm-none-eabi-ar
#ARM_LD      :=  arm-none-eabi-g++
ARM_LD      :=  arm-none-eabi-ld
ARM_GDB     :=  arm-none-eabi-gdb
ARM_OBJCOPY :=  arm-none-eabi-objcopy

#TOOLCHAIN = arm-none-eabi
include $(MK_DIR)/find_tools.mk

.DEFAULT_GOAL := all

##
## Target-specific configuration.  This determines some compiler and
## linker options/flags.
##


PATH:=$(PATH):/usr/local/bin

# these default to a Maple Flash build.
BOARD            ?= revomini_MP32V1F4
MEMORY_TARGET    ?= flash

##
## Useful paths, constants, etc.
##

STFLASH := /usr/local/stlink/st-flash
STUTIL  := /usr/local/stlink/st-util

FLASH_ADDRESS := 0x08010000

UPLOADER := dfu-util
#leaflabs bootloader
USBID := 1EAF:0003

BLDROOT := $(SRCROOT)/..

#BUILD_PATH       := $(BLDROOT)/build
BUILD_PATH       := $(BUILDROOT)

LIBRARIES_PATH   := $(BLDROOT)/libraries
REVO_PATH        := $(LIBRARIES_PATH)/AP_HAL_REVOMINI
WIRISH_PATH      := $(REVO_PATH)/wirish
HARDWARE_PATH    := $(REVO_PATH)/hardware
HAL_PATH         := $(REVO_PATH)/hardware/hal
STM32_PATH       := $(HARDWARE_PATH)/STM32F4xx_DSP_StdPeriph_Lib_V1.1.0
STM32USB_PATH    := $(HARDWARE_PATH)/STM32_USB_Driver
cpu_flags        := -mthumb -mcpu=cortex-m4 -march=armv7e-m -mfpu=fpv4-sp-d16 -mfloat-abi=hard


FIRMWARE_PATH    := $(BLDROOT)/$(SKETCH)

# List of input files to the sketch.cpp file in the order they should be appended to create it
LIBRARY_INCLUDES := 

SUPPORT_PATH     := $(BLDROOT)/libraries/AP_HAL_REVOMINI/support
TOOLS_PATH       := $(BLDROOT)/libraries/AP_HAL_REVOMINI/support/tools
# Support files for linker
LDDIR            := $(WIRISH_PATH)/boards/$(BOARD)/ld


# $(BOARD)- and $(MEMORY_TARGET)-specific configuration
include $(WIRISH_PATH)/boards/$(BOARD)/target-config.mk

SIZE     := arm-none-eabi-size
DISAS    := arm-none-eabi-objdump

##
## Compilation flags
##

EXTRAFLAGS += -DHAVE_STD_NULLPTR_T=0  -DHAVE_BYTESWAP_H=0
EXTRAFLAGS += $(SKETCHLIBINCLUDES) -DARDUPILOT_BUILD -DTESTS_MATHLIB_DISABLE  -DSKETCH_MAIN=ArduPilot_main
EXTRAFLAGS += -DGIT_VERSION="\"$(GIT_VERSION) $(shell date --rfc-3339=seconds)\""


# -Wformat=1 
# -Werror=init-self -Wno-missing-field-initializers 

#-Werror
WARNFLAGS       =   -Wall -Wshadow -Wpointer-arith -Wcast-align -Wno-psabi -Wno-unused-parameter -Wno-error=cast-align -Wno-error=unused-but-set-variable
WARNFLAGS      +=   -Wwrite-strings -Wformat=2 -Wshadow -Wfloat-equal -Wpointer-arith -Wlogical-op -Wmissing-declarations -Wpacked -Wno-pragmas
WARNFLAGS      +=   -Wextra -Wlogical-op  -Wno-unknown-pragmas -Wno-redundant-decls -Wno-packed -Wno-error=double-promotion -Wno-error=type-limits
WARNFLAGS      +=   -Wno-error=unused-variable -Wno-error=reorder -Wno-error=float-equal -Wno-error=unused-parameter -Wno-missing-field-initializers
WARNFLAGS      +=   -Wno-error=pmf-conversions -Wno-error=missing-declarations -Wno-error=unused-function -Werror=format-security -Werror=array-bounds
WARNFLAGS      +=   -Wno-error=unused-label

#OPTFLAGS        = -Og
OPTFLAGS        = -Os
OPTFLAGS       += -fsingle-precision-constant -g3 -fno-strict-aliasing -fno-strength-reduce -fomit-frame-pointer
OPTFLAGS       += -fno-builtin-printf -fno-aggressive-loop-optimizations -fpredictive-commoning
OPTFLAGS       += -fassociative-math
# -freciprocal-math -ffast-math - cause EKF errors
OPTFLAGS       += -fno-signed-zeros -fno-trapping-math 
OPTFLAGS       += -finline-functions-called-once -fearly-inlining -finline-small-functions
OPTFLAGS       += -fmerge-all-constants 

WARNFLAGSCXX    =   -Wno-reorder
DEPFLAGS        =   -MD -MT $@

GLOBAL_FLAGS    := -D$(VECT_BASE_ADDR)
GLOBAL_FLAGS    += -DBOARD_$(BOARD)

GLOBAL_FLAGS    += -DMCU_$(MCU)
GLOBAL_FLAGS    += -DCONFIG_HAL_BOARD=$(HAL_BOARD)
GLOBAL_FLAGS    += -DSTM32F4XX
GLOBAL_FLAGS    += -DUSE_STDPERIPH_DRIVER
GLOBAL_FLAGS    += -DHSE_VALUE=8000000
GLOBAL_FLAGS    += -DARM_MATH_CM4
GLOBAL_FLAGS    += -DUSE_OTG_FS_CORE
GLOBAL_FLAGS    += -DUSE_USB_OTG_FS
GLOBAL_FLAGS    += -DUSE_EMBEDDED_PHY
GLOBAL_FLAGS    += -D__FPU_PRESENT
GLOBAL_FLAGS    += -D__FPU_USED=1
GLOBAL_FLAGS    += -DAPM_BUILD_DIRECTORY=APM_BUILD_$(SKETCH)
GLOBAL_FLAGS    += $(WARNFLAGS) $(DEPFLAGS)  $(EXTRAFLAGS) $(OPTS)
#
GLOBAL_FLAGS    += -mthumb -mcpu=cortex-m4 -march=armv7e-m -mfpu=fpv4-sp-d16 -mfloat-abi=hard
GLOBAL_FLAGS    += -ffunction-sections
GLOBAL_FLAGS    += -fdata-sections


#Produce debugging information in the operating system's native format
GLOBAL_FLAGS   += -ggdb
#GLOBAL_FLAGS += -v -save-temps # for GCC bugreport

# GLOBAL_CFLAGS -----------------------------------------------------------------------------------
GLOBAL_CFLAGS   := $(cpu_flags)
#GLOBAL_CFLAGS   += -nostdlib           #Do not use the standard system startup files or libraries when linking
GLOBAL_CFLAGS   += -Wall               #This enables all the warnings about constructions that some users consider questionable, and that are easy to avoid (or modify to prevent the warning), even in conjunction with macros
GLOBAL_CFLAGS   += $(GLOBAL_FLAGS) $(OPTFLAGS)
GLOBAL_CFLAGS   += -include $(WIRISH_PATH)/boards/$(BOARD)/board.h 


# GLOBAL_CXXFLAGS ---------------------------------------------------------------------------------
GLOBAL_CXXFLAGS := -fno-rtti       #Disable generation of information about every class with virtual functions for use by the C++ runtime type identification features
GLOBAL_CXXFLAGS += -fno-exceptions -fno-threadsafe-statics # not true C++
GLOBAL_CXXFLAGS += -fconserve-space -fno-enforce-eh-specs

#GLOBAL_CXXFLAGS += -Wall  -std=gnu++0x
GLOBAL_CXXFLAGS += -Wall  -std=gnu++11
#GLOBAL_CXXFLAGS += -Wall  -std=gnu++1y
#GLOBAL_CXXFLAGS += -Wall  -std=c++11

# Downgrade some diagnostics about nonconformant code from errors to warnings. Thus, using "-fpermissive" will allow some nonconforming code to compile.
GLOBAL_CXXFLAGS += -fpermissive $(GLOBAL_CFLAGS)
# missing definitions
GLOBAL_CXXFLAGS += -c -include $(WIRISH_PATH)/defs.h

# GLOBAL_ASFLAGS ----------------------------------------------------------------------------------
GLOBAL_ASFLAGS  := $(cpu_flags)
GLOBAL_ASFLAGS  += -x assembler-with-cpp
GLOBAL_ASFLAGS  += $(GLOBAL_FLAGS) $(OPTFLAGS)

# GLOBAL_LDFLAGS ----------------------------------------------------------------------------------
LDFLAGS         := $(cpu_flags)
LDFLAGS         += -static
LDFLAGS         += -lm -lgcc
LDFLAGS         += -Wall
LDFLAGS         += -Wl,--gc-sections
LDFLAGS         += -Wl,--cref
#LDFLAGS         += -Wl,--print-gc-sections
LDFLAGS         += -Wl,-Map -Wl,$(BUILD_PATH)/$(BOARD).map
LDFLAGS         += -T$(LDDIR)/$(LDSCRIPT)
LDFLAGS         += -L$(LDDIR) 
LDFLAGS         += -Wl,--warn-common
# -Wl,--verbose
#let's linker optimize whole program itself
LDFLAGS         += -Wl,--relax
#LDFLAGS         += -fwhole-program
#LDFLAGS         += -Wl,--cref
##
# -fwhole-program
#--plugin=/usr/local/arm-none-eabi/lib/gcc/arm-none-eabi/5.2.1/liblto_plugin.so
# -flto -fuse-linker-plugin



TGT_BIN := 



COREINCLUDES = -I$(HARDWARE_PATH) -I$(HAL_PATH) -I$(STM32_PATH) -I$(STM32_PATH)/Libraries/STM32F4xx_StdPeriph_Driver/inc \
 -I$(STM32_PATH)/Libraries/CMSIS/Include  -I$(STM32_PATH)/Libraries/CMSIS/Device/ST/STM32F4xx/Include  \
 -I$(STM32USB_PATH) -I$(WIRISH_PATH) -I$(WIRISH_PATH)/boards/$(BOARD) \
 -I$(STM32USB_PATH)/STM32_USB_Device_Library/Class/cdc/inc -I$(STM32USB_PATH)/STM32_USB_Device_Library/Core/inc \
 -I$(BUILDROOT)/libraries/GCS_MAVLink/ -I$(BUILDROOT)/libraries/GCS_MAVLink/include/mavlink/v2.0


TGT_ASFLAGS = $(COREINCLUDES)

##
## Build rules and useful templates
##

include $(MK_DIR)/find_tools.mk

# all subfolder's rules
include $(WIRISH_PATH)/rules.mk
include $(HARDWARE_PATH)/hal/rules.mk
include $(HARDWARE_PATH)/4way/rules.mk
#include $(HARDWARE_PATH)/osd/rules.mk - per board
include $(HARDWARE_PATH)/sd/rules.mk
include $(HARDWARE_PATH)/massstorage/rules.mk
include $(HARDWARE_PATH)/STM32_USB_Driver/rules.mk
include $(HARDWARE_PATH)/STM32F4xx_DSP_StdPeriph_Lib_V1.1.0/rules.mk

include $(MK_DIR)/build_rules.mk


LIBGCC := $(shell $(CC) $(GLOBAL_FLAGS) -print-libgcc-file-name)


CFLAGS   = $(GLOBAL_CFLAGS) $(TGT_CFLAGS) $(COREINCLUDES) -I$(LIBRARIES_PATH) $(LIBRARY_INCLUDES)
CXXFLAGS = $(GLOBAL_CXXFLAGS) $(TGT_CFLAGS) $(COREINCLUDES) -I$(LIBRARIES_PATH) $(LIBRARY_INCLUDES)
ASFLAGS  = $(GLOBAL_ASFLAGS) $(TGT_ASFLAGS) 


##
## Set all submodules here
##


SKETCH_INCLUDES = $(SKETCHLIBINCLUDES) $(ARDUINOLIBINCLUDES) 

SLIB_INCLUDES  =       -I$(dir $<)/utility $(SKETCHLIBINCLUDES) $(ARDUINOLIBINCLUDES) 

################################################################################
# Built products
#


# Library object files
LIBOBJS			:=	$(SKETCHLIBOBJS)

# The ELF file
SKETCHELF		=	$(BUILDROOT)/$(SKETCH).elf
BUILDELF                =       $(notdir $(SKETCHELF))

# HEX file
SKETCHHEX		=	$(BUILDROOT)/$(SKETCH).hex

# BIN file
SKETCHBIN		=	$(BUILDROOT)/$(SKETCH).bin

# EEP file
SKETCHEEP		=	$(BUILDROOT)/$(SKETCH).eep

# Map file
SKETCHMAP		=	$(BUILDROOT)/$(SKETCH).map

# All of the objects that may be built
ALLOBJS			=	$(SKETCHOBJS) $(LIBOBJS)

# All of the dependency files that may be generated
ALLDEPS			=	$(ALLOBJS:%.o=%.d)



CAT := cat

################################################################################
# Targets
#

all: $(SKETCHELF) $(SKETCHEEP) $(BUILD_PATH)/$(BOARD).bin

print-%:
	echo "$*=$($*)"

.PHONY: upload

#upload via maple bootloader
revomini-maple-upload: $(SKETCHBIN)
	$(LIBMAPLE_PATH)/support/scripts/reset.py && sleep 1 &&  $(UPLOADER) -a1 -d $(USBID) -D $(SKETCHBIN) -R
	
#upload via BOOT0 hardware bootloader
revomini-upload: $(SKETCHBIN)
	$(UPLOADER) -a 0 --dfuse-address $(FLASH_ADDRESS)  -D $(SKETCHBIN) -R



# sorry byt I use ST_LINK and SWD, not JTAG - @NG

#revomini-debug:
#	$(AVARICE) --mkII --capture --jtag usb :4242 & \
#	gnome-terminal -x $(GDB) $(SKETCHELF) & \
#	echo -e '\n\nat the gdb prompt type "target remote localhost:4242"'

## this allows you to flash your image via JTAG for when you
## have completely broken your USB
#revomini-jtag-program:
#	$(AVARICE) --mkII --jtag usb --erase --program --file $(SKETCHELF)

revomini-debug:
	$(STUTIL) -m -p 4242

## this allows you to flash your image via SWD for when you have completely broken your USB
revomini-program:
	$(STFLASH)  --reset write $(SKETCHBIN) $(FLASH_ADDRESS)

################################################################################
# Rules
#


# fetch dependency info from a previous build if any of it exists
-include $(ALLDEPS)


revomini-clean: clean
	$(v) /bin/find $(REVO_PATH) -type f -name "*.d" -delete
	$(v) /bin/find $(REVO_PATH) -type f -name "*.o" -delete
	$(v) /bin/find $(REVO_PATH) -type f -name "*.i" -delete
	$(v) /bin/find $(REVO_PATH) -type f -name "*.ii" -delete


# Link the final object
$(SKETCHELF):	$(LIBOBJS) $(SKETCHOBJS) $(TGT_BIN) $(BUILD_PATH)/main.o

	
	$(RULEHDR)
#	$(v)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(SILENT_LD) $(CXX) $(LDFLAGS) -o $@ $(TGT_BIN) $(BUILD_PATH)/main.o $(SKETCHOBJS) $(LIBOBJS) $(LIBS) $(LIBGCC)  -Wl,-Map,$(BUILD_PATH)/$(BOARD).map
#	$(v) $(LD) $(LDFLAGS) -Map $(BUILD_PATH)/$(BOARD).map -o $(TGT_BIN) --start-group $(BUILD_PATH)/main.o $(SKETCHOBJS) $(LIBOBJS) $(LIBS) $(EXTRA_LIBS) $(LIBGCC) --end-group
#  
	$(v)cp $(SKETCHELF) .
	@echo "Firmware is in $(BUILDELF)"

# Create the eep file
$(SKETCHEEP):	$(SKETCHELF)
	$(RULEHDR)
	$(v)$(OBJCOPY) -O ihex -j.eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 $< $@

$(BUILD_PATH)/main.o: $(WIRISH_PATH)/main.cxx
	$(SILENT_CXX) $(CXX) $(CXXFLAGS) $(LIBRARY_INCLUDES) -o $@ -c $<

$(BUILD_PATH)/$(BOARD).bin: $(SKETCHELF)
	$(SILENT_OBJCOPY) $(OBJCOPY) -v -Obinary $(SKETCHELF) --gap-fill 0xFF $@ 1>/dev/null
	$(OBJCOPY) -v -Oihex $(SKETCHELF) $(BUILD_PATH)/$(BOARD).hex 1>/dev/null
	$(SILENT_DISAS) $(DISAS) -S $(SKETCHELF) > $(BUILD_PATH)/$(BOARD).disas
	@echo " "
	$(TOOLS_PATH)/dfu-convert --ihex $(BUILD_PATH)/$(BOARD).hex $(BUILD_PATH)/$(BOARD).dfu 
	$(v)cp $(BUILD_PATH)/$(BOARD).hex .
	$(v)cp $(BUILD_PATH)/$(BOARD).bin .
	$(v)cp $(BUILD_PATH)/$(BOARD).dfu .
	@echo "Object file sizes:"
	@find $(BUILD_PATH) -iname *.o | xargs $(SIZE) -t >  $(BUILD_PATH)/$(BOARD).sizes
	@cd $(REVO_PATH); find .             -iname *.o | xargs $(SIZE) -t >> $(BUILD_PATH)/$(BOARD).sizes
	@cat $(BUILD_PATH)/$(BOARD).sizes
	@echo " "
	@echo "Final Size:"
	@$(SIZE) $<
	@echo $(MEMORY_TARGET) > $(BUILD_PATH)/build-type

#
# Build sketch objects
#
