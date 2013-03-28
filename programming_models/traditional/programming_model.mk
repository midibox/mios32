# $Id$
# defines rules building the programming model

# philetaylor - changed to use umm_malloc and added MemMang to the include dirs.

# where is FreeRTOS located

FREE_RTOS      =    $(MIOS32_PATH)/FreeRTOS

# extend include path
C_INCLUDE += 	-I $(MIOS32_PATH)/programming_models/traditional \
		-I $(FREE_RTOS)/Source/include \
		-I $(FREE_RTOS)/Source/portable/GCC/ARM_CM3 \
		-I $(FREE_RTOS)/Source/portable/MemMang \

# required by FreeRTOS to select the port
CFLAGS    +=    -DGCC_ARMCM3

# add modules to thumb sources
THUMB_SOURCE += \
		$(MIOS32_PATH)/programming_models/traditional/main.c \
		$(FREE_RTOS)/Source/tasks.c \
		$(FREE_RTOS)/Source/list.c \
		$(FREE_RTOS)/Source/queue.c \
		$(FREE_RTOS)/Source/timers.c \
		$(FREE_RTOS)/Source/portable/GCC/ARM_CM3/port.c \
		$(FREE_RTOS)/Source/portable/MemMang/umm_malloc.c 

ifeq ($(FAMILY),STM32F10x)
THUMB_SOURCE += $(MIOS32_PATH)/programming_models/traditional/startup_stm32f10x_hd.c
endif
ifeq ($(FAMILY),LPC17xx)
THUMB_SOURCE += $(MIOS32_PATH)/programming_models/traditional/startup_LPC17xx.c
endif

THUMB_CPP_SOURCE += $(MIOS32_PATH)/programming_models/traditional/mini_cpp.cpp \
		    $(MIOS32_PATH)/programming_models/traditional/freertos_heap.cpp

# add MIOS32 sources
include $(MIOS32_PATH)/mios32/mios32.mk

# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/programming_models/traditional
