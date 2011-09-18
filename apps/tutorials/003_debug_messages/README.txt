$Id$

MIOS32 Tutorial #003: Debug Messages
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

===============================================================================

Before continuing with the next tutorials, I would like to introduce an
important debugging function which will help you to understand the data flow
in applications better.

With MIOS32_MIDI_SendDebugMessage, formatted messages can be sent
to the MIOS Terminal. This terminal is part of MIOS Studio, but is
also available as command line tool (-> $MIOS32_PATH/tools/mios32_terminal)


MIOS32_MIDI_SendDebugMessage works similar to "printf", but doesn't
provide all formatting possibilities to save program memory and
speed up execution.

Here a general overview of formats:
http://www.pixelbeat.org/programming/gcc/format_specs.html

Supported conversion specifiers:
%s (string)
%d (signed int)
%x (unsigned hex int with lower-case letters)
%X (unsigned hex int with upper-case letters)
%u (unsigned int)
%c (single character)

Supported flags:
0[field with] (pad with zeroes, e.g. %03d for 000..999)
- (to print item left-aligned)


Following code of the example in app.c:

-------------------------------------------------------------------------------
  // send received MIDI package to MIOS Terminal
  MIOS32_MIDI_SendDebugMessage("Port:%02X  Type:%X  Evnt0:%02X  Evnt1:%02X  Evnt2:%02X\n",
			       port,
			       midi_package.type,
			       midi_package.evnt0, midi_package.evnt1, midi_package.evnt2);
-------------------------------------------------------------------------------

will lead to following output in the MIOS terminal whenever a MIDI
event is received (here: Note and CC events):

-------------------------------------------------------------------------------
00000000003621 ms | Port:10  Type:9  Evnt0:90  Evnt1:32  Evnt2:64
00000000003758 ms | Port:10  Type:9  Evnt0:90  Evnt1:32  Evnt2:00
00000000004271 ms | Port:10  Type:9  Evnt0:90  Evnt1:3C  Evnt2:64
00000000004388 ms | Port:10  Type:9  Evnt0:90  Evnt1:3C  Evnt2:00
00000000005223 ms | Port:10  Type:9  Evnt0:90  Evnt1:45  Evnt2:64
00000000005345 ms | Port:10  Type:9  Evnt0:90  Evnt1:45  Evnt2:00
00000000007568 ms | Port:10  Type:B  Evnt0:B0  Evnt1:07  Evnt2:3A
00000000008210 ms | Port:10  Type:B  Evnt0:B0  Evnt1:07  Evnt2:46
00000000008702 ms | Port:10  Type:B  Evnt0:B0  Evnt1:07  Evnt2:52
-------------------------------------------------------------------------------


Technical background: debug strings are framed in a SysEx stream and sent
to the DEBUG port, which points to USB0 by default, but could also be
changed by the user during runtime.

The format string contains %X/%02X conversion specifiers to translate
arguments passed to the MIOS32_MIDI_SendDebugMessage() function
into hexadecimal numbers, padded with zeroes.


Exercise
--------

Enhance the APP_MIDI_NotifyPackage() hook, so that it prints the MIDI port
and Event Type in plain text format, e.g.:

-------------------------------------------------------------------------------
00000000003621 ms | USB0  NoteOn  (90 32 64)
00000000003758 ms | USB0  NoteOn  (90 32 00)
00000000004271 ms | USB0  NoteOn  (90 3C 64)
00000000004388 ms | USB0  NoteOn  (90 3C 00)
00000000005223 ms | USB0  NoteOn  (90 45 64)
00000000005345 ms | USB0  NoteOn  (90 45 00)
00000000007568 ms | USB0  CC      (B0 07 3A)
00000000008210 ms | USB0  CC      (B0 07 46)
00000000008702 ms | USB0  CC      (B0 07 52)
-------------------------------------------------------------------------------

Who will program the most verbose MIDI monitor?

===============================================================================
