$Id$

MIOS32 Tutorial #014: Sending relative MIDI events with rotary encoders
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
   o at least one MBHP_DINX4 module
   o 1..64 rotary encoders

===============================================================================

Rotary encoders are a nice alternative solution to analog pots, as they
are behaving like "endless knobs" without absolute position. This is
especially useful whenever parameter values are changed internally (e.g. 
on a "patch change") or from an external host (e.g. switching to another 
virtual synth).

Encoders output a quadrature code which allows to determine, into
which direction (and how fast) the encoder is turned.

Each encoder has two "position" pins, which are connected to two inputs
of a MBHP_DINX4 module, and a common pin which is connected to ground.

MIOS32 already provides a quadrature decoder for different encoder
types, and scans for changes in background, so that the application
is notified whenever an encoder has been moved.

This is done via the APP_ENC_NotifyChange() hook.


Each encoder can be configured separately. Usually we configure the
type and speed mode - see also documentation under:
  http://www.midibox.org/mios32/manual/group___m_i_o_s32___e_n_c.html


Following code in APP_Init() configures 64 encoders. This is the maximum
configuration for 128 digital pins, provided by four MBHP_DINX4 modules:
-------------------------------------------------------------------------------
  int enc;
  for(enc=0; enc<NUM_ENCODERS; ++enc) {
    u8 pin_sr = enc >> 2; // each DIN SR has 4 encoders connected
    u8 pin_pos = (enc & 0x3) << 1; // Pin position of first ENC channel: either 0, 2, 4 or 6

    mios32_enc_config_t enc_config = MIOS32_ENC_ConfigGet(enc);
    enc_config.cfg.type = DETENTED2; // see mios32_enc.h for available types
    enc_config.cfg.sr = pin_sr;
    enc_config.cfg.pos = pin_pos;
    enc_config.cfg.speed = NORMAL;
    enc_config.cfg.speed_par = 0;
    MIOS32_ENC_ConfigSet(enc, enc_config);
  }
-------------------------------------------------------------------------------

Note that SR and pin positions are calculated depending on the enc number.

Encoders have to be connected to:
  o First DIN SR, Pin D0/D1
  o First DIN SR, Pin D2/D3
  o First DIN SR, Pin D4/D5
  o First DIN SR, Pin D6/D7
  o Second DIN SR, Pin D0/D1
  o Second DIN SR, Pin D2/D3
  ...
  o Last DIN SR, Pin D6/D7



On increments/decrements, APP_ENC_NotifyChange() will notifies about the
moved encoder and the so called "incrementer", which is positive on
clockwise turns, and negative on anti-clockwise turns.
Now we can generate a CC Event based on this value:
-------------------------------------------------------------------------------
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
  // toggle Status LED on each AIN value change
  MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

  // determine relative value: 64 +/- <incrementer>
  int value = 64 + incrementer;

  // ensure that value is in range of 0..127
  if( value < 0 )
    value = 0;
  else if( value > 127 )
    value = 127;

  // send event
  MIOS32_MIDI_SendCC(DEFAULT, Chn1, 0x10 + encoder, value);
}
-------------------------------------------------------------------------------


After code upload, move the encoders and check the generated MIDI events
with the MIDI monitor of MIOS Studio.

===============================================================================
