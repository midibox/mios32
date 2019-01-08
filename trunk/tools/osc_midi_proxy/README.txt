$Id$

OSC->MIDI proxy
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

This proxy forwards OSC messages of "MIDI" type to a MIDI Port and vice versa.
The method name "/midi" is used, and m type packets are sent/received.

The program has to be started with
   osc_midi_proxy <remote-host> <port>
E.g.:
   osc_midi_proxy 10.0.0.3 8888
or:
   osc_midi_proxy www.midibox.org 8888   (HaHa ;)


Currently only a makefile for MacOS is provided, but it shouldn't be so
difficult to adapt it for other operating systems.

Required Libraries:
   - MacOS Core Framework
   - libportmidi (-> see $MIOS32_PATH/drivers/gnustep/portmidi/pm_mac/README_MAC.txt)


Some MIOS32 files are used as well (although this app doesn't run on a 
microcontroller). They are taken from $MIOS32_PATH

===============================================================================
