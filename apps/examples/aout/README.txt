$Id$

AOUT Example
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or STM32 Primer
   o at least one MBHP_AOUT, MBHP_AOUT_LC or MBHP_AOUT_NG module

===============================================================================

This example demonstrates the usage of the AOUT module driver which is
located under $MIOS32_PATH/modules/aout


AOUT Channels (and MBHP_AOUT only: two gates) are mapped to following MIDI events:

- AOUT Channel #1 controlled by MIDI Note at MIDI Channel #1
- AOUT Channel #2 controlled by Note Velocity at MIDI Channel #1
- AOUT Digital Output (MBHP_AOUT only) controlled by Note at MIDI Channel #1
- AOUT Channel #3 controlled by CC#1 at MIDI Channel #1
- AOUT Channel #4 controlled by CC#7 at MIDI Channel #1
- AOUT Channel #5 controlled by MIDI Note at MIDI Channel #2
- AOUT Channel #6 controlled by Note Velocity at MIDI Channel #2
- AOUT Digital Output (MBHP_AOUT only) controlled by Note at MIDI Channel #2
- AOUT Channel #7 controlled by CC#1 at MIDI Channel #2
- AOUT Channel #8 controlled by CC#7 at MIDI Channel #2


MBHP_AOUT is selected by default.

If a different AOUT module should be tested, an adaption in Init() (-> app.c)
is required. Change config.if_type to one of the following values:
  - AOUT_IF_MAX525
  - AOUT_IF_74HC595
  - AOUT_IF_TLV5630


The Module is connected to J19 of the MBHP_CORE_STM32 module:
   J19:Vs -> AOUT Vs
   J19:Vd -> AOUT Vd
   J19:SO -> AOUT SI
   J19:SC -> AOUT SC
   J19:RC1 -> AOUT CS
Note that this is *not* an 1:1 pin assignment (an adapter has to be used)!

The voltage configuration jumper of J19 has to be set to 5V, and
a 4x1k Pull-Up resistor array should be installed, since the IO pins
are configured in open-drain mode for 3.3V->5V level shifting.

More informations can be found in the AOUT driver documented located under
http://www.midibox.org/mios32/manual

===============================================================================
