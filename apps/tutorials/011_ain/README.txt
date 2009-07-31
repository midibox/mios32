$Id$

MIOS32 Tutorial #011: Scanning 12 analog pots
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
   o up to 12 10k pots (alternatively 1k or 4.7k)

===============================================================================

STM32F103RE provides 16 analog input channels, where 12 of them are directly
accessible via J5A, J5B and J5C. The 4 remaining channels are available
at J16, but this port is normaly used for accessing a SD Card, an Ethernet
Interface (MBHP_ETH) or similar serial devices.

The provided resultion is 12bit

Similar to DINs/DOUTs, analog inputs (AINs) are scanned by MIOS32 in background,
and a DMA is used as well to safe CPU time.

AIN inputs have to be explictely enabled (and configured) in the
mios32_config.h file.

Following example uses the non-multiplexed variant, where 12 analog pots
are directly connected to J5A.A0..3, J5B.A4..7 and J5C.A8..A11

Unusued analog inputs should be connected to ground (just stuff jumpers)
to avoid random values.
Alternatively, adapt the MIOS32_AIN_CHANNEL_MASK in mios32_config.h
before compiling the source code.


Reacting on AIN pin changes is so simple like known from DIN pin changes:
-------------------------------------------------------------------------------
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  // toggle Status LED on each AIN value change
  MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

  // convert 12bit value to 7bit value
  u8 value_7bit = pin_value >> 5;

  // send MIDI event
  MIOS32_MIDI_SendCC(DEFAULT, Chn1, pin + 0x10, value_7bit);
}
-------------------------------------------------------------------------------

Since CC events are 7bit values, we need to convert the pin_value
(12bit range: 0..1023) to a 7bit value (0..127) by right-shifting
it 5 times.

Max value 4095 -> 2047 -> 1023 -> 511 -> 255 -> 127
          12bit   11bit   10bit  9bit    8bit   7bit
                   1.       2.     3.     4.     5. shift

We could also divide it by 32 (2^5), which has the same effect.

Finally the MIDI event is generated with the MIOS32_MIDI_SendCC() function.


Counter example: if a PitchBender value should be generated (14 bit
resolution), we would have to increase the value by left-shifting
it two times:
-------------------------------------------------------------------------------
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  // toggle Status LED on each AIN value change
  MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

  // convert 12bit value to 14bit value
  u16 value_14bit = pin_value << 2;

  // send Pitch Bender event
  MIOS32_MIDI_SendPitchBend(DEFAULT, Chn1, value_14bit);
}
-------------------------------------------------------------------------------

Max value 4095 -> 8191 -> 16383
          12bit   13bit   14bit
                   1.       2. shift


MIOS32_AIN_DEADBAND - sounds dangerous?

Don't worry! The deadband (configured in mios32_config.h) defines a 
range over which value changes won't trigger the APP_AIN_NotifyChange()
hook.
It's typically set to (2^(12-desired_resolution)-1)
E.g., for a resolution of 7 bit, it's set to (2^(12-7)-1) = (2^5 - 1) = 31

The usage of a deadband has two advantages:

1) value changes which are not relevant for the application are not notified
to APP_AIN_NotifyChange(). E.g., if the application only works with 7bit
values, a conversion value change from 4095 to 4090 doesn't really matter,
as (4095>>5) gives 127, and (4090>>5) gives 127 as well!

E.g., in the tutorial example, this avoids to trigger CCs with the same
value multiple times when a pot is moved slowly.

2) the deadband helps to filter jitter noise, which will be especially
noticed with higher resolutions. In distance to the PIC (which was used
for the MIOS8 based platform), STM32 ADCs are working in a voltage range
of 0..3.3V, which makes it less tolerant against noise in the mV range.

For comparison:
  PIC ADC is working with 10 bit resolution at 5V
    -> 1 LSB difference is 5V / 2^10 = ca. 5 mV
  STM32 ACDs are working with 12 bit resolution at 3.3V
    -> 1 LSB difference is 3.3V / 2^12 = ca. 0.8 mV (!!!)

===============================================================================
