$Id$

MBHP_CORE_STM32 module scan test
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o two MBHP_CORE_STM32 modules

===============================================================================

This application allows to test the pin connections of a core module
(called "DUT" = Device Under Test) with a second core module (called "Tester").

All pins which are listed in io_pin_table (see check.c) have to be connected 1:1
between DUT and Tester. Whenever possible, this should be done from Jxx
junction pads to cover the traces between STM32 and the Jxx header.


The DUT will drive a running-one sequence to all listed pins each second.
The status LED flashes whenever this is done.


The Tester will check this sequence.
The status LED will be turned on if the complete sequence has been
detected in the correct order.

It will be turned off whenever
  - the wrong pin has toggled
  - multiple pins have toggled
  - no pin has toggled (timeout after 5 seconds)

Accordingly, after 5 seconds the Tester should output a valid "Passed" message
on the Status LED. If a short time is desired, just decrease TIMEOUT_CYCLES
in check.c - 5 seconds has been selected by default, so that the sequence
should even pass if the DUT runs at low frequency without crystal.


Some useful debug messages are sent to the MIOS Terminal by the Tester core.

Debug messages sent by the DUT should be ignored (you will notice some MIDI
timeouts whenever MIDI Rx pins are stimulated... simply ignore them, no measure
required to fix this)


For a first check of this program, it's recommented to shorten this list,
e.g. only check J5A.
Later all pins can be enabled.
Always keep the order of pins in synch between DUT and Tester program,
otherwise the running-one sequence won't be detected.


By running "make -s", separate hex and bin files will be created for DUT and Tester.
Have a look into the Makefile to determine, how to build the projects separately
(not recommented, as test settings should always be identical between the
two binaries)


The two applications will show different names on MIOS32 queries ("Query" button
in MIOS Studio). This allows you to verify, which application is currently
running on the core.

The Tester project is called "IO Scan test (Tester)"
The DUT project is called "IO Scan test (DUT)"

===============================================================================
