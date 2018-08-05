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
THUMB_SOURCE    =   src/app.c \
                    src/capturer.c \
                    src/cmdeditor.c \
                    src/demoprograms.c \
                    src/filebrowser.c \
                    src/frontpanel.c \
                    src/interface.c \
                    src/genesisstate.c \
                    src/mode_system.c \
                    src/mode_voice.c \
                    src/mode_chan.c \
                    src/mode_prog.c \
                    src/mode_vgm.c \
                    src/mode_mdltr.c \
                    src/mode_sample.c \
                    src/mode_controller.c \
                    src/nameeditor.c \
                    src/syeng.c \
                    src/tracker.c

# add C++ files here
THUMB_CPP_SOURCE =  


# (following source stubs not relevant for Cortex M3 derivatives)
THUMB_AS_SOURCE =
ARM_SOURCE      =
ARM_AS_SOURCE   =

C_INCLUDE = 	-I src
A_INCLUDE = 	-I . # previously -I src

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

# VGM module driver
include $(MIOS32_PATH)/modules/vgm/vgm.mk

# BLM_X module driver
include $(MIOS32_PATH)/modules/blm_x/blm_x.mk

# Random module driver
include $(MIOS32_PATH)/modules/random/random.mk

# common make rules
# Please keep this include statement at the end of this Makefile. Add new modules above.
include $(MIOS32_PATH)/include/makefile/common.mk
