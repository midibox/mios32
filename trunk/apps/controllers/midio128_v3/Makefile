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
		  src/midio_sysex.c \
		  src/midio_patch.c \
		  src/midio_din.c \
		  src/midio_dout.c \
		  src/midio_ain.c \
		  src/midio_matrix.c \
		  src/midio_file.c \
		  src/midio_file_p.c \
		  src/mid_file.c \
		  src/seq.c \
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

# MIDI Router (and port handling)
include $(MIOS32_PATH)/modules/midi_router/midi_router.mk

# MIDImon
include $(MIOS32_PATH)/modules/midimon/midimon.mk

# UIP driver
include $(MIOS32_PATH)/modules/uip/uip.mk

# UIP Standard Task (with DHCPC + OSC server and client)
include $(MIOS32_PATH)/modules/uip_task_standard/uip_task_standard.mk

# AINSER driver
include $(MIOS32_PATH)/modules/ainser/ainser.mk

# FATFS Driver
include $(MIOS32_PATH)/modules/fatfs/fatfs.mk

# FILE Access Layer
include $(MIOS32_PATH)/modules/file/file.mk

# generic sequencer modules
include $(MIOS32_PATH)/modules/sequencer/sequencer.mk

# MIDI file Player
include $(MIOS32_PATH)/modules/midifile/midifile.mk

# USB Mass Storage Device Driver
include $(MIOS32_PATH)/modules/msd/msd.mk

# Sources for Standard Control Surface
include $(MIOS32_PATH)/modules/scs/scs.mk

# common make rules
# Please keep this include statement at the end of this Makefile. Add new modules above.
include $(MIOS32_PATH)/include/makefile/common.mk
