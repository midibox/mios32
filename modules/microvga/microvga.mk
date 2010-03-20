# $Id: sid.mk 838 2010-01-20 23:32:53Z tk $
# defines additional rules for integrating the button/LED matrix

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/microvga


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/microvga/microvga.c \
	$(MIOS32_PATH)/modules/microvga/conio.c \
	$(MIOS32_PATH)/modules/microvga/ui.c \


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/microvga
