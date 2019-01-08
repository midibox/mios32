$Id$

MBHP_CORE_STM32 module test
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 module

===============================================================================

This application scans all IO pins of the MBHP_CORE_STM32 module to
check for shorts/bridges.

The test will be repeated with a period of 5 seconds

Please open the MIOS Terminal in MIOS Studio to see the test results.

Ensure that no external module/device is connected to the board while
the test is running (no JTAG Wiggler, no MIDI cables, no LCD, no SD Card, no pots, etc...)

===============================================================================
