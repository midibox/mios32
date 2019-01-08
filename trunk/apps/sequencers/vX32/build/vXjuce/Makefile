# $Id: Makefile 451 2009-04-19 16:37:35Z stryd_one $

################################################################################
# following setup taken from environment variables
################################################################################

PROCESSOR =		MIOSJUCE
FAMILY    = 	MIOSJUCE
BOARD	  = 	MIOSJUCE
LCD       =     MIOSJUCE


################################################################################
# Source Files, include paths and libraries
################################################################################

THUMB_SOURCE =  ../../core/src/tasks.c \
				../../core/src/app.c \
				../../core/src/utils.c \
				../../core/src/ui.c \
				../../core/src/graph.c \
				../../core/src/mclock.c \
				../../core/src/patterns.c \
				../../core/src/mod_send.c \
				../../core/src/mod_xlate.c \
				../../core/src/modules.c \
\
				../../vxmodules/src/mod_sclk.c \
				../../vxmodules/src/mod_sxh.c \
				../../vxmodules/src/mod_seq.c \
				../../vxmodules/src/mod_midiout.c


C_INCLUDE = 	-I . -I ../../core/inc/ -I ../../vxmodules/inc/



##################################################################
# MIOSJUCE Emulation Source Files, include paths and libraries
##################################################################

# CS emulation sources
THUMB_SOURCE += ../../vXjuce/src/AppComponent.cpp


# CS emulation includes
C_INCLUDE += -I ../../vXjuce/inc/


# Operating system
# Choose LINUX MACOS _WIN32 or _WIN64
OS = LINUX

# Path to JUCE tree - libJUCE should be compiled with the supplied scripts
JUCE_PATH = $(MIOS32_PATH)/drivers/MIOSJUCE/juced/juce


################################################################################
# Remaining variables
################################################################################

# LD_FILE unused in this emulation

PROJECT   = vX32

OPTIMIZE  =	-O2
DEBUG     =	-g

# Optimise/debug flags applied in MIOSJUCEcommon.mk makefile


################################################################################
# Include source modules via additional makefiles
################################################################################

# sources of MIOSJUCE model
include $(MIOS32_PATH)/programming_models/MIOSJUCE/programming_model.mk

# MIOSJUCE specific LCD driver (selected via makefile variable)
include $(MIOS32_PATH)/drivers/MIOSJUCE/modules/app_lcd/$(LCD)/app_lcd.mk

# generic sequencer modules
include $(MIOS32_PATH)/modules/sequencer/sequencer.mk

# MIOSJUCE common make rules
# Please keep this include statement at the end of this Makefile. Add new modules above.
include $(MIOS32_PATH)/include/makefile/MIOSJUCEcommon.mk
