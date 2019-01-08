$Id$

BLM_SCALAR Firmware for MIOS32
===============================================================================
Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o one MBHP_CORE_STM32 module
   o a MBHP_BLM_SCALAR system with up to 320 buttons and Duo-Colour -LEDs
   o optional: up to 8 pots or faders

===============================================================================


!! IMPORTANT !! IMPORTANT !! IMPORTANT !! IMPORTANT !! IMPORTANT !!

Since this application supports faders/pots connected to J5,
all unusused analog inputs (J5A.A0..J5B.A7) have to be connected to ground!

If analog inputs are open (neither connected to a fader/pot nor to ground),
random CC events will be generated!

!! IMPORTANT !! IMPORTANT !! IMPORTANT !! IMPORTANT !! IMPORTANT !!



MIDI Protocol
~~~~~~~~~~~~~

  All numbers in hexadecimal format!
  <row>: 0..F (coded into MIDI channel)
  <column>: 00..0F (Note Number)
  <button-state>: 00: depressed, 7F: pressed
  <colour>: 00: off, 20: green colour only, 40: red colour only, 7F: all colours
  <fader>: 00..0F (coded into MIDI channel)


Sent fader events:
+---------------------------------------+-----------------------------------+
| up to 8 faders connected to J5        | B<fader> 01 <value>               |
+---------------------------------------+-----------------------------------+


Sent button events:
+---------------------------------------+-----------------------------------+
| BLM16x16 Buttons                      | 9<row> <column> <button-state>    |
| Extra column buttons                  | 9<row> <40+column> <button-state> |
| Extra row buttons                     | 90 <60+column> <button-state>     |
| Additional extra buttons (e.g. Shift) | 9F <60+function> <button-state>   |
+---------------------------------------+-----------------------------------+


Received LED events (single access):
+---------------------------------------+-----------------------------+
| BLM16x16 LEDs                         | 9<row> <column> <colour>    |
| Extra column LEDs                     | 9<row> <40+column> <colour> |
| Extra row LEDs                        | 90 <60+column> <colour>     |
| Additional extra LEDs (e.g. Shift)    | 9F <60+function> <colour>   |
+---------------------------------------+-----------------------------+


BLM16x16 optimized LED pattern transfer (prefered usage):
+-----------------------------------------------+---------------------+
| column LEDs 0..6, green colour, 8th LED off   | B<row> 10 <pattern> |
| column LEDs 0..6, green colour, 8th LED on    | B<row> 11 <pattern> |
| column LEDs 8..14, green colour, 16th LED off | B<row> 12 <pattern> |
| column LEDs 8..14, green colour, 16th LED on  | B<row> 13 <pattern> |
+-----------------------------------------------+---------------------+
| column LEDs 0..6, red colour, 8th LED off     | B<row> 20 <pattern> |
| column LEDs 0..6, red colour, 8th LED on      | B<row> 21 <pattern> |
| column LEDs 8..14, red colour, 16th LED off   | B<row> 22 <pattern> |
| column LEDs 8..14, red colour, 16th LED on    | B<row> 23 <pattern> |
+-----------------------------------------------+---------------------+


BLM16x16 optimized LED pattern transfer with 90° rotated view
(rows and columns swapped, LSB starts at top left edge!)
+--------------------------------------------+------------------------+
| row LEDs 0..6, green colour, 8th LED off   | B<column> 18 <pattern> |
| row LEDs 0..6, green colour, 8th LED on    | B<column> 19 <pattern> |
| row LEDs 8..14, green colour, 16th LED off | B<column> 1A <pattern> |
| row LEDs 8..14, green colour, 16th LED on  | B<column> 1B <pattern> |
+--------------------------------------------+------------------------+
| row LEDs 0..6, red colour, 9th LED off     | B<column> 28 <pattern> |
| row LEDs 0..6, red colour, 9th LED on      | B<column> 29 <pattern> |
| row LEDs 8..14, red colour, 16th LED off   | B<column> 2A <pattern> |
| row LEDs 8..14, red colour, 16th LED on    | B<column> 2B <pattern> |
+--------------------------------------------+------------------------+


Extra Column optimized LED pattern transfer (prefered usage):
NOTE: in distance to single LED access, we always sent over the same channel!
+--------------------------------------------+-----------------+
| row LEDs 0..6, green colour, 8th LED off   | B0 40 <pattern> |
| row LEDs 0..6, green colour, 8th LED on    | B0 41 <pattern> |
| row LEDs 8..14, green colour, 16th LED off | B0 42 <pattern> |
| row LEDs 8..14, green colour, 16th LED on  | B0 43 <pattern> |
+--------------------------------------------+-----------------+
| row LEDs 0..6, red colour, 8th LED off     | B0 48 <pattern> |
| row LEDs 0..6, red colour, 8th LED on      | B0 49 <pattern> |
| row LEDs 8..14, red colour, 16th LED off   | B0 4A <pattern> |
| row LEDs 8..14, red colour, 16th LED on    | B0 4B <pattern> |
+--------------------------------------------+-----------------+


Extra Row optimized LED pattern transfer (prefered usage):
+-----------------------------------------------+-----------------+
| column LEDs 0..6, green colour, 8th LED off   | B0 60 <pattern> |
| column LEDs 0..6, green colour, 8th LED on    | B0 61 <pattern> |
| column LEDs 8..14, green colour, 16th LED off | B0 62 <pattern> |
| column LEDs 8..14, green colour, 16th LED on  | B0 63 <pattern> |
+-----------------------------------------------+-----------------+
| column LEDs 0..6, red colour, 8th LED off     | B0 68 <pattern> |
| column LEDs 0..6, red colour, 8th LED on      | B0 69 <pattern> |
| column LEDs 8..14, red colour, 16th LED off   | B0 6A <pattern> |
| column LEDs 8..14, red colour, 16th LED on    | B0 6B <pattern> |
+-----------------------------------------------+-----------------+


Additional extra LEDs, optimized LED pattern transfer (prefered usage):
+-----------------------------------------------+-----------------+
| extra  LEDs 0..6, green colour, 8th LED off   | BF 60 <pattern> |
| extra  LEDs 0..6, green colour, 8th LED on    | BF 61 <pattern> |
+-----------------------------------------------+-----------------+
| extra LEDs 0..6, red colour, 8th LED off      | BF 68 <pattern> |
| extra LEDs 0..6, red colour, 8th LED on       | BF 69 <pattern> |
+-----------------------------------------------+-----------------+



SysEx Commands
~~~~~~~~~~~~~~

  o F0 00 00 7E 4E <device-id> 00 F7
    Request Layout Informations

    Returns:
    F0 00 00 7E 4E <device-id> 01 <num-rows> <num-columns> <num-colours>
       <num-extra-rows> <num-extra-columns> <num-extra-buttons> F7

  o F0 00 00 7E 4E <device-id> 01 F7
    ignored if received

  o F0 00 00 7E 4E <device-id> 0E F7
    ignored if received

  o F0 00 00 7E 4E <device-id> 0F F7
    Ping: returns F0 00 00 7E 4E <device-id> 0F 00 F7


Communication
~~~~~~~~~~~~~
   - host requests layout informations with F0 00 00 7E 4E 00 00 F7
   - this application replies with F0 00 00 7E 4E 00 01 ... F7 to send back the
     available number of buttons and LEDs
   - host updates all LEDs by sending CC events (Bn ..)

   - during runtime host updates LEDs that have changed whenever required

   - if the application doesn't receive LED pattern updates within 5 seconds,
     the layout information SysEx string will be sent again.
     This keeps the BLM up-to-date if it temporary has been disconnected from
     the host, and allows to implement an additional timeout mechanism at the 
     host site to check the bidirectional BLM connection.

   - in any case, each 5 seconds a Sysex string will be sent. Either the
     layout info, or a ping.
     The host is allowed to stop sending data if no SysEx strig has been received
     within 15 seconds.


As long as no MIDI data has been received after startup, the BLM is in test mode.
The buttons cycle the colour of the corresponding LED:
   - first button press: green colour
   - second button press: red colour
   - third button press: both colours
   - fourth button press: all LEDs off
   - period

===============================================================================
