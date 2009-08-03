$Id$

MIOS32 Tutorial #017: A simple Sequencer
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

===============================================================================

This tutorial lesson demonstrates the usage of:

  o $MIOS32_PATH/modules/sequencer: provides a very flexible BPM generator
    (SEQ_BPM) and a MIDI event scheduler (SEQ_MIDI_OUT)

  o $MIOS32_PATH/modules/notestack: a note stack handler which stores
    received Notes in a stack, so that it is possible to recall the
    note number and velocity of still played notes when a key is released

Modules are added to the project from the Makefile (-> see comments there)


The sequencer toplevel functions are implemented in a separate file (seq.c)
to improve the oversight.


Sequencer
---------

The BPM generator can be used in MIDI Clock Master and Slave mode with
a definable resolution, which can optionally be (much) higher than the
common MIDI clock resolution. It it configured for Auto mode, which means,
that it will generate an internal clock by default (Master), and synchronize
to an extern clock once the appr. MIDI clock events are received (Slave).
In slave mode, the incoming clock is multiplied depending on the defined PPQN
(pulses per quarter note).

The MIDI event scheduler queues MIDI events which should be played at
a given MIDI clock tick. The approach has the big advantage, that
events can be pre-generated, eg. to bridge the time while loading new
pattern(s) from a SD Card, or to generate effects like MIDI Echo (very
simple, just put the note multiple times into the queue with different
bpm_tick values).
The advantage of using the MIDI clock as time base instead of an absolute
time is, that the sequencer is even in synch if the tempo is changed
while unplayed events are in the queue.

This module is used by MIDIbox SEQ V4 and has been proven as very
stable! :-)


The sequencer handler (SEQ_Handler in seq.c) manages requests which 
are notified by the BPM generator. It is called peridocially 
from a separate task with high priority (higher than MIOS32 
background tasks for most stable timings).

The order in which requests like SEQ_BPM_ChkReqStop(), SEQ_BPM_ChkReqCont().
SEQ_BPM_ChkReqStart(), SEQ_BPM_ChkReqSongPos() and SEQ_BPM_ChkReqClk()
are processed shouldn't be changed!


In this simple example application, a predefined sequence is
played whenever a note is played on a keyboard. The note number
is used to transpose the sequence.

The sequencer will stop once all notes/keys have been depressed.

Other strategies are possible as well of course - this handling 
has been choosen since it allows to demonstrate the usage w/o 
the need for a Play/Stop button which control the BPM generator.


Notestack Driver
----------------

see description in 016_aout/README.txt


The notestack is forwarded to SEQ_NotifyNewNote(), from there
the sequencer is started/stopped, and the transpose value is
changed depending on the last played key.


For a more effective usage of the note stack, see the next tutorial
(018_arpeggiator) which demonstrates an arpeggiator.

===============================================================================
