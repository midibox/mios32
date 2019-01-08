# $Id: dpot.mk $

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/dpot

# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
                $(MIOS32_PATH)/modules/dpot/dpot.c

# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/dpot
