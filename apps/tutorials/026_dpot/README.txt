$Id: README.txt $

MIOS32 Tutorial #026: Using DPOTs
===============================================================================
Copyright (C) 2010 Jonathan Farmer (JRFarmer.com@gmail.com)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17

Optional hardware:
   o 2x Microchip MCP42XXX digital potentiometers with SPI interface, for a
     total of 4 digital potentiometers.  MCP42100 devices were used during
     development and testing.  Digital pots are configured using Figure 5-4 
     "Daisy-Chain Configuration" found in the datasheet:
     http://ww1.microchip.com/downloads/en/DeviceDoc/11195c.pdf

===============================================================================

This tutorial lesson demonstrates the digital potentiometer (DPOT) application
interface and device driver located at $MIOS32_PATH/modules/dpot 

The DPOT Module is added to the project from the Makefile (-> see comments there)

This tutorial is setup to communicate with 2x MCP42XXX chips.  These chips are 
commanded from SPI2 port, J19, using RC pin 0.


IMPORTANT
---------

Before digital potentiometer changes are transfered to the external hardware,
the DPOT_Set_Value function compares the new value with the current one.
If equal, the register transfer will be omitted, otherwise it
will be requested and performed once DPOT_Update() is called.

This method has two advantages:
  o if DPOT_Set_Value doesn't notice value changes, the appr. DPOT channels
     won't be updated to save CPU performance
  o all potentiometers will be updated at the same moment


So, don't forget to execute:
-------------------------------------------------------------------------------
    DPOT_Update();
-------------------------------------------------------------------------------

either immediately after potentiometer changes, or periodically from a timed task.


Testing the application
-----------------------

The 4 potentiometers channels will be initialized with distinct values:
	1: 0x00
	2: 0x40
	3: 0x80
	4: 0xC0

During each cycle of the infinite loop located in APP_Background(), the digital
potentiometer values will be committed to the hardware.  Then, each potentiometer 
value will be incremented.  The increment will be commited to the hardware on the 
next cycle.  If a supply voltage is provided across the A and B terminals of the 
potentiometers, a "saw-tooth" waveform will be present at the wiper terminal.

Vd __    ,    ,    ,    ,    , 
        /|   /|   /|   /|   /|
       / |  / |  / |  / |  / |  
      /  | /  | /  | /  | /  |  . . .
Vs __/   |/   |/   |/   |/   |

Each of the four digital potentiometers can be modified using a MIDI CC message.
This application will accept MIDI messages on any MIDI channel.  The first potentiometer 
can be modified using MIDI CC #7, the second using MIDI CC#8 and so on.

Once a potentiometer has been modified using MIDI CC message, the auto-incrementing
for that potentiometer is disabled (forever, until you power cycle the core).

===============================================================================
