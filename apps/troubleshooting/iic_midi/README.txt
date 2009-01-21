$Id$

IIC_MIDI test
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 module
   o at least one IIC_MIDI module

Optional hardware:
   o LCD to display connection status of IIC_MIDI modules


===============================================================================

This application scans for available MBHP_IIC_MIDI devices each second
and displays the (dis)connection status on the LCD.

In addition, the status will be sent to the MIOS Terminal whenever it has changed.

To test the MIDI In/Out port of a module, just connect them with the
MIDI interface of your PC, select the MIDI ports of your interface in MIOS Studio,
and press the "Query" button. You should get a response like if you would
query via USB.


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

===============================================================================
