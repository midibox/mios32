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

# (following source stubs not relevant for Cortex M3 derivatives)
THUMB_AS_SOURCE =
ARM_SOURCE      =
ARM_AS_SOURCE   =

C_INCLUDE = 	-I . -I ../../core/inc/ -I ../../vxmodules/inc/

A_INCLUDE = 	-I .

LIBS = 	


################################################################################
# Remaining variables
################################################################################

LD_FILE   = 	$(MIOS32_PATH)/etc/ld/$(FAMILY)/$(PROCESSOR).ld
PROJECT   = 	vX32

OPTIMIZE  =	-Os
DEBUG     =	-g

CFLAGS =	$(DEBUG) $(OPTIMIZE)
# CFLAGS =	 $(OPTIMIZE)


################################################################################
# Include source modules via additional makefiles
################################################################################

# sources of programming model
include $(MIOS32_PATH)/programming_models/traditional/programming_model.mk

# application specific LCD driver (selected via makefile variable)
include $(MIOS32_PATH)/modules/app_lcd/$(LCD)/app_lcd.mk

# generic sequencer modules
include $(MIOS32_PATH)/modules/sequencer/sequencer.mk

# common make rules
# Please keep this include statement at the end of this Makefile. Add new modules above.
include $(MIOS32_PATH)/include/makefile/common.mk
