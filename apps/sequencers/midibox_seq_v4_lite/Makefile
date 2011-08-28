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

THUMB_SOURCE    = core/app.c \
		mios32/tasks.c \
		core/seq_hwcfg.c  \
		core/seq_ui.c  \
		core/seq_ui_pages.c \
		core/seq_ui_util.c \
		core/seq_cv.c \
		core/seq_midi_port.c \
		core/seq_midi_in.c \
		core/seq_midi_sysex.c \
		core/seq_midi_router.c \
		core/seq_cc.c  \
		core/seq_core.c \
		core/seq_scale.c \
		core/seq_groove.c \
		core/seq_morph.c \
		core/seq_humanize.c \
		core/seq_lfo.c \
		core/seq_chord.c \
		core/seq_pattern.c \
		core/seq_record.c \
		core/seq_live.c \
		core/seq_file.c \
		core/seq_file_b.c \
		core/seq_file_t.c \
		core/seq_file_g.c \
		core/seq_file_gc.c \
		core/seq_file_hw.c \
		core/seq_layer.c \
		core/seq_random.c \
		core/seq_trg.c \
		core/seq_par.c \
		core/seq_terminal.c \
		core/seq_statistics.c

# (following source stubs not relevant for Cortex M3 derivatives)
THUMB_AS_SOURCE =
ARM_SOURCE      =
ARM_AS_SOURCE   =

C_INCLUDE = 	-I mios32 \
		-I core

A_INCLUDE = 	-I .

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

# AOUT Driver
include $(MIOS32_PATH)/modules/aout/aout.mk

# generic sequencer modules
include $(MIOS32_PATH)/modules/sequencer/sequencer.mk

# Notestack functions
include $(MIOS32_PATH)/modules/notestack/notestack.mk

# BLM_CHEAPO driver
include $(MIOS32_PATH)/modules/blm_cheapo/blm_cheapo.mk

# FATFS Driver
include $(MIOS32_PATH)/modules/fatfs/fatfs.mk

# FILE Access Layer
include $(MIOS32_PATH)/modules/file/file.mk

# Portable randomize module
include $(MIOS32_PATH)/modules/random/random.mk

# USB Mass Storage Device Driver
include $(MIOS32_PATH)/modules/msd/msd.mk

# For performance measurings
include $(MIOS32_PATH)/modules/freertos_utils/freertos_utils.mk

# UIP driver
include $(MIOS32_PATH)/modules/uip/uip.mk

# UIP Standard Task (with OSC Client/Server)
include $(MIOS32_PATH)/modules/uip_task_standard/uip_task_standard.mk

# common make rules
include $(MIOS32_PATH)/include/makefile/common.mk
