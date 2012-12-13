# $Id$

################################################################################
# following setup taken from environment variables
################################################################################

PROCESSOR =	$(MIOS32_PROCESSOR)
FAMILY    = 	$(MIOS32_FAMILY)
BOARD	  = 	$(MIOS32_BOARD)
LCD       =     $(MIOS32_LCD)


################################################################################
# Source Files, include paths and libraries
################################################################################

THUMB_SOURCE    = src/app.c \
		  src/mbng_sysex.c \
		  src/mbng_patch.c \
		  src/mbng_event.c \
		  src/mbng_din.c \
		  src/mbng_dout.c \
		  src/mbng_enc.c \
		  src/mbng_ain.c \
		  src/mbng_matrix.c \
		  src/mbng_lcd.c \
		  src/mbng_file.c \
		  src/mbng_file_c.c \
		  src/mbng_file_l.c \
                  src/terminal.c \
		  src/scs_config.c


# (following source stubs not relevant for Cortex M3 derivatives)
THUMB_AS_SOURCE =
ARM_SOURCE      =
ARM_AS_SOURCE   =

C_INCLUDE = 	-I src
A_INCLUDE = 	-I src

LIBS = 		


################################################################################
# Remaining variables
################################################################################

LD_FILE   = 	$(MIOS32_PATH)/etc/ld/$(FAMILY)/$(PROCESSOR).ld
PROJECT   = 	project

DEBUG     =	-g
OPTIMIZE  =	-Os

CFLAGS =	$(DEBUG) $(OPTIMIZE)


################################################################################
# Include source modules via additional makefiles
################################################################################

# sources of programming model
include $(MIOS32_PATH)/programming_models/traditional/programming_model.mk

# application specific LCD driver (selected via makefile variable)
include $(MIOS32_PATH)/modules/app_lcd/$(LCD)/app_lcd.mk

# buffered LCD output
include $(MIOS32_PATH)/modules/buflcd/buflcd.mk

# MIDI Router (and port handling)
include $(MIOS32_PATH)/modules/midi_router/midi_router.mk

# MIDImon
include $(MIOS32_PATH)/modules/midimon/midimon.mk

# UIP driver
include $(MIOS32_PATH)/modules/uip/uip.mk

# UIP Standard Task (with DHCPC + OSC server and client)
include $(MIOS32_PATH)/modules/uip_task_standard/uip_task_standard.mk

# For performance measurings
include $(MIOS32_PATH)/modules/freertos_utils/freertos_utils.mk

# AINSER driver
include $(MIOS32_PATH)/modules/ainser/ainser.mk

# FATFS Driver
include $(MIOS32_PATH)/modules/fatfs/fatfs.mk

# FILE Access Layer
include $(MIOS32_PATH)/modules/file/file.mk

# MD5 checksum generator
include $(MIOS32_PATH)/modules/md5/md5.mk

# generic sequencer modules
include $(MIOS32_PATH)/modules/sequencer/sequencer.mk

# USB Mass Storage Device Driver
include $(MIOS32_PATH)/modules/msd/msd.mk

# Sources for Standard Control Surface
include $(MIOS32_PATH)/modules/scs/scs.mk

# common make rules
include $(MIOS32_PATH)/include/makefile/common.mk
