# $Id$
# defines additional rules for integrating the Mass Storage Device Driver

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/msd


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/msd/msd.c \
	$(MIOS32_PATH)/modules/msd/msd_desc.c \
	$(MIOS32_PATH)/modules/msd/msd_bot.c \
	$(MIOS32_PATH)/modules/msd/msd_scsi.c \
	$(MIOS32_PATH)/modules/msd/msd_scsi_data.c \
	$(MIOS32_PATH)/modules/msd/msd_memory.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/msd
