$Id$

MIOS32 Tutorial #018: A simple Arpeggiator
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32
   o MBHP_AOUT, MBHP_AOUT_LC or MBHP_AOUT_NG module

===============================================================================


This tutorial example works very similar to the sequencer demonstrated
in 017_sequencer (see README.txt for details), but instead of playing
a predefined sequence, we cycle the keys pressed on a MIDI keyboard.

Another modification: the notestack is used in "SORT_HOLD" mode.
New note values are automatically sorted by the notestack handler, and
they will be held in the stack when keys are released.

The SEQ_NotifyNoteOn() function will clear the notestack (and stop the
sequencer) once all keys have been released.


Exercises
---------

Implement different arpeggiator modes, e.g. reverse direction and up&down mode.

Advanced programmers could implement an octave transpose function (e.g. notes
are cycled three times, each time transposed by +0, +1 and +2 octaves).

Very advanced programmers could implement a random key selection by using the 
$MIOS32_PATH/modules/random module.

===============================================================================
