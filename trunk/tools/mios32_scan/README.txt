$Id$

MIOS32 Scan
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

This tool scans for MIOS32 devices on all available MIDI Ports, and prints
some useful informations (e.g. OS, Board, Derivative, etc.)

The same informations are output by the QUERY button of MIOS Studio.


The program can be started with:
   mios_terminal [--in <in-port-number>] [--out <out-port-number>] [--device_id <sysex-device-id>]

E.g.:
   mios_terminal
   (MIDI In/Out port numbers will be requested)
or:
   mios_terminal --in 0 --out 16
   (if you already know the numbers)


Example for a successful query response:
--------------------------------------------------------------------------------
Scanning [28] CoreMIDI: MIDIbox SEQ V4 Anschluss 1
Operating System: MIOS32
Board: MBHP_CORE_STM32
Core Family: STM32F10x
Chip ID: 0x00000000
Serial Number: #35FFFFFF5836393643472443
Flash Memory Size: 524288 bytes
RAM Size: 65536 bytes
MIDIbox SEQ V4.0Alpha
(C) 2009 T. Klose
--------------------------------------------------------------------------------



Currently only makefiles for MacOS/Windows are provided, but it shouldn't be so
difficult to adapt it for other operating systems.

Required Libraries:
   - MacOS Core Framework
   - libportmidi (-> see $MIOS32_PATH/drivers/gnustep/portmidi/pm_mac/README_MAC.txt)


Some MIOS32 files are used as well (although this app doesn't run on a 
microcontroller). They are taken from $MIOS32_PATH

===============================================================================

Compiling under Windows
This is tested under Microsoft Visual C++ 9.0 Express Edition.

First you must create portmidi.lib and porttime.lib. from $MIOS32_PATH/drivers/gnustep/portmidi
directory open the portmidi (or portmidi-vc9) .sln file and build portmidi and porttime. 

You will need to check that VSINSTALLDIR/VCINSTALLDIR are correct in make.bat. 
It is a simple batch file which should set the rest of the environment. Then type

make /f makefile.win 

===============================================================================