# $Id$
# defines additional rules for integrating the Mass Storage Device Driver


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
ifeq ($(FAMILY),STM32F10x)
C_INCLUDE += -I $(MIOS32_PATH)/modules/msd/STM32F10x
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/msd/STM32F10x/msd.c \
	$(MIOS32_PATH)/modules/msd/STM32F10x/msd_desc.c \
	$(MIOS32_PATH)/modules/msd/STM32F10x/msd_bot.c \
	$(MIOS32_PATH)/modules/msd/STM32F10x/msd_scsi.c \
	$(MIOS32_PATH)/modules/msd/STM32F10x/msd_scsi_data.c \
	$(MIOS32_PATH)/modules/msd/STM32F10x/msd_memory.c
endif

ifeq ($(FAMILY),LPC17xx)
C_INCLUDE += -I $(MIOS32_PATH)/modules/msd/LPC17xx
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/msd/LPC17xx/msd.c \
	$(MIOS32_PATH)/modules/msd/LPC17xx/msc_bot.c \
	$(MIOS32_PATH)/modules/msd/LPC17xx/msc_scsi.c \
	$(MIOS32_PATH)/modules/msd/LPC17xx/blockdev_sd.c
endif

# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/msd
