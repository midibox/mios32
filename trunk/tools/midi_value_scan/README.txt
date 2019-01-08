$Id: README.txt 595 2009-06-06 11:25:57Z philetaylor $

MIDI Value Scan
===============================================================================
Copyright (C) 2016 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

This application allows me to generate MIDI value sweeps and to check the
response from a MIOS32 core

The program can be started with:
   midi_value_scan [--device_id <sysex-device-id>]

E.g.:
   midi_value_scan


Currently only a makefile for MacOS is provided, but it shouldn't be so
difficult to adapt it for other operating systems.

Required Libraries (MacOS):
   - MacOS Core Framework
   - libportmidi (-> see $MIOS32_PATH/drivers/gnustep/portmidi/pm_mac/README_MAC.txt)

Some MIOS32 files are used as well (although this app doesn't run on a 
microcontroller). They are taken from $MIOS32_PATH

===============================================================================
