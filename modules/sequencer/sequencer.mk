# $Id$
# defines additional rules for integrating the sequencer modules

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/sequencer


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/sequencer/seq_bpm.c \
	$(MIOS32_PATH)/modules/sequencer/seq_midi_out.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/sequencer
