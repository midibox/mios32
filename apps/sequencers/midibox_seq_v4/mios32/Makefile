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
		../core/seq_ui_todo.c \
		../core/seq_ui_menu.c \
		../core/seq_ui_edit.c \
		../core/seq_ui_mute.c \
		../core/seq_ui_pattern.c \
		../core/seq_ui_song.c \
		../core/seq_ui_trkevnt.c \
		../core/seq_ui_trkmode.c \
		../core/seq_ui_trkdir.c \
		../core/seq_ui_trkdiv.c \
		../core/seq_ui_trklen.c \
		../core/seq_ui_trktran.c \
		../core/seq_ui_trkgrv.c \
		../core/seq_ui_trkrnd.c \
		../core/seq_ui_trkrec.c \
		../core/seq_ui_trgasg.c \
		../core/seq_ui_fx.c \
		../core/seq_ui_fx_echo.c \
		../core/seq_ui_util.c \
		../core/seq_ui_bpm.c \
		../core/seq_ui_opt.c \
		../core/seq_ui_save.c \
		../core/seq_ui_midi.c \
		../core/seq_ui_mixer.c \
		../core/seq_ui_stepsel.c \
		../core/seq_ui_trgsel.c \
		../core/seq_ui_parsel.c \
		../core/seq_midi_port.c \
		../core/seq_midi_in.c \
		../core/seq_midi_router.c \
		../core/seq_midply.c \
		../core/seq_cc.c  \
		../core/seq_core.c \
		../core/seq_scale.c \
		../core/seq_groove.c \
		../core/seq_humanize.c \
		../core/seq_chord.c \
		../core/seq_pattern.c \
		../core/seq_record.c \
		../core/seq_file.c \
		../core/seq_file_b.c \
		../core/seq_file_m.c \
		../core/seq_file_s.c \
		../core/seq_file_c.c \
		../core/seq_layer.c \
		../core/seq_random.c \
		../core/seq_trg.c \
		../core/seq_par.c \
		../core/seq_mixer.c \
		../core/seq_song.c \
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
#DEBUG     =	-g -Wall -Wno-format -Wno-switch
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

# BLM_X
include $(MIOS32_PATH)/modules/blm_x/blm_x.mk

# generic sequencer modules
include $(MIOS32_PATH)/modules/sequencer/sequencer.mk

# MIDI file Player
include $(MIOS32_PATH)/modules/midifile/midifile.mk

# DOSFS Driver
include $(MIOS32_PATH)/modules/dosfs/dosfs.mk

# Portable randomize module
include $(MIOS32_PATH)/modules/random/random.mk

# common make rules
include $(MIOS32_PATH)/include/makefile/common.mk
