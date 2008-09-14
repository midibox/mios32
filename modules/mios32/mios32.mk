# $Id$
# defines additional rules for MIOS32

C_INCLUDE += -I $(MIOS32_PATH)/modules/mios32

# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/mios32
