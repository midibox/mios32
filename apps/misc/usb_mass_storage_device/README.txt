$Id$

USB Mass Storage Device
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o STM32 or MBHP_CORE_STM32
   o a SD Card connected to J16 of the core module

Optional hardware:
   o a LCD to display device status

===============================================================================

This application allows to access a connected SD Card via USB by emulating
a mass storage device.

The USB driver will provide this device once a SD Card is connected.

Other USB drivers which were running before (e.g. MIOS32_USB_MIDI 
or MIOS32_USB_COM) will be bypassed to avoid driver conflicts at the
OS side. An enumeration will be triggered as if the USB cable has
been re-connected.

The (optional) LCD displays the connection status.
Once a SD Card has been connected, the right upper corner shows four characters:
  o U: USB device ready (e.g. cable connected)
  o M: SD Card is mounted by OS, resp. is ready for mounting if U not displayed
  o R: Read access to SD Card
  o W: Write access to SD Card


By removing the SD Card the previously active USB device (e.g. MIOS32_USB_MIDI)
will be available again.


This means also: if a new application should be uploaded via USB MIDI, the 
SD Card has to be removed so that the MIDI device is available again.


Please note: the USB access is very slow compared to the speed you know from
common SD Card readers or USB sticks, because the device is accessed with
a single parallel line and not with a parallel interface. 

However, transfers are fast enough to upload/download data for MIOS32 
applications w/o removing the SD Card from the MBHP_CORE_STM32 module.

===============================================================================
