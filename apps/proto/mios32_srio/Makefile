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
		-I $(FREE_RTOS)/Source/portable/GCC/ARM_CM3 \
		-I $(DRIVER_LIB)/inc

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
		$(DRIVER_LIB)/src/stm32f10x_gpio.c \
		$(DRIVER_LIB)/src/stm32f10x_spi.c \
		$(DRIVER_LIB)/src/stm32f10x_rcc.c \
		$(DRIVER_LIB)/src/stm32f10x_systick.c \
		$(DRIVER_LIB)/src/stm32f10x_nvic.c \
		crt0_STM32x.c \
		main.c \
		$(FREE_RTOS)/Source/tasks.c \
		$(FREE_RTOS)/Source/list.c \
		$(FREE_RTOS)/Source/queue.c \
		$(FREE_RTOS)/Source/portable/GCC/ARM_CM3/port.c \
		$(FREE_RTOS)/Source/portable/MemMang/heap_2.c 

THUMB_AS_SOURCE =

ARM_SOURCE =

ARM_AS_SOURCE =


# add MIOS32 sources
include $(MIOS32_PATH)/modules/mios32/mios32.mk

# include common make rules
include $(MIOS32_PATH)/include/makefile/common.mk
