$Id$

MIOS32 Tutorial #008: J5 Outputs
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
   o 12 LEDs
   o 12 220 Ohm resistors
   o 3 2x5 pin sockets

===============================================================================

Back to the IO pins of J5A/J5B/J5C, which where used in previous lessons
as digital inputs. Now we want to test the output function.

Remove all jumpers on J5 sockets (if still stuffed) to avoid short circuits!

Now connect LEDs between J5A:A0..J5A:A3, J5B:A4..J5A:A7, J5C:A8..J5A:A11
and ground the following way: The cathodes (short legs) have to be connected 
to Ground (J5A.Vs), and the anodes (long legs) via serial resistors (220 Ohm) 
to Ax pins.
It is recommented to solder them at the top of 2x5 pin sockets (don't solder
directly on the board), so that the LEDs can be easily removed later.


In APP_Init(), the output function of all J5 pins is activated:
-------------------------------------------------------------------------------
  // initialize all pins of J5A, J5B and J5C as outputs in Push-Pull Mode
  int pin;
  for(pin=0; pin<12; ++pin)
    MIOS32_BOARD_J5_PinInit(pin, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
-------------------------------------------------------------------------------


In APP_MIDI_NotifyPackage(), LEDs are turned on/off based on the received
MIDI note (on any MIDI port and any MIDI channel):
-------------------------------------------------------------------------------
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // 1) the LED should be turned on whenever a Note On Event with velocity > 0
  // has been received
  if( midi_package.type == NoteOn && midi_package.velocity > 0 ) {
    // determine LED number (one of 12, each octave)
    u8 led = midi_package.note % 12;
    // turn on LED
    MIOS32_BOARD_J5_PinSet(led, 1);
  }

  // 2) the LED should be turned off whenever a Note Off Event or a Note On
  // event with velocity == 0 has been received (the MIDI spec says, that velocity 0
  // should be handled like Note Off)
  else if( midi_package.type == NoteOff ||
	   midi_package.type == NoteOn && midi_package.velocity == 0 ) {
    // determine LED number (one of 12, each octave)
    u8 led = midi_package.note % 12;
    // turn off LED
    MIOS32_BOARD_J5_PinSet(led, 0);
  }
}
-------------------------------------------------------------------------------

Now play some notes, either with a connected MIDI keyboard, or with the virtual
keyboard of MIOS Studio.


Exercise
--------

Connect a reed relay, and control your coffee machine with the core module.

===============================================================================
