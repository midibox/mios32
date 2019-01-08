$Id$

MIOS32 Tutorial #010: Scanning up to 128 buttons connected to DINX4 modules
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17 or MBHP_CORE_STM32F4 or similar
   o up to 4 DINX4 modules
   o up to 128 buttons

Optionally:
   o up to 4 DOUTX4 modules
   o up to 128 LEDs

===============================================================================

While 009_dout controlled 128 LEDs, with this tutorial lesson we will learn
how to scan 128 buttons.

Similar to the approaches explained before, we could time multiplex button
scans as well. But using DINX4 modules is the most simple and prefered one
for MIDIboxes.

Buttons are connected to 74HC165 shift registers (short term: DIN SRs).
Each input pin has a 10k pull-up resistor to 5V. To ensures a stable
voltage level when the button is not pressed (pin not shortened to
ground).

Accordingly, we will read 0 (=ground) whenever the button is
pressed, and 1 (=5V9 whenever the button is depressed (-> low active)

Similar to DOUT registers, DIN registers are scanned via DMA in
background as well, so that almost no CPU time is consumed whenever
MIOS32 has to check for button changes.

Each millisecond we get the new state of new buttons, and MIOS32
(resp. the programming model) will call the APP_DIN_NotifyToggle()
hook whenever a button state has been changed.

Sending a MIDI note on each button movement is very simple:
-------------------------------------------------------------------------------
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
  // toggle Status LED on each DIN pin change
  MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

  // send MIDI event
  MIOS32_MIDI_SendNoteOn(DEFAULT, Chn1, (pin + MIDI_STARTNOTE) & 0x7f, pin_value ? 0x00 : 0x7f);
}
-------------------------------------------------------------------------------

The first button will send C-2 (as defined with MIDI_STARTNOTE)

We always send a Note On Event
Velocity is 0x7f (= max value) when the button has been pressed (pin_value == 0),
and 0x00 (equal to Note Off) when the button has been depressed (pin_value == 1)

===============================================================================
