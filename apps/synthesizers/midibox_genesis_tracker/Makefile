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

# add plain C files here
THUMB_SOURCE    =   src/vgmplayer_ll.c \
                    src/frontpanel.c

# add C++ files here
THUMB_CPP_SOURCE =  src/app.cpp \
                    src/vgmhead.cpp \
                    src/vgmsourcestream.cpp \
                    src/vgmplayer.cpp \


# (following source stubs not relevant for Cortex M3 derivatives)
THUMB_AS_SOURCE =
ARM_SOURCE      =
ARM_AS_SOURCE   =

C_INCLUDE = 	-I src
A_INCLUDE = 	-I src

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

# FAT filesystem module driver
include $(MIOS32_PATH)/modules/fatfs/fatfs.mk

# FILE module driver
include $(MIOS32_PATH)/modules/file/file.mk

# MBHP_Genesis module driver
include $(MIOS32_PATH)/modules/genesis/genesis.mk

# BLM_X module driver
include $(MIOS32_PATH)/modules/blm_x/blm_x.mk

# common make rules
# Please keep this include statement at the end of this Makefile. Add new modules above.
include $(MIOS32_PATH)/include/makefile/common.mk
