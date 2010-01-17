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

THUMB_SOURCE    = ../core/app.c \
		../mios32/tasks.c \
		../core/sid_midi.c \
		../core/sid_midi_l.c \
		../core/sid_midi_b.c \
		../core/sid_midi_d.c \
		../core/sid_midi_m.c \
		../core/sid_sysex.c \
		../core/sid_asid.c \
		../core/sid_bank.c \
		../core/sid_patch.c \
		../core/sid_knob.c \
		../core/sid_par.c \
		../core/sid_random.c \
		../core/sid_dmodel.c \
		../core/sid_voice.c \
		../core/sid_se.c \
		../core/sid_se_l.c \
		../core/sid_se_b.c \
		../core/sid_se_d.c \
		../core/sid_se_m.c

# (following source stubs not relevant for Cortex M3 derivatives)
THUMB_AS_SOURCE =
ARM_SOURCE      =
ARM_AS_SOURCE   =

C_INCLUDE = 	-I . \
		-I ../core

A_INCLUDE = 	-I .

LIBS = 		


################################################################################
# Remaining variables
################################################################################

LD_FILE   = 	$(MIOS32_PATH)/etc/ld/$(FAMILY)/$(PROCESSOR).ld
PROJECT   = 	project

DEBUG     =	-g
#DEBUG     =	-g -Wall -Wno-format -Wno-switch
OPTIMIZE  =	-O3

CFLAGS =	$(DEBUG) $(OPTIMIZE)


################################################################################
# Include source modules via additional makefiles
################################################################################

# sources of programming model
include $(MIOS32_PATH)/programming_models/traditional/programming_model.mk

# application specific LCD driver (selected via makefile variable)
include $(MIOS32_PATH)/modules/app_lcd/$(LCD)/app_lcd.mk

# AOUT Driver
include $(MIOS32_PATH)/modules/aout/aout.mk

# SID module driver
include $(MIOS32_PATH)/modules/sid/sid.mk

# Notestack functions
include $(MIOS32_PATH)/modules/notestack/notestack.mk

# MIDI file Player
include $(MIOS32_PATH)/modules/midifile/midifile.mk

# DOSFS Driver
include $(MIOS32_PATH)/modules/dosfs/dosfs.mk

# Portable randomize module
include $(MIOS32_PATH)/modules/random/random.mk

# USB Mass Storage Device Driver
include $(MIOS32_PATH)/modules/msd/msd.mk

# For performance measurings
include $(MIOS32_PATH)/modules/freertos_utils/freertos_utils.mk

# common make rules
include $(MIOS32_PATH)/include/makefile/common.mk
