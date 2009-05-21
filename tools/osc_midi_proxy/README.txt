$Id$

OSC->MIDI proxy
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

This proxy forwards OSC messages of "MIDI" type to a MIDI Port and vice versa.

The program has to be started with
   osc_midi_proxy <port-number>
E.g.:
   osc_midi_proxy 8888


Currently only a makefile for MacOS is provided, but it shouldn't be so
difficult to adapt it for other operating systems.

Required Libraries:
   - MacOS Core Framework
   - libportmidi (-> see $MIOS32_PATH/drivers/gnustep/portmidi/pm_mac/README_MAC.txt)


Some MIOS32 files are used as well (although this app doesn't run on a 
microcontroller). They are taken from $MIOS32_PATH

===============================================================================

Note: MIDI IN->OSC Out direction not implemented yet (I've no use for it yet...)

===============================================================================
