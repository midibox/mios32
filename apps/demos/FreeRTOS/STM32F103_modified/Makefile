# $Id$

PROCESSOR = 	STM32F103
FAMILY    = 	STM32F10x

LD_FILE   = 	$(MIOS32_PATH)/etc/ld/$(FAMILY)/$(PROCESSOR).ld
PROJECT   = 	project

DRIVER_LIB =	$(MIOS32_PATH)/drivers/$(FAMILY)/v0.3.0
FREE_RTOS  =    $(MIOS32_PATH)/FreeRTOS

DEBUG     =	-g
OPTIMIZE  =	-Os

C_INCLUDE = 	-I . \
		-I ./ST_Code \
		-I $(FREE_RTOS)/Demo/Common/include \
		-I $(FREE_RTOS)/Source/include \
		-I $(FREE_RTOS)/Source/portable/ARM_CM3 \
		-I $(DRIVER_LIB)/inc

A_INCLUDE = 	

LIBS = 		

CFLAGS =	$(DEBUG) \
		$(OPTIMIZE) \
		-MD \
		-D_STM32F103RBT6_ \
		-D_STM3x_ \
		-D_STM32x_ \
		-DGCC_ARMCM3

THUMB_SOURCE = \
		$(DRIVER_LIB)/src/stm32f10x_gpio.c \
		$(DRIVER_LIB)/src/stm32f10x_tim.c \
		$(DRIVER_LIB)/src/stm32f10x_spi.c \
		$(DRIVER_LIB)/src/stm32f10x_rcc.c \
		$(DRIVER_LIB)/src/stm32f10x_systick.c \
		$(DRIVER_LIB)/src/stm32f10x_nvic.c \
		ST_Code/led.c \
		ST_Code/crt0_STM32x.c \
		ST_Code/lcd.c \
		ST_Code/Util.c \
		ST_Code/draw.c \
		ST_Code/mems.c \
		ST_Code/buzzer.c \
		ST_Code/pointer.c \
		main.c \
		ParTest/ParTest.c \
		timertest.c \
		printf-stdarg.c \
		$(FREE_RTOS)/Demo/Common/Minimal/GenQTest.c \
		$(FREE_RTOS)/Demo/Common/Minimal/BlockQ.c \
		$(FREE_RTOS)/Demo/Common/Minimal/blocktim.c \
		$(FREE_RTOS)/Demo/Common/Minimal/QPeek.c \
		$(FREE_RTOS)/Demo/Common/Minimal/PollQ.c \
		$(FREE_RTOS)/Source/tasks.c \
		$(FREE_RTOS)/Source/list.c \
		$(FREE_RTOS)/Source/queue.c \
		$(FREE_RTOS)/Source/portable/GCC/ARM_CM3/port.c \
		$(FREE_RTOS)/Source/portable/MemMang/heap_2.c 

THUMB_AS_SOURCE = \
		$(DRIVER_LIB)/src/cortexm3_macro.s

ARM_SOURCE =

ARM_AS_SOURCE =


include $(MIOS32_PATH)/include/makefile/common.mk
