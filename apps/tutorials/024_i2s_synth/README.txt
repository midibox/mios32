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
   o MBHP_CORE_STM32 or STM32 Primer
   o a I2S compatible audio DAC (like TDA1543 or PCM1725U)

Optional:
   o a 2x16 LCD
   o a DINX1 module with 4 buttons to select waveforms for L/R channel

===============================================================================


Tutorial text: TODO


I2S chip is connected to J8 of the core module
The system clock is available at J15B:E (not for STM32 Primer, as GLCD is 
connected to this IO)

Accordingly, STM32 Primer requires an I2S DAC without "master clock", like TDA1543
For MBHP_CORE_STM32, all "common" I2S DACs like PCM1725U can be used


Detailed pinning:
   - VSS -> Vss (J8:Vs)
   - VCC -> 5V (J8:Vd)
   - LRCIN (WS)   -> PB12 (J8:RC)
   - BCKIN (CK)   -> PB13 (J8:SC)
   - DIN   (SD)   -> PB15 (J8:SO)
   - SCKI  (MCLK) -> PC6 (J15B:E)


The application starts with a 440 Hz triangle wave at L/R channel

If DIN connected: Left/Right buttons allow to select the waveform (Tri/Saw/Pulse/Sine)

MIDI Note Events over Channel #1 control the frequency
MIDI CC Events over Channel #1 allow to select the waveform:
    0- 31: Triangle
   32- 63: Saw
   64- 95: Pulse
   96-127: Sine

===============================================================================
