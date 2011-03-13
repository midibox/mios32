$Id$

CV Autotune
===============================================================================
Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or STM32 Primer
   o MBHP_AOUT, MBHP_AOUT_LC or MBHP_AOUT_NG module
   o a LM339 comparator (see schematics/comparator_circuit.pdf)

===============================================================================

This application demonstrates how an Autotune mechanism for analog synthesizers
could be implemented.

frq_meter.c provides a frequency measurement function which uses a
capture/compare channel of the STM32 (available at J5B.A0) to deterime
the frequency of the VCO.

See schematics/comparator_circuit.pdf for details, and 
schematics/comparator_waveforms.jpg for the expected waveform.

The VCO is controlled by an AOUT module (by default: AOUT_NG, Channel #1)

The Autotune mechanism is implemented in TASK_Frq_Measure (-> app.c)

Autotune starts when MIDI Note c-2 (0x00) is played (this key has been selected
just for demo purposes).

It sweeps the AOUT value and collects matching frequencies from the lowest 
to the highest MIDI note.

Since individual frequencies for each notes are captured, this means that
the mechanism will work with V/Oct and Hz/V w/o additional configuration!

It's even possible to use different frequency tables, by default the
"equal-tempered scale" is predefined.

===============================================================================
