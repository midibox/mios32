# $Id$
# defines additional rules for integrating the midi_router modules

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/midi_router


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/midi_router/midi_router.c \
	$(MIOS32_PATH)/modules/midi_router/midi_port.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/midi_router
