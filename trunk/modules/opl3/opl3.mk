# $Id: opl3.mk $
# defines additional rules for integrating the OPL3 board
# Modified from sid module code by Sauraen

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/opl3


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/opl3/opl3.c \


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/opl3
