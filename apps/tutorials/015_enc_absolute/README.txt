$Id$

MIOS32 Tutorial #015: Sending absolute MIDI events with rotary encoders
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
   o at least one MBHP_DINX4 module
   o 1..64 rotary encoders

===============================================================================

In the previous tutorial lesson "014_enc_relative" we learnt how to generate 
relative MIDI events with rotary encoders. Now we want to generate
"absolute MIDI events" like analog pots.

This only requires to store the "virtual position" in a variable, which
is incremented/decremented on each encoder movement. This variable
can also be changed from an external device to update the "virtual
position". This is demonstrated in this tutorial as well.


The encoders are initialized the same way like in the previous tutorial,
we prepare up to 64 encoders.


In addition, we declare an array of bytes which contains the virtual
position:
-------------------------------------------------------------------------------
#define NUM_ENCODERS 64

u8 enc_virtual_pos[NUM_ENCODERS];
-------------------------------------------------------------------------------

All positions are set to 0 in APP_Init()


The APP_ENC_NotifyChange() is changed the following way:
-------------------------------------------------------------------------------
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
  // toggle Status LED on each AIN value change
  MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

  // increment to virtual position and ensure that the value is in range 0..127
  int value = enc_virtual_pos[encoder] + incrementer;
  if( value < 0 )
    value = 0;
  else if( value > 127 )
    value = 127;
  enc_virtual_pos[encoder] = value;

  // only send if value has changed
  if( enc_virtual_pos[encoder] != value ) {
    // store new value
    enc_virtual_pos[encoder] = value;

    // send event
    MIOS32_MIDI_SendCC(DEFAULT, Chn1, 0x10 + encoder, value);
  }
}
-------------------------------------------------------------------------------

Note that for temporal calculation, a signed integer is used.
This is an important detail, because the original datatype of
enc_virtual_pos is a u8 (unsigned char), which won't get negative.
Accordingly, we wouldn't be able to check for an underrun (value < 0)

The new value in the range of 0..127 will be written back into the
enc_virtual_pos array, so that the position is available again on
the next encoder movement.


A MIDI event will only be changed, if the virtual position has
been changed. It wouldn't change once it reached 0, and the encoder
is turned counter clockwise, or if it reached 127 and the encoder
is turned clockwise.


Hint: in order to speed up encoder movements, change the configuration
the following way in APP_Init()
-------------------------------------------------------------------------------
    // higher incrementer values on fast movements
    enc_config.cfg.speed = FAST;
    enc_config.cfg.speed_par = 2;
-------------------------------------------------------------------------------

You will still be able to change the virtual position precisely so long
the encoder is moved slowly. Once it is turned faster, the incrementer
value increases (usually by 2..5), so that it should be possible to
send values over the complete range with a single turn (without removing 
the fingers from the encoder).

===============================================================================
