# $Id$
# defines additional rules for MIOS32 family

# select driver library
DRIVER_LIB =	$(MIOS32_PATH)/drivers/$(FAMILY)/v3.3.0

# enhance include path
C_INCLUDE +=	-I $(MIOS32_PATH)/mios32/$(FAMILY) -I $(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/inc -I $(DRIVER_LIB)/STM32_USB-FS-Device_Driver/inc -I $(DRIVER_LIB)/CMSIS/CM3/CoreSupport -I $(DRIVER_LIB)/CMSIS/CM3/DeviceSupport/ST/STM32F10x

# new for v3.3.0: now we can configure the stm32f10x.h values from command line
ifeq ($(PROCESSOR),STM32F105RC)
CFLAGS += -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=12000000 -DSTM32F10X_CL
else
CFLAGS += -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=12000000 -DSTM32F10X_HD
endif

# add modules to thumb sources
THUMB_SOURCE += \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_adc.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_dac.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_rtc.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.c \
	$(DRIVER_LIB)/STM32F10x_StdPeriph_Driver/src/misc.c \
	$(DRIVER_LIB)/CMSIS/CM3/CoreSupport/core_cm3.c \
	$(DRIVER_LIB)/STM32_USB-FS-Device_Driver/src/usb_core.c \
	$(DRIVER_LIB)/STM32_USB-FS-Device_Driver/src/usb_int.c \
	$(DRIVER_LIB)/STM32_USB-FS-Device_Driver/src/usb_mem.c \
	$(DRIVER_LIB)/STM32_USB-FS-Device_Driver/src/usb_regs.c \
	$(DRIVER_LIB)/STM32_USB-FS-Device_Driver/src/otgd_fs_cal.c \
	$(DRIVER_LIB)/STM32_USB-FS-Device_Driver/src/otgd_fs_dev.c \
	$(DRIVER_LIB)/STM32_USB-FS-Device_Driver/src/otgd_fs_int.c \
	$(DRIVER_LIB)/STM32_USB-FS-Device_Driver/src/otgd_fs_pcd.c


THUMB_AS_SOURCE += 

# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/mios32/$(FAMILY)
