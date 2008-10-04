# $Id$

PROCESSOR = 	STM32F103
FAMILY    = 	STM32F10x

LD_FILE   = 	$(MIOS32_PATH)/etc/ld/$(FAMILY)/$(PROCESSOR).ld
PROJECT   = 	project

DRIVER_LIB =	$(MIOS32_PATH)/drivers/$(FAMILY)/v2.0.1
FREE_RTOS  =    $(MIOS32_PATH)/FreeRTOS

DEBUG     =	-g
OPTIMIZE  =	-Os

C_INCLUDE = 	-I . \
		-I $(FREE_RTOS)/Source/include \
		-I $(FREE_RTOS)/Source/portable/GCC/ARM_CM3

A_INCLUDE = 	

LIBS = 		

CFLAGS =	$(DEBUG) \
		$(OPTIMIZE) \
		-MD \
		-D_STM32F103RBT6_ \
		-D_STM3x_ \
		-D_STM32x_ \
		-D_STM32_PRIMER_ \
		-DGCC_ARMCM3


THUMB_SOURCE = \
		app.c

THUMB_AS_SOURCE =

ARM_SOURCE =

ARM_AS_SOURCE =


# add sources of programming model
include $(MIOS32_PATH)/programming_models/traditional/programming_model.mk

# add application specific LCD driver
include $(MIOS32_PATH)/modules/app_lcd/st7637/app_lcd.mk
#include $(MIOS32_PATH)/modules/app_lcd/clcd/app_lcd.mk

# include common make rules
include $(MIOS32_PATH)/include/makefile/common.mk
