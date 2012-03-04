$Id$

MIDIbox KB V1
===============================================================================
Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_LPC17
   o at least one MBHP_DINX4 module
   o at least one MBHP_DOUTX4 module
   o keyboard(s), e.g. from Fatar

Optional hardware:
   o TODO

Note that LCD output is currently not supported!

===============================================================================

This application is under construction!

===============================================================================

IMPORTANT:

This application has it's own USB product ID (e.g. to give it an own name), 
so that the device has to be enumerated again after the code has been uploaded.

Just reconnect the USB cable after code upload, so that your operating system
searches for the new device.

Thereafter (depending on the OS you are using) it could be required to
re-start MIOS Studio, and to configure the MIDI I/O ports again before
you are able display MIDI Monitor output on the MIOS Terminal.

===============================================================================

SysEx messages received on MIDI IN1 or transmitted to MIDI OUT1 via USB
are not displayed by the MIDI monitor to avoid data corruption (the SysEx 
stream would interfere with monitor messages)

===============================================================================

Connection Plan for Yamaha P80:

Two DOUT and two DIN Shift registers are required.

Pin b00 of the keyboard row -> connected to 1st DOUT, Pin D7
Pin b01 of the keyboard row -> connected to 1st DOUT, Pin D6
Pin b02 of the keyboard row -> connected to 1st DOUT, Pin D5
Pin b03 of the keyboard row -> connected to 1st DOUT, Pin D4
Pin b04 of the keyboard row -> connected to 1st DOUT, Pin D3
Pin b05 of the keyboard row -> connected to 1st DOUT, Pin D2
Pin b06 of the keyboard row -> connected to 1st DOUT, Pin D1
Pin b07 of the keyboard row -> connected to 1st DOUT, Pin D0
Pin b08 of the keyboard row -> connected to 2nd DOUT, Pin D7
Pin b09 of the keyboard row -> connected to 2nd DOUT, Pin D6
Pin b10 of the keyboard row -> connected to 2nd DOUT, Pin D5
Pin b11 of the keyboard row -> connected to 2nd DOUT, Pin D4
Pin b12 of the keyboard row -> connected to 2nd DOUT, Pin D3
Pin b13 of the keyboard row -> connected to 2nd DOUT, Pin D2
Pin b14 of the keyboard row -> connected to 2nd DOUT, Pin D1

Pin n10 of the keyboard column -> connected to 1st DIN, Pin D0
Pin n11 of the keyboard column -> connected to 1st DIN, Pin D1
Pin n12 of the keyboard column -> connected to 1st DIN, Pin D2
Pin n13 of the keyboard column -> connected to 1st DIN, Pin D3
Pin n14 of the keyboard column -> connected to 1st DIN, Pin D4
Pin n15 of the keyboard column -> connected to 1st DIN, Pin D5

Pin n20 of the keyboard column -> connected to 2nd DIN, Pin D0
Pin n21 of the keyboard column -> connected to 2nd DIN, Pin D1
Pin n22 of the keyboard column -> connected to 2nd DIN, Pin D2
Pin n23 of the keyboard column -> connected to 2nd DIN, Pin D3
Pin n24 of the keyboard column -> connected to 2nd DIN, Pin D4
Pin n25 of the keyboard column -> connected to 2nd DIN, Pin D5

===============================================================================

Connection Plan for Korg microKONTROL:

1st Socket with 10 pins:
Pin  1 of the keyboard row -> connected to 1st DOUT, Pin D7
Pin  2 of the keyboard row -> connected to 1st DOUT, Pin D6
Pin  3 of the keyboard row -> connected to 1st DOUT, Pin D5
Pin  4 of the keyboard row -> connected to 1st DOUT, Pin D4
Pin  5 of the keyboard row -> connected to 1st DOUT, Pin D3
Pin  6 of the keyboard row -> connected to 1st DOUT, Pin D2
Pin  7 of the keyboard row -> connected to 1st DOUT, Pin D1
Pin  8 of the keyboard row -> connected to 1st DOUT, Pin D0
Pin  9 of the keyboard row -> connected to 2nd DOUT, Pin D7
Pin 10 of the keyboard row -> connected to 2nd DOUT, Pin D6

2nd socket with 8 pins:
Pin 1 of the keyboard column -> connected to DIN, Pin D0
Pin 2 of the keyboard column -> connected to DIN, Pin D1
Pin 3 of the keyboard column -> connected to DIN, Pin D2
Pin 4 of the keyboard column -> connected to DIN, Pin D3
Pin 5 of the keyboard column -> connected to DIN, Pin D4
Pin 6 of the keyboard column -> connected to DIN, Pin D5
Pin 7 of the keyboard column -> connected to DIN, Pin D6
Pin 8 of the keyboard column -> connected to DIN, Pin D7

===============================================================================
