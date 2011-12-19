$Id$

AINSER Jitter Monitor
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
   o a MBHP_AINSER64 module
   o up to 64 1k pots (alternatively 4.7k or 10k)

===============================================================================

Measures the jitter on the given analog inputs.

This app can be controlled via MIOS Terminal, type "help" for commands!

Following commands are available:
check_module <off|1..1>:  which AINSER module should be checked
check_mux <off|1..8>:     which AINSER multiplexer should be checked
cc <on|off>:              send CC on AIN pin changes
deadband <0..255>:        sets the AIN deadband (should be 0 for jitter checks!)
reset:                    resets the MIDIbox (!)
help:                     the help page

===============================================================================
