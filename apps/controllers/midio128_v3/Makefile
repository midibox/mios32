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

THUMB_SOURCE    = src/app.c \
		  src/midio_sysex.c \
		  src/midio_patch.c \
		  src/midio_din.c \
		  src/midio_dout.c \
		  src/midio_file.c \
		  src/midio_file_p.c \
                  src/midimon.c \
                  src/terminal.c \
                  src/uip_task.c \
                  src/osc_server.c \
                  src/osc_client.c \
                  src/dhcpc.c \
                  src/telnetd.c \
                  src/shell.c \
		  src/scs_config.c


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

# UIP driver
include $(MIOS32_PATH)/modules/uip/uip.mk

# FATFS Driver
include $(MIOS32_PATH)/modules/fatfs/fatfs.mk

# Sources for Standard Control Surface
include $(MIOS32_PATH)/modules/scs/scs.mk

# common make rules
include $(MIOS32_PATH)/include/makefile/common.mk
