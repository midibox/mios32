# $Id$
# defines additional rules for integrating the button/LED matrix

C_INCLUDE += -I $(MIOS32_PATH)/modules/blm

# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/blm
