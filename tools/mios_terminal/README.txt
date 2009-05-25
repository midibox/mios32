$Id$

MIOS Input/Output Terminal
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

This is the command line version of the MIOS Terminal, which can also be found
in MIOS Studio as very helpful debugging option.

Messages sent with MIOS32_MIDI_SendDebug* are print by the terminal.

In addition, it's also possible to send strings (commands) to the core, which
is forwarded to APP_NotifyReceivedCOM()

The program can be started with:
   mios_terminal [--in <in-port-number>] [--out <out-port-number>] [--device_id <sysex-device-id>]

E.g.:
   mios_terminal
   (MIDI In/Out port numbers will be requested)
or:
   mios_terminal --in 0 --out 16
   (if you already know the numbers)


Currently only a makefile for MacOS is provided, but it shouldn't be so
difficult to adapt it for other operating systems.

Required Libraries:
   - MacOS Core Framework
   - libportmidi (-> see $MIOS32_PATH/drivers/gnustep/portmidi/pm_mac/README_MAC.txt)


Some MIOS32 files are used as well (although this app doesn't run on a 
microcontroller). They are taken from $MIOS32_PATH

===============================================================================
