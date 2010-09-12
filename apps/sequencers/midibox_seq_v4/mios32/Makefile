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

APP_DIR = $(MIOS32_PATH)/apps/sequencers/midibox_seq_v4

THUMB_SOURCE    = $(APP_DIR)/core/app.c \
		$(APP_DIR)/mios32/tasks.c \
		$(APP_DIR)/mios32/uip_task.c \
		$(APP_DIR)/mios32/dhcpc.c \
		$(APP_DIR)/mios32/osc_server.c \
		$(APP_DIR)/core/seq_hwcfg.c  \
		$(APP_DIR)/core/seq_ui.c  \
		$(APP_DIR)/core/seq_ui_todo.c \
		$(APP_DIR)/core/seq_ui_menu.c \
		$(APP_DIR)/core/seq_ui_edit.c \
		$(APP_DIR)/core/seq_ui_mute.c \
		$(APP_DIR)/core/seq_ui_pmute.c \
		$(APP_DIR)/core/seq_ui_pattern.c \
		$(APP_DIR)/core/seq_ui_song.c \
		$(APP_DIR)/core/seq_ui_trkevnt.c \
		$(APP_DIR)/core/seq_ui_trkmode.c \
		$(APP_DIR)/core/seq_ui_trkdir.c \
		$(APP_DIR)/core/seq_ui_trkdiv.c \
		$(APP_DIR)/core/seq_ui_trklen.c \
		$(APP_DIR)/core/seq_ui_trktran.c \
		$(APP_DIR)/core/seq_ui_trkgrv.c \
		$(APP_DIR)/core/seq_ui_trkmorph.c \
		$(APP_DIR)/core/seq_ui_trkrnd.c \
		$(APP_DIR)/core/seq_ui_trkrec.c \
		$(APP_DIR)/core/seq_ui_trgasg.c \
		$(APP_DIR)/core/seq_ui_fx.c \
		$(APP_DIR)/core/seq_ui_fx_echo.c \
		$(APP_DIR)/core/seq_ui_fx_humanize.c \
		$(APP_DIR)/core/seq_ui_fx_limit.c \
		$(APP_DIR)/core/seq_ui_fx_lfo.c \
		$(APP_DIR)/core/seq_ui_fx_scale.c \
		$(APP_DIR)/core/seq_ui_manual.c \
		$(APP_DIR)/core/seq_ui_util.c \
		$(APP_DIR)/core/seq_ui_bpm.c \
		$(APP_DIR)/core/seq_ui_bpm_presets.c \
		$(APP_DIR)/core/seq_ui_opt.c \
		$(APP_DIR)/core/seq_ui_save.c \
		$(APP_DIR)/core/seq_ui_midi.c \
		$(APP_DIR)/core/seq_ui_sysex.c \
		$(APP_DIR)/core/seq_ui_disk.c \
		$(APP_DIR)/core/seq_ui_eth.c \
		$(APP_DIR)/core/seq_ui_metronome.c \
		$(APP_DIR)/core/seq_ui_midimon.c \
		$(APP_DIR)/core/seq_ui_mixer.c \
		$(APP_DIR)/core/seq_ui_info.c \
		$(APP_DIR)/core/seq_ui_stepsel.c \
		$(APP_DIR)/core/seq_ui_trgsel.c \
		$(APP_DIR)/core/seq_ui_parsel.c \
		$(APP_DIR)/core/seq_ui_tracksel.c \
		$(APP_DIR)/core/seq_midi_port.c \
		$(APP_DIR)/core/seq_midi_in.c \
		$(APP_DIR)/core/seq_midi_osc.c \
		$(APP_DIR)/core/seq_midi_sysex.c \
		$(APP_DIR)/core/seq_midi_router.c \
		$(APP_DIR)/core/seq_midply.c \
		$(APP_DIR)/core/seq_midexp.c \
		$(APP_DIR)/core/seq_midimp.c \
		$(APP_DIR)/core/seq_blm.c \
		$(APP_DIR)/core/seq_cc.c  \
		$(APP_DIR)/core/seq_cc_labels.c  \
		$(APP_DIR)/core/seq_core.c \
		$(APP_DIR)/core/seq_scale.c \
		$(APP_DIR)/core/seq_groove.c \
		$(APP_DIR)/core/seq_morph.c \
		$(APP_DIR)/core/seq_humanize.c \
		$(APP_DIR)/core/seq_lfo.c \
		$(APP_DIR)/core/seq_chord.c \
		$(APP_DIR)/core/seq_pattern.c \
		$(APP_DIR)/core/seq_record.c \
		$(APP_DIR)/core/seq_file.c \
		$(APP_DIR)/core/seq_file_b.c \
		$(APP_DIR)/core/seq_file_m.c \
		$(APP_DIR)/core/seq_file_s.c \
		$(APP_DIR)/core/seq_file_g.c \
		$(APP_DIR)/core/seq_file_c.c \
		$(APP_DIR)/core/seq_file_gc.c \
		$(APP_DIR)/core/seq_file_t.c \
		$(APP_DIR)/core/seq_file_hw.c \
		$(APP_DIR)/core/seq_layer.c \
		$(APP_DIR)/core/seq_random.c \
		$(APP_DIR)/core/seq_trg.c \
		$(APP_DIR)/core/seq_par.c \
		$(APP_DIR)/core/seq_mixer.c \
		$(APP_DIR)/core/seq_song.c \
		$(APP_DIR)/core/seq_label.c \
		$(APP_DIR)/core/seq_lcd.c \
		$(APP_DIR)/core/seq_lcd_logo.c \
		$(APP_DIR)/core/seq_ui_fx_loop.c \
		$(APP_DIR)/core/seq_terminal.c \
		$(APP_DIR)/core/seq_statistics.c \
		$(APP_DIR)/core/seq_led.c

# (following source stubs not relevant for Cortex M3 derivatives)
THUMB_AS_SOURCE =
ARM_SOURCE      =
ARM_AS_SOURCE   =

C_INCLUDE = 	-I . \
		-I $(APP_DIR)/core

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

CFLAGS =	$(DEBUG) $(OPTIMIZE)


################################################################################
# Include source modules via additional makefiles
################################################################################

# sources of programming model
include $(MIOS32_PATH)/programming_models/traditional/programming_model.mk

# application specific LCD driver (selected via makefile variable)
include $(MIOS32_PATH)/modules/app_lcd/$(LCD)/app_lcd.mk

# BLM
include $(MIOS32_PATH)/modules/blm/blm.mk

# BLM_X
include $(MIOS32_PATH)/modules/blm_x/blm_x.mk

# AOUT Driver
include $(MIOS32_PATH)/modules/aout/aout.mk

# generic sequencer modules
include $(MIOS32_PATH)/modules/sequencer/sequencer.mk

# Notestack functions
include $(MIOS32_PATH)/modules/notestack/notestack.mk

# MIDI file Player
include $(MIOS32_PATH)/modules/midifile/midifile.mk

# FATFS Driver
include $(MIOS32_PATH)/modules/fatfs/fatfs.mk

# Portable randomize module
include $(MIOS32_PATH)/modules/random/random.mk

# USB Mass Storage Device Driver
include $(MIOS32_PATH)/modules/msd/msd.mk

# For performance measurings
include $(MIOS32_PATH)/modules/freertos_utils/freertos_utils.mk

# UIP driver
include $(MIOS32_PATH)/modules/uip/uip.mk

# common make rules
include $(MIOS32_PATH)/include/makefile/common.mk
