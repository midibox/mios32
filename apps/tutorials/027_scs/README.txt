$Id$

MIOS32 Tutorial #027: Standard Control Surface
===============================================================================
Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_LPC17 (MBHP_CORE_STM32 *)
   o a "standard control surface"
     Schematic: http://www.ucapps.de/mbhp_scs.html

(*) MBHP_CORE_STM32 doesn't provide the Digital IO port J10, therefore
a serial shift register (74HC165, "DINX1") is required in addition

===============================================================================

Doc: TODO

===============================================================================
