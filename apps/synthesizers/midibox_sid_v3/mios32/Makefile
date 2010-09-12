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

APP_DIR = $(MIOS32_PATH)/apps/synthesizers/midibox_sid_v3

THUMB_SOURCE    = $(APP_DIR)/mios32/tasks.c

THUMB_CPP_SOURCE = $(APP_DIR)/core/app.cpp \
		   $(APP_DIR)/core/components/MbSidClock.cpp \
		   $(APP_DIR)/core/components/MbSidRandomGen.cpp \
		   $(APP_DIR)/core/components/MbSidVoiceQueue.cpp \
		   $(APP_DIR)/core/components/MbSidLfo.cpp \
		   $(APP_DIR)/core/components/MbSidEnv.cpp \
		   $(APP_DIR)/core/components/MbSidEnvLead.cpp \
		   $(APP_DIR)/core/components/MbSidWt.cpp \
		   $(APP_DIR)/core/components/MbSidWtDrum.cpp \
		   $(APP_DIR)/core/components/MbSidMod.cpp \
		   $(APP_DIR)/core/components/MbSidArp.cpp \
		   $(APP_DIR)/core/components/MbSidSeq.cpp \
		   $(APP_DIR)/core/components/MbSidSeqBassline.cpp \
		   $(APP_DIR)/core/components/MbSidSeqDrum.cpp \
		   $(APP_DIR)/core/components/MbSidFilter.cpp \
		   $(APP_DIR)/core/components/MbSidMidiVoice.cpp \
		   $(APP_DIR)/core/components/MbSidVoice.cpp \
		   $(APP_DIR)/core/components/MbSidVoiceDrum.cpp \
		   $(APP_DIR)/core/components/MbSidDrum.cpp \
		   $(APP_DIR)/core/MbSidEnvironment.cpp \
		   $(APP_DIR)/core/MbSidSysEx.cpp \
		   $(APP_DIR)/core/MbSidAsid.cpp \
		   $(APP_DIR)/core/MbSidTables.cpp \
		   $(APP_DIR)/core/MbSid.cpp \
		   $(APP_DIR)/core/MbSidPatch.cpp \
		   $(APP_DIR)/core/MbSidSe.cpp \
		   $(APP_DIR)/core/MbSidSeBassline.cpp \
		   $(APP_DIR)/core/MbSidSeDrum.cpp \
		   $(APP_DIR)/core/MbSidSeMulti.cpp \
		   $(APP_DIR)/core/MbSidSeLead.cpp \


# (following source stubs not relevant for Cortex M3 derivatives)
THUMB_AS_SOURCE =
ARM_SOURCE      =
ARM_AS_SOURCE   =

C_INCLUDE = 	-I . \
		-I $(APP_DIR)/core \
		-I $(APP_DIR)/core/components

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
