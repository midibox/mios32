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
		../core/seq_ui.c  \
		../core/seq_ui_todo.c ../core/seq_ui_edit.c ../core/seq_ui_trkdir.c \
		../core/seq_midi.c \
		../core/seq_bpm.c \
		../core/seq_core.c \
		../core/seq_trg.c \
		../core/seq_par.c \
		../core/seq_lcd.c \
		../core/seq_led.c

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
OPTIMIZE  =	-Os

#                                    temporary select mapping for wilba's frontpanel here
CFLAGS =	$(DEBUG) $(OPTIMIZE) -DSRIO_MAPPING=1


################################################################################
# Include source modules via additional makefiles
################################################################################

# sources of programming model
include $(MIOS32_PATH)/programming_models/traditional/programming_model.mk

# application specific LCD driver (selected via makefile variable)
include $(MIOS32_PATH)/modules/app_lcd/$(LCD)/app_lcd.mk

# BLM8x8
include $(MIOS32_PATH)/modules/blm8x8/blm8x8.mk

# common make rules
include $(MIOS32_PATH)/include/makefile/common.mk
