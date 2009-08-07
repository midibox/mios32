$Id$

MIOS32 Tutorial #019: A MIDI Player plays from SD Card
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
   o a SD Card + SD Card socket

===============================================================================

This tutorial is provided in the hope that it doesn't confuse too much
due to the number of files and cross references between functions.

Main intention is to demonstrate some of the useful modules provided with MIOS32.
The simpleness of the user interface should inspire you to improve the small app
to an even more powerful MIDI file player.


Features:
  - reads and playbacks .mid files from SD Card
  - files are played once SD Card has been detected.
  - the sequencer stops automatically when SD Card is removed.
  - Start/Stop/Pause/Next Song can be controlled from an external MIDI keyboard
    (or with virtual keyboard of MIOS Studio)
  - MIOS Terminal outputs file accesses and Lyrics (if provided by .mid file)
  - MIDI event schedule ensures perfect timings, even if the CPU is loaded
    with additional tasks
  - thanks to the generic tempo generator (MIDI_BPM), MIDI player can be 
    synchronized to external MIDI clock source


We already know the sequencer and MIDI scheduler module from tutorials
017_sequencer and 018_arpeggiator. Now we will use it for playing
back MIDI files.

Some additional modules are used here:

  o $MIOS32_PATH/modules/midifile: provides a generic MIDI file parser
    which uses callback functions to access the file system

  o $MIOS32_PATH/modules/dosfs: driver to access FAT12/FAT16/FAT32
    file system, provided as Open Source by Lewin Edwards.
    This driver has been enhanced for MIOS32, so that MIOS32_SDCARD
    functions are used to read/write sectors


Source files:

  o app.c: you should already know this one... this file contains all
    hooks called from the programming model, and it starts a task which
    handles sequencer (and SD Card access) functions

  o seq.c: should already been known from previous tutorials. But instead
    of generating MIDI events internally, they will be fetched from the
    MIDI file parser (see comments in code)
    DEBUG_VERBOSE_LEVEL is set to 1, so that Lyrics are sent to MIOS Terminal

  o mid_file.c: contains the callbacks to access the filesystem.
    This is the link between the MIDI File parser and DOSFS driver
    DEBUG_VERBOSE_LEVEL is set to 2 to monitor the file access activity


Exercises
---------

1) implement a user interface to control the sequencer and MIDI file reader
  (buttons/status LED)

3) output filename on an LC display

4) output Lyrics on a LCD in a nice readable format!

2) advanced: enhance SEQ_PlayFileReq(), SEQ_PlayFile(), so that the previous
   file can be selected (especially requires new code in mid_file.c, see
   MID_FILE_FindNext()

===============================================================================
