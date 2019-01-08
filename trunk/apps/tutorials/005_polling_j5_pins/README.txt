$Id$

MIOS32 Tutorial #005: Polling J5 Pins
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
   o 12 Jumpers for J5A/J5B/J5C, or a cable to ground

===============================================================================

In the previous lesson we already learnt how to send MIDI events, now we want
to trigger them with input pins of the MBHP_CORE_STM32 module.

The J5A/J5B/J5C ports are predestinated for such experiments. Each port
provides 4 IO pins (A0..A3, A4..A7, A8..A11 - see schematic!) which can 
be used as digital inputs, digital outputs (3.3V level) or analog inputs.

Each pin can be configured separately, which means that a mixed configuration
(partly digital IOs, partly analog inputs) is possible.


For our example, we want to use all 12 pins as a digital inputs.

A pin should send a Note On Event whenever it is connected to ground
(reason: see layout of the J5 ports: each IO has a dedicated ground
connection, so that jumpers can be easily stuffed)

A pin should send a Note Off Event whenever it is not connected to ground
(jumper, cable, junction removed)

In order to ensure a stable voltage level when no Jumper or cable is
connected, we want to use Pull-Up resistors to pull the signal level
to 3.3V. This is required to avoid sending random values (floating
inputs -> not good!)

Fortunately STM32 provides internal pull-up devices, which can be
activated via software, so that external resistors are not required.


We will get in touch with some new functions of the MIOS32_BOARD class:

  s32 MIOS32_BOARD_J5_PinInit(u8 pin, mios32_board_pin_mode_t mode);
and
  s32 MIOS32_BOARD_J5_PinGet(u8 pin);

They are documented under
  http://www.midibox.org/mios32/manual/group___m_i_o_s32___b_o_a_r_d.html

The mios32_board_pin_mode_t we are searching for:
   MIOS32_BOARD_PIN_MODE_INPUT_PU


Initialisation of pins available at J5A, J5B and J5C (12 pins) is done
in APP_Init():
-------------------------------------------------------------------------------
  // initialize all pins of J5A, J5B and J5C as inputs with internal Pull-Up
  int pin;
  for(pin=0; pin<12; ++pin)
    MIOS32_BOARD_J5_PinInit(pin, MIOS32_BOARD_PIN_MODE_INPUT_PU);
-------------------------------------------------------------------------------


We frequently want to check the pin state of these pins.
This can be done from the APP_Background() hook, which is processed by
FreeRTOS whenever nothing else is to do (-> the system is idle).

Later we will learn better methods (-> usage of periodically called
tasks), but for the first experiments this should be sufficient.

Inside the APP_Background() hook it is allowed to run a routine
in an endless loop. Other FreeRTOS tasks (which handle MIOS32
functions in background) will interrupt the routine whenever
something is to do (e.g. checking MIDI ports for incoming data).


This is the polling loop:
-------------------------------------------------------------------------------
void APP_Background(void)
{
  u8 old_state[12]; // to store the state of 12 pins

  while( 1 ) { // endless loop
    // toggle Status LED to as a sign of live
    MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

    // check each J5 pin for changes
    int pin;
    for(pin=0; pin<12; ++pin) {
      u8 new_state = MIOS32_BOARD_J5_PinGet(pin);

      // pin changed?
      if( new_state != old_state[pin] ) {
	// store new state
	old_state[pin] = new_state;

	// send Note On/Off Event depending on new pin state
	if( new_state == 0 ) // Jumper closed -> 0V
	  MIOS32_MIDI_SendNoteOn(DEFAULT, Chn1, 0x3c + pin, 127);
	else
	  MIOS32_MIDI_SendNoteOff(DEFAULT, Chn1, 0x3c + pin, 0);
      }
    }
  }
}
-------------------------------------------------------------------------------

The code should be self-explaining, no?


You may notice, that sometimes multiple MIDI Events of the same Note value are
triggered when a Jumper/Cable is stuffed or removed.

In order to solve this, we will need to debounce the inputs.
This will be part of the next tutorial: "006_rtos_tasks"

===============================================================================
