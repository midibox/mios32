$Id$

MIDI USB 4x4 Interface Driver
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_LPC17

Optional hardware:
   o four LEDs connected to J5A.A0..A3 and J5B.A4..A7 to display Tx/Rx status
   o four jumpers to select diagnosis options

Note that LCD output for the MIDI monitor function is not supported!

===============================================================================

This application acts as a simple 4x4 USB MIDI interface.

Beside of the common usecase (communication with MIDI devices), it is intended
as a test application for the four MIDI IN and OUT ports of the MBHP_CORE_LPC17
module.

An integrated MIDI monitor is provided which sends incoming MIDI events to 
a MIOS terminal (part of MIOS Studio) in plain text format via the first 
USB port for diagnosis.

This function can be activated by jumpering J5B.A4 to ground.

By jumpering J5B.A5 to ground, Active Sense, MIDI Clock and MTC messages
are filtered.

By jumpering J5B.A6 to ground, the beat number and tempo (BPM) is displayed
on each beat when a MIDI clock is received.

By jumpering J5B.A7 to ground, the MIDI time code (MTC) is displayed 
in plain text format whenever all 8 MTC messages have been received.
(NOT IMPLEMENTED YET)


4 LEDs can be connected to J5A.A0..A3 to display the receive/transmit
status. The cathodes (short legs) have to be connected to Ground (J5A.Vs),
and the anodes (long legs) via serial resistors (220 Ohm) to following
J5A.Ax pins:

J5A.A0: OUT1 Tx
J5A.A1: OUT2 Tx
J5A.A2: OUT3 Tx
J5A.A3: OUT4 Tx
J5B.A4: IN1 Rx
J5B.A5: IN2 Rx
J5B.A6: IN3 Rx
J5B.A7: IN4 Rx

The Status LED of the MBHP_CORE_STM32 module flickers on *each*
incoming/outgoing MIDI event.

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
