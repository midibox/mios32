$Id$

MIOS32 Tutorial #013: Controlling Motorfaders
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
   o up to 16 motorfaders
   o one (for up to 8 faders) or two (for up to 16 faders) MBHP_MF modules

===============================================================================

MIOS32 allows to control up to 16 motorfaders as descriped under
   http://www.ucapps.de/mbhp_mf.html
Please read these background infos first.


In this tutorial, we will
   - send a PitchBend event whenever a fader has been moved
   - move all faders whenever the first fader is moved manually
   - allow to move motorfaders via incoming PitchBend events


Motorfaders are enabled in mios32_config.h
Here we also set the number of scanned AIN channels (must match with number
of motorfaders) and the AIN deadband value.


In distance to MIOS8, MIOS32 allows to set individual deadband and PWM
values for each fader. This is done in APP_Init():

-------------------------------------------------------------------------------
  // configure motorfaders
  // see http://www.ucapps.de/mbhp_mf.html for details about the parameters
  int mf;
  for(mf=1; mf<MIOS32_MF_NUM; ++mf) {
    mios32_mf_config_t mf_config = MIOS32_MF_ConfigGet(mf);
    mf_config.cfg.deadband = MIOS32_AIN_DEADBAND;
    mf_config.cfg.pwm_period = 3;
    mf_config.cfg.pwm_duty_cycle_up = 1;
    mf_config.cfg.pwm_duty_cycle_down = 1;
    MIOS32_MF_ConfigSet(mf, mf_config);
  }
-------------------------------------------------------------------------------

Here, all faders get the same parameter values, but for fine calibration it would
be possible to read individual values from a table (or to send them from
an external host).


We want to move the faders with PitchBend events sent over different MIDI
channels. Each channel controls an individual fader:
-------------------------------------------------------------------------------
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // move motorfader on incoming pitchbend events
  if( midi_package.type == PitchBend ) {
    int mf = midi_package.chn;
    u16 pitchbend_value_14bit = (midi_package.evnt2 << 7) | (midi_package.evnt1 & 0x7f);
    int mf_pos_12bit = pitchbend_value_14bit >> 2;
    MIOS32_MF_FaderMove(mf, mf_pos_12bit);
  }
}
-------------------------------------------------------------------------------

PitchBend events contain 14bit values, whereas motorfaders are moved with
12bit resolution. Accordingly, we have to convert the 14bit value by dividing
it by 4 (or right-shifting it two times, as it has been done above).


Inside the APP_AIN_NotifyChange() hook send a PitchBend value (note: 12bit
converted to 14 bit), and we move fader 1..MIOS32_MF_NUM-1 to the same
position like the first fader whenever it has been moved.

This is just for demonstration and testing purposes: you can check the connectivity
and calibration of all faders (minus the first one which is used for controlling).

-------------------------------------------------------------------------------
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  // convert 12bit value to 14bit value
  u16 value_14bit = pin_value << 2;

  // send Pitch Bender event
  MIOS32_MIDI_SendPitchBend(DEFAULT, pin, value_14bit);

  // if first fader has been moved, move the remaining faders to the same position
  if( pin == 0 ) {
    int mf;
    for(mf=1; mf<MIOS32_MF_NUM; ++mf)
      MIOS32_MF_FaderMove(mf, pin_value);
  }
}
-------------------------------------------------------------------------------


Exercise
--------

Allow to change motorfader calibration values via CC events


===============================================================================
