$Id$

EmbSpeech Demo for MIOS32
===============================================================================
Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17
   o a I2S compatible audio DAC (like TDA1543 or PCM1725U)

===============================================================================

Documentation: TODO


I2S DAC Schematics: see http://www.ucapps.de/mbhp_i2s.html

If a STM32 is used instead of LPC17, connect the I2S chip to J8 of the core module.
The system clock is available at J15B:E

If a LPC17 is used, connect the I2S DAC to J28 of the MBHP_CORE_LPC17 module

===============================================================================
