$Id$

SID Testtone generator
===============================================================================
Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 module
   o at least one MBHP_SID module

===============================================================================

This MIOS application generates a simple 1kHz triangle testtone
on the SID. There is no "MIDI Note On" event required to start the
sound, you should hear it immediately after the MIOS bootphase

TODO:
In addition, a 1 kHz pulse will be generated on the CS lines to the
two SIDs. This feature allows you to test the output amplifier of the
SID module (unplug SID, connect socket pin #8 with #27)

===============================================================================
