$Id$

MIOS32 Tutorial #029: Fast Scan Matrix for velocity sensitive Keyboard
===============================================================================
Copyright (C) 2012 Thorsten Klose(tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17
   o 1 DIN SR
   o 2 DOUT SRs

===============================================================================

This application demonstrates how to midify a keyboard with velocity sensitivity.

The keyboard of a Korg microKONTROL has been used for testing.
Each key has two contacts: one which closes early when you are starting to press
the key, and another contact which closes when the key is completely pressed.
They are called "break" and "make" contact below.

In order to determine the velocity, we've to measure the time between the closing
break and make contact. Via experiments I found out, that the minimum delay
(key pressed fast) is ca. 1.2 mS, and the maximum delay is ca. 60 mS

Since a common SRIO scan is only performed each mS, which is too slow for high
accuracy delay measurements, I had to bypass this mechanism via
MIOS32_DONT_SERVICE_SRIO_SCAN in mios32_config.h, and handle the scan in app.c
instead.

MIOS32_SRIO_ScanStart is now called immediately (again) when a scan has finished,
accordingly only the transfer rate limits the scan speed. It has been set to
MIOS32_SPI_PRESCALER_64 (0.64 uS on a 120 MHz LPC1769).

The resulting matrix scan speed is ca. 300 uS for 16 rows and 8 columns (= 128 contacts)

For comparison: the original firmware of Korg microKONTROL scans the matrix each
500 uS, which means that the accuracy of our own solution is even better than the
one used for a commercial keyboard! :-)

The keyboard matrix of the microKONTROL keyboard is connected to two DOUT
shift registers and one DIN shiftregister.

It's organized the following way:

1st Socket with 10 pins:
Pin  1 of the keyboard row -> connected to 1st DOUT, Pin D7
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


This pinning leads to a certain "scrambling" of row and column events
which has to be considered when calculating the resulting key number.

This is done in the BUTTON_NotifyToggle() function.
First I set the DEBUG_VERBOSE_LEVEL to 2 in order to print the row/column
number on each key press, the results were:

-------------------------------------------------------------------------------
  // the make  contacts are at row 0, 2, 4, 6, 8, 10, 12, 14
  // the break contacts are at row 1, 3, 5, 7, 9, 11, 13, 15
-------------------------------------------------------------------------------

The "break contacts" don't lead to a note event, we only need them later
to calculate the delay:

-------------------------------------------------------------------------------
  u8 break_contact = (row & 1); // odd numbers
  // we ignore button changes on the break contacts:
  if( break_contact )
    return;
-------------------------------------------------------------------------------


Now we can determine the pin number and add/substract an offset to normalize
the first pin to 0. Finally an octave offset will be added to the note_number
to transpose the note:

-------------------------------------------------------------------------------
  // determine key number:
  int key = 8*(row / 2) + column;

  // check if key is assigned to an "break contact"
  u8 break_contact = (row & 1); // odd numbers

  // determine note number (here we could insert an octave shift)
  int note_number = key + 36;
-------------------------------------------------------------------------------

This code probably has to be adapted for other keyboards.

In addition, you may want to check the minimum and maximum delay to scale
the velocity between 1..127 - these values are defined at the header:

-------------------------------------------------------------------------------
#define INITIAL_KEYBOARD_DELAY_FASTEST 4
#define INITIAL_KEYBOARD_DELAY_SLOWEST 200
-------------------------------------------------------------------------------

It seems that these values will also work fine for a Fatar keyboard (as tested
by Robin some time ago)

Once the adaption has been finished, you can set DEBUG_VERBOSE_LEVEL to 1 in app.c
so that no debug messages are displayed anymore.


For debouncing the keys a note based locking mechanism has been implemented:
- whenever a key plays a Note On event, no additional Note On will be played
  until the "break contact" has been released
- whenever a key plays a Note Off event, no additional Note Off will be played
  until the "break contact" has been released

Enjoy your keyboard! :-)

===============================================================================

Please note: the MIDIbox KB application contains a more sophisticated
scanning routine which also considers different keyboard types (e.g. Fatar keyboards)

-> see http://www.ucapps.de/midibox_kb.html
Sources are located under $MIOS32_PATH/apps/controllers/midibox_kb_v1/src/keyboard.c

This scanning routine uses a special optimisation algorithm which allows to scan
a keyboard with 65 uS (as long as no key is pressed)

===============================================================================
