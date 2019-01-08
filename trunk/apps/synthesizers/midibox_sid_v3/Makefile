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

THUMB_SOURCE    = mios32/tasks.c

THUMB_CPP_SOURCE = core/app.cpp \
		   core/components/MbSidClock.cpp \
		   core/components/MbSidRandomGen.cpp \
		   core/components/MbSidVoiceQueue.cpp \
		   core/components/MbSidLfo.cpp \
		   core/components/MbSidEnv.cpp \
		   core/components/MbSidEnvLead.cpp \
		   core/components/MbSidWt.cpp \
		   core/components/MbSidWtDrum.cpp \
		   core/components/MbSidMod.cpp \
		   core/components/MbSidArp.cpp \
		   core/components/MbSidSeq.cpp \
		   core/components/MbSidSeqBassline.cpp \
		   core/components/MbSidSeqDrum.cpp \
		   core/components/MbSidFilter.cpp \
		   core/components/MbSidMidiVoice.cpp \
		   core/components/MbSidVoice.cpp \
		   core/components/MbSidVoiceDrum.cpp \
		   core/components/MbSidDrum.cpp \
		   core/MbSidEnvironment.cpp \
		   core/MbSidSysEx.cpp \
		   core/MbSidAsid.cpp \
		   core/MbSidTables.cpp \
		   core/MbSid.cpp \
		   core/MbSidPatch.cpp \
		   core/MbSidSe.cpp \
		   core/MbSidSeBassline.cpp \
		   core/MbSidSeDrum.cpp \
		   core/MbSidSeMulti.cpp \
		   core/MbSidSeLead.cpp \


# (following source stubs not relevant for Cortex M3 derivatives)
THUMB_AS_SOURCE =
ARM_SOURCE      =
ARM_AS_SOURCE   =

C_INCLUDE = 	-I mios32 \
		-I core \
		-I core/components

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
#include $(MIOS32_PATH)/modules/msd/msd.mk

# MBNet module
include $(MIOS32_PATH)/modules/mbnet/mbnet.mk

# For performance measurings
include $(MIOS32_PATH)/modules/freertos_utils/freertos_utils.mk

# common make rules
# Please keep this include statement at the end of this Makefile. Add new modules above.
include $(MIOS32_PATH)/include/makefile/common.mk
