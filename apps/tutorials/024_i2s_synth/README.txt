$Id$

MIOS32 Tutorial #024: I2S Synthesizer
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17
   o a I2S compatible audio DAC (like TDA1543 or PCM1725U)

Optional:
   o a 2x16 LCD
   o a DINX1 module with 4 buttons to select waveforms for L/R channel

===============================================================================


Tutorial text: TODO


I2S DAC Schematics: see http://www.ucapps.de/mbhp_i2s.html

If a STM32 is used instead of LPC17, connect the I2S chip to J8 of the core module.
The system clock is available at J15B:E

If a LPC17 is used, connect the I2S DAC to J28 of the MBHP_CORE_LPC17 module


The application starts with a 440 Hz triangle wave at L/R channel

If DIN connected: Left/Right buttons allow to select the waveform (Tri/Saw/Pulse/Sine)

MIDI Note Events over Channel #1 control the frequency
MIDI CC Events over Channel #1 allow to select the waveform:
    0- 31: Triangle
   32- 63: Saw
   64- 95: Pulse
   96-127: Sine

===============================================================================
