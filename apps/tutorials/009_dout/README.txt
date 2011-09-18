$Id$

MIOS32 Tutorial #009: Controlling up to 128 LEDs with DOUTX4 Modules
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17
   o up to 4 DOUTX4 modules
   o up to 128 LEDs

===============================================================================

While the method introduced with the 008_j5_outputs tutorial only allowed to 
control 12 LEDs, we want to increase the number now.

There are several ways to realize this:

1) time multiplexing the LEDs on J5, e.g. a 4x8 configuration would allow
   to control up to 32 LEDs
   Advantage:
     - cheap
   Disadvantages:
     - reduced brightness of LEDs,
     - J5 not free for other purposes (e.g. connecting jumpers or analog pots)
     - requires some programming as not natively supported by MIOS32

2) adding serial shift registers, and let MIOS32_SRIO driver scan them
   in background.
   Advantages:
     - common method for most MIDIboxes (most people already own
       MBHP_DOUTX4 modules)
     - dedicated (74HC595 based) driver for each LED
     - 5V outputs (instead of 3.3V outputs provided by STM32)
     - J5 pins free for other purposes
   Disadvantage:
     - slightly higher costs (however, DOUTX4 modules are still cheap!)


3) combination of 1) and 2): time multiplexing of serial shift registers.
   The usage of the so called "BLM" (Button/LED Matrix driver) will be 
   part of a later tutorial.


Demonstration of Method 2) - the most simple and prefered one.

Connect one or more (chained) MBHP_DOUTX4 modules to J8 (socket combined
with J9). Select the 5V option, and ensure that a 4x220 Ohm resistor
array is stuffed on R31 to pull-up the control lines to 5V


No special initialisation is required in APP_Init(), as MIOS32 always
scans up to 16 74HC595 shift registers (short term: DOUT SR) periodically
each millisecond.

Note that almost no CPU time is consumed for this process, since the
integrated DMA controller of STM32 is used which handles the scan 
autonomously, so that MIOS32 only needs to take over the new values
whenever the scan is finished.

Accordingly, you wouldn't notice an effect if less registers are
scanned. More than 16 registers shouldn't be connected (and they
are not natively supported) due to physical reasons (longer chains
can lead to unstable serial shifts).


Enough bla bla, the MIDI handler which allows to control 128 LEDs
looks really simple. You probably already know this processing structure
from previous examples - only MIOS32_BOARD* has been replaced by
MIOS32_DOUT_PinSet()

MIOS32_DOUT functions are documented under:
  http://www.midibox.org/mios32/manual/group___m_i_o_s32___d_o_u_t.html

-------------------------------------------------------------------------------
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // toggle Status LED on each incoming MIDI package
  MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

  // 1) the LED should be turned on whenever a Note On Event with velocity > 0
  // has been received
  if( midi_package.type == NoteOn && midi_package.velocity > 0 ) {
    // determine pin number (add offset, so that the first LED starts at C-2)
    u8 pin = (midi_package.note - MIDI_STARTNOTE) & 0x7f;
    // turn on LED
    MIOS32_DOUT_PinSet(pin, 1);
  }

  // 2) the LED should be turned off whenever a Note Off Event or a Note On
  // event with velocity == 0 has been received (the MIDI spec says, that velocity 0
  // should be handled like Note Off)
  else if( midi_package.type == NoteOff ||
	   midi_package.type == NoteOn && midi_package.velocity == 0 ) {
    // determine pin number (add offset, so that the first LED starts at C-2)
    u8 pin = (midi_package.note - MIDI_STARTNOTE) & 0x7f;
    // turn off LED
    MIOS32_DOUT_PinSet(pin, 0);
  }
}
-------------------------------------------------------------------------------

Now play some notes, either with a connected MIDI keyboard, or with the virtual
keyboard of MIOS Studio.

The first LED is controlled by C-2 (as defined with MIDI_STARTNOTE)

===============================================================================
