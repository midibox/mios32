$Id$

AIN Jitter Monitor
===============================================================================
Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17 or MBHP_CORE_STM32F4
   o pots for each AIN input

===============================================================================

Measures the jitter on the given analog inputs.

This app can be controlled via MIOS Terminal, type "help" for commands!

Following commands are available:
check <on|off>:           enable/disable the monitor
cc <on|off>:              send CC on AIN pin changes
deadband <0..255>:        sets the AIN deadband (should be 0 for jitter checks!)
reset:                    resets the MIDIbox (!)
help:                     the help page

See also http://www.ucapps.de/mbhp_ainser64.html for further explanations.

===============================================================================
