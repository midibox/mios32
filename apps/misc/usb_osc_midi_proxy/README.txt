$Id$

USB OSC MIDI Proxy
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

Optional hardware:
   o four LEDs connected to J5A.A0..A3 to display Tx/Rx status
   o MBHP_ETH module for ethernet connection

Note that LCD output is not supported!

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

OSC Port mapping:
   USB0 <-> OSC /midi1
   USB1 <-> OSC /midi2
   UART0 <-> OSC /midi3
   UART1 <-> OSC /midi4

===============================================================================
