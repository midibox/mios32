$Id$

Goom Synth adaption to MIOS32
===============================================================================
Code taken from http://www.quinapalus.com/goom.html and integrated into
a common MIOS32 framework:

-> app.c: the MIOS32 hooks
-> synth.c: code which was originally located in main.c (w/o the UART MIDI handling, which is done by MIOS32)
-> wave.s: from goom project w/o changes
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32F4 (has an on-board DAC)

===============================================================================
