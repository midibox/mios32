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

# path to complete MBSEQ V4 application
MIDIBOX_SEQ_V4_PATH = ../midibox_seq_v4
# midibox_seq_v4_lite only contains different files where a selection
# via "#ifdef MBSEQV4L" doesn't make sense

THUMB_SOURCE    = $(MIDIBOX_SEQ_V4_PATH)/core/app.c \
		$(MIDIBOX_SEQ_V4_PATH)/mios32/tasks.c \
		core/seq_hwcfg.c  \
		core/seq_ui.c  \
		core/seq_ui_pages.c \
		core/seq_ui_util.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_led.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_tpd.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_cv.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_midi_port.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_midi_in.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_midi_sysex.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_midi_router.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_midply.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_midexp.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_midimp.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_blm.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_cc.c  \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_core.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_scale.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_groove.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_morph.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_humanize.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_robotize.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_lfo.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_chord.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_pattern.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_record.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_live.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_file.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_file_b.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_file_m.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_file_s.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_file_t.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_file_g.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_file_c.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_file_gc.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_file_bm.c \
		core/seq_file_hw.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_layer.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_random.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_trg.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_par.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_mixer.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_song.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_terminal.c \
		$(MIDIBOX_SEQ_V4_PATH)/core/seq_statistics.c

# (following source stubs not relevant for Cortex M3 derivatives)
THUMB_AS_SOURCE =
ARM_SOURCE      =
ARM_AS_SOURCE   =

C_INCLUDE = 	-I mios32 \
		-I core \
		-I $(MIDIBOX_SEQ_V4_PATH)/core

A_INCLUDE = 	-I .

LIBS = 		


################################################################################
# Remaining variables
################################################################################

LD_FILE   = 	$(MIOS32_PATH)/etc/ld/$(FAMILY)/$(PROCESSOR).ld
PROJECT   = 	project

DEBUG     =	-g
OPTIMIZE  =	-Os

###
# --> HERE WE SELECT THE MBSEQV4L VARIANT
###
CFLAGS =	$(DEBUG) $(OPTIMIZE) -DMBSEQV4L


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

# BLM_X
include $(MIOS32_PATH)/modules/blm_x/blm_x.mk

# BLM_SCALAR_MASTER
include $(MIOS32_PATH)/modules/blm_scalar_master/blm_scalar_master.mk

# MIDI file Player
include $(MIOS32_PATH)/modules/midifile/midifile.mk

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
# Please keep this include statement at the end of this Makefile. Add new modules above.
include $(MIOS32_PATH)/include/makefile/common.mk


# ensure that midibox_seq_v4 directory (contains .o files for build) will be removed as well!
clean:
	rm -rf project_build
	rm -rf midibox_seq_v4
