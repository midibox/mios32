# $Id$

PROCESSOR = 	STR91x
FAMILY    = 	STR9x

LD_FILE   = 	$(MIOS32_PATH)/etc/ld/$(FAMILY)/$(PROCESSOR).ld
PROJECT   = 	project

DRIVER_LIB =	$(MIOS32_PATH)/drivers/$(FAMILY)/v1.1.0
FREE_RTOS  =    $(MIOS32_PATH)/FreeRTOS

DEBUG     =	-g
OPTIMIZE  =	-Os

C_INCLUDE = 	-I . \
		-I $(FREE_RTOS)/Demo/Common/include \
		-I $(FREE_RTOS)/Source/include \
		-I $(FREE_RTOS)/Source/portable/STR9x \
		-I $(DRIVER_LIB)/inc

A_INCLUDE = 	

LIBS = 		

CFLAGS =	$(DEBUG) \
		$(OPTIMIZE) \
		-D GCC_STR9x \
		-D THUMB_INTERWORK


THUMB_SOURCE = \
		main.c \
		hw_config.c \
		$(FREE_RTOS)/Source/list.c \
		$(FREE_RTOS)/Source/queue.c \
		$(FREE_RTOS)/Source/tasks.c \
		$(FREE_RTOS)/Source/portable/GCC/STR9x/port.c \
		$(FREE_RTOS)/Source/portable/MemMang/heap_2.c 

THUMB_AS_SOURCE = 

ARM_SOURCE = \
		vectors.c \
		$(FREE_RTOS)/Source/portable/GCC/STR9x/portISR.c \

ARM_AS_SOURCE = \
		startup.s vector.s


include $(MIOS32_PATH)/include/makefile/common.mk
