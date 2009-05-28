# $Id$
# defines additional rules for MIOS32 family

# select driver library
DRIVER_LIB =	$(MIOS32_PATH)/drivers/$(FAMILY)/v3.0.0

# enhance include path
C_INCLUDE +=	-I $(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/inc -I $(DRIVER_LIB)/STM32_USB-FS-Device_Driver/inc -I $(DRIVER_LIB)/CMSIS/Core/CM3

# add modules to thumb sources
THUMB_SOURCE += \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_adc.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/misc.c \
	$(DRIVER_LIB)/CMSIS/Core/CM3/core_cm3.c \
	$(DRIVER_LIB)/CMSIS/Core/CM3/system_stm32f10x.c \
	$(DRIVER_LIB)/STM32_USB-FS-Device_Driver/src/usb_core.c \
	$(DRIVER_LIB)/STM32_USB-FS-Device_Driver/src/usb_int.c \
	$(DRIVER_LIB)/STM32_USB-FS-Device_Driver/src/usb_mem.c \
	$(DRIVER_LIB)/STM32_USB-FS-Device_Driver/src/usb_regs.c

THUMB_AS_SOURCE += 

# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/mios32/$(FAMILY)
