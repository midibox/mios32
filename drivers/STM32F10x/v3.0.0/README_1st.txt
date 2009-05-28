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


Additional changes:
- STM32_USB-FS-Device_Driver/src/usb_int.c
  made "wIstr" local. This was a global variable before which could lead to 
  unintended overwrites if CTR_LP() was called from a different task

- STM32_USB-FS-Device_Driver/src/usb_int.c
  same for EPindex, since we have a conflict if CTR_LP() and CTR_HP() are 
  called with different priorities

- STM32_USB-FS-Device_Driver/src/usb_core.c
  replaced Device_Property.MaxPacketSize by pProperty->MaxPacketSize
  (programming error at ST side)
