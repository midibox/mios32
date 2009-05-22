$Id$

Following files have been touched to integrate the library into the MIOS32 setup:

- STM32_USB-FS-Device_Driver/inc/usb_type.h
  o removed #include "usb_conf.h"

- CMSIS/Core/CM3/stm32f10x.h
  o enabled USE_STDPERIPH_DRIVER
  o adapted HSE_Value to 12 MHz (could be replaced by a MIOS32 define later)

- CMSIS/Core/CM3/startup/gcc/startup_stm32f10x_hd.c
  o copied to $MIOS32_PATH/programming_model/traditional
  o added adaptions for FreeRTOS, removed unneeded code

