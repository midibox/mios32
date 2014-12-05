$Id$

SPI MIDI Test
===============================================================================
Copyright (C) 2014 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32, MBHP_CORE_LPC17 or MBHP_CORE_STM32F4
   o KissBox OEM connected to J16 (Chip select to RC2)

===============================================================================

This feature requires an update to MIOS32 bootloader v1.018
In the bootloader update app, enter "set spi_midi 1" to enable the SPI MIDI device at J16 (RC2 chip select line).

===============================================================================
