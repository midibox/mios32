# $Id$
# defines additional rules for MIOS32

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/mios32


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/mios32/mios32_sys.c \
	$(MIOS32_PATH)/modules/mios32/mios32_srio.c \
	$(MIOS32_PATH)/modules/mios32/mios32_din.c \
	$(MIOS32_PATH)/modules/mios32/mios32_dout.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/mios32
