# $Id$
# defines additional rules for MIOS32 family

# select driver library
DRIVER_LIB =	$(MIOS32_PATH)/drivers/$(FAMILY)/v2.0.1

# enhance include path
C_INCLUDE +=	-I $(DRIVER_LIB)/inc

# add modules to thumb sources
THUMB_SOURCE += \
	$(DRIVER_LIB)/src/stm32f10x_gpio.c \
	$(DRIVER_LIB)/src/stm32f10x_flash.c \
	$(DRIVER_LIB)/src/stm32f10x_adc.c \
	$(DRIVER_LIB)/src/stm32f10x_spi.c \
	$(DRIVER_LIB)/src/stm32f10x_usart.c \
	$(DRIVER_LIB)/src/stm32f10x_i2c.c \
	$(DRIVER_LIB)/src/stm32f10x_dma.c \
	$(DRIVER_LIB)/src/stm32f10x_tim.c \
	$(DRIVER_LIB)/src/stm32f10x_rcc.c \
	$(DRIVER_LIB)/src/stm32f10x_systick.c \
	$(DRIVER_LIB)/src/stm32f10x_nvic.c \
	$(DRIVER_LIB)/src/usb_core.c \
	$(DRIVER_LIB)/src/usb_init.c \
	$(DRIVER_LIB)/src/usb_int.c \
	$(DRIVER_LIB)/src/usb_mem.c \
	$(DRIVER_LIB)/src/usb_regs.c

THUMB_AS_SOURCE += $(DRIVER_LIB)/src/cortexm3_macro.s

# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/mios32/$(FAMILY)
