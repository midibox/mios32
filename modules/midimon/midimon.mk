# $Id$
# defines additional rules for integrating the midimon modules

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/midimon


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/midimon/midimon.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/midimon
