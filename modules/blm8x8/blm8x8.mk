# $Id$
# defines additional rules for integrating the button/LED matrix

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/blm8x8


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/blm8x8/blm8x8.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/blm8x8
