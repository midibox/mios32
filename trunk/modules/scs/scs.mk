# $Id$
# defines additional rules for integrating the module

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/scs


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/scs/scs.c \
	$(MIOS32_PATH)/modules/scs/scs_lcd.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/scs
