$Id$

Benchmark for MIDI Out Ports
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

Optional hardware:
   o a LCD to display result
     (however, MIOS Terminal output is much more verbose!)

===============================================================================

This benchmark sends 128 Note On events and 128 Note Off Events over different
MIDI Ports (USB0, UART0, IIC0) with and without Running Status optimisation,
and measures the transfer time.

The notes can be recorded with a sequencer for visualisation, see also this
forum posting: http://www.midibox.org/forum/index.php/topic,13542.0.html

===============================================================================
