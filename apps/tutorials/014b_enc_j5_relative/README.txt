$Id$

MIOS32 Tutorial #014b: Sending relative MIDI events with rotary encoders connected to J5
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
   o 1..6 rotary encoders

===============================================================================

This is a variant of tutorial #014 that scans encoders connected to J5 pins
so that no shift registers are required.

For general informations about rotary encoders are explained in the README.txt
file of tutorial #014


All J5 pins are configured as inputs with interal pull-ups in APP_Init()
-------------------------------------------------------------------------------
  // initialize all pins of J5A, J5B and J5C as inputs with internal Pull-Up
  int pin;
  for(pin=0; pin<12; ++pin)
    MIOS32_BOARD_J5_PinInit(pin, MIOS32_BOARD_PIN_MODE_INPUT_PU);
-------------------------------------------------------------------------------

The encoder configuration sets enc_config.cfg.sr to 0 (!) to enable application
control of the encoder states:
-------------------------------------------------------------------------------
  int enc;
  for(enc=0; enc<NUM_ENCODERS; ++enc) {
    mios32_enc_config_t enc_config = MIOS32_ENC_ConfigGet(enc);
    enc_config.cfg.type = DETENTED2; // see mios32_enc.h for available types
    enc_config.cfg.sr = 0; // must be 0 if controlled from application
    enc_config.cfg.pos = 0; // doesn't matter if controlled from application
    enc_config.cfg.speed = NORMAL;
    enc_config.cfg.speed_par = 0;
    MIOS32_ENC_ConfigSet(enc, enc_config);
  }
-------------------------------------------------------------------------------


The J5 pins should be scanned periodically in the APP_SRIO_ServicePrepare()
hook as shown in this example:
-------------------------------------------------------------------------------
void APP_SRIO_ServicePrepare(void)
{
  // get current J5 value and update all encoder states
  int enc;
  u16 state = MIOS32_BOARD_J5_Get();
  for(enc=0; enc<NUM_ENCODERS; ++enc) {
    MIOS32_ENC_StateSet(enc, state & 3); // two pins
    state >>= 2; // shift to next two pins
  }
}
-------------------------------------------------------------------------------


Note: of course, you can mix encoders and buttons at J5, and encoders can
be connected to any J5 pins. It's totally under your control.

E.g., if only a single encoder is connected to J5A:A2 and A3, change:


-------------------------------------------------------------------------------
// only a single encoder is used
#define NUM_ENCODERS 1
-------------------------------------------------------------------------------

And the content of APP_SRIO_ServicePrepare():
-------------------------------------------------------------------------------
void APP_SRIO_ServicePrepare(void)
{
  u16 state = MIOS32_BOARD_J5_Get();
  MIOS32_ENC_StateSet(enc, (state >> 2) & 3);
}
-------------------------------------------------------------------------------


After code upload, move the encoders and check the generated MIDI events
with the MIDI monitor of MIOS Studio.

===============================================================================
