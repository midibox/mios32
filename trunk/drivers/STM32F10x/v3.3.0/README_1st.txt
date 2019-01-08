$Id$

Following files have been touched to integrate the library into the MIOS32 setup:

- STM32_USB-FS-Device_Driver/src/usb_int.c
  made "wIstr" local. This was a global variable before which could lead to 
  unintended overwrites if CTR_LP() was called from a different task

- STM32_USB-FS-Device_Driver/src/usb_int.c
  same for EPindex, since we have a conflict if CTR_LP() and CTR_HP() are 
  called with different priorities

- STM32_USB-FS-Device_Driver/src/usb_core.c and STM32_USB-FS-Device_Driver/inc/otgd_fs_dev.h
  replaced Device_Property.* by pProperty->*
  (programming error at ST side)
