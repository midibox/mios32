$Id$

MIOS32 Tutorial #012: Scanning up to 64 analog pots
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17 or MBHP_CORE_STM32F4 or similar
   o one or two MBHP_AIN modules
   o up to 64 1k pots (alternatively 4.7k or 10k)

===============================================================================

While we connected pots directly to J5 ports in the previous tutorial
lesson (011_ain), now we want to scan even more by multiplexing them
via 4051 CMOS multiplexers (-> MBHP_AIN module)

Each multiplexer channel allows to select one of 8 channels.
The mux inputs should be connected to J5A.A0..A3 and J5B.A4..A7.
The three mux selection lines should be connected to J5C.A8..A10
as specified in the mios32_config.h file.


With MIOS32_AIN_MUX_PINS set to 3, MIOS32 will automatically scan
each analog input 8 (2^3) times, each time with a different multiplexer
selection.

Accordingly, the APP_AIN_NotifyChange() hook will forward pin
numbers between 0 and 63.

The pin values are scrambled the following way:
  - J5A.A0 input samples pin 0, 8, 16, 24, 32, 40, 48, 56
  - J5A.A1 input samples pin 1, 9, 17, 25, 33, 41, 49, 57
  - ...
  - J5A.A7 input samples pin 7, 15, 23, 31, 39, 47, 55, 63


If you prefer the same scrambling which was used for MIOS8
  - J5A.A0 input should lead to pin number 0-7
  - J5A.A0 input should lead to pin number 8-15
  - ...
  - J5A.A7 input should lead to pin number 56-63

then just convert the pin number at the top of the AIN notification hook:
-------------------------------------------------------------------------------
  // for MIOS8 compatible pin enumeration:
  pin = ((pin&0x7)<<3) | (pin>>3);
-------------------------------------------------------------------------------


Note that due to the reduced conversion voltage range (0..3.3V, see also
descriptions in 011_ain), it has to be taken special care for cables to
the MBHP_AIN modules and to the pots.
Shielded cables, as short as possible, are strongly recommented to avoid
jittering values!

===============================================================================
