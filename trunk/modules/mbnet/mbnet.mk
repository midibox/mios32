# $Id$
# defines additional rules for integrating the button/LED matrix

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/mbnet -I $(MIOS32_PATH)/modules/mbnet/$(MIOS32_FAMILY)


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/mbnet/mbnet.c \
	$(MIOS32_PATH)/modules/mbnet/$(MIOS32_FAMILY)/mbnet_hal.c

ifeq ($(FAMILY),STM32F10x)
THUMB_SOURCE += $(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_can.c
endif


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/mbnet
