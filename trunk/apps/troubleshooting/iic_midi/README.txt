$Id$

IIC_MIDI test
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32, MBHP_CORE_LPC17 or MBHP_CORE_STM32F4 module
   o at least one IIC_MIDI module


===============================================================================

This application scans for available MBHP_IIC_MIDI devices each 200 mS
and displays the (dis)connection status in the MIOS Terminal (MIOS Studio)

Module behaviour:
   o Power LED (green) on: the PIC is running, initialization phase finished
   o Rx LED (yellow) flickers: a MIDI message has been received
   o Tx LED (red) flickers: a MIDI message has been sent

Notes:
   o the yellow LED will only flicker until the MIDI buffer 
     of the MBHP_IIC_MIDI slave is full. This scenario can happen 
     if the core cannot poll for the incoming data. 
     This means in other words: this effect can be noticed, if the 
     SC/SD lines are not connected correctly

   o this test program does *not* require a connection between 
     MBHP_IIC_MIDI::J2:RI and the core
     To test this connection, open mios32_config.h, set the appr. 
     MIOS32_IIC_MIDIx_ENABLED switch to "2", ensure that the correct
     pin is specified with MIOS32_IIC_MIDIx_RI_N_* and rebuild this
     application.


Available commands (enter 'help' in MIOS terminal)

scan:                    scan for available IIC_MIDI interfaces
ping:                    ping all IIC_MIDI interfaces
stress:                  stressing all IIC_MIDI interfaces for ca. 1 second
outtest:                 enables/disables MIDI OUT test (sends a CC each 200 mS)
reset:                   resets the MIDIbox (!)
help:                    this page


The "outtest" is the most useful test, because it sends CC events over each
module periodically in a round robin manner.

Means:
- first the red (Tx) LED of the module assigned to IIC1 should flicker
- after 200 mS the red (Tx) LED of the module assigned to IIC2 should flicker
- after 200 mS the red (Tx) LED of the module assigned to IIC3 should flicker
- etc

It's allowed to connect/disconnect the modules during runtime, they will be
automatically detected, and a message will appear, such as:

[407563.920] MBHP_IIC_MIDI module #1 has been disconnected!
[407563.920] MBHP_IIC_MIDI module #2 has been disconnected!
[407563.921] MBHP_IIC_MIDI module #3 has been disconnected!
[407563.921] MBHP_IIC_MIDI module #4 has been disconnected!
[407564.520] MBHP_IIC_MIDI module #1 connected!
[407564.520] MBHP_IIC_MIDI module #2 connected!
[407564.521] MBHP_IIC_MIDI module #3 connected!
[407564.521] MBHP_IIC_MIDI module #4 connected!

===============================================================================
