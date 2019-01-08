$Id$

MIOS32 Tutorial #004: Sending MIDI
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

===============================================================================

You already know MIOS32_MIDI_SendPackage() from the first tutorial lesson 
(001_forwarding_midi). This function allows you to send a MIDI package to 
a given MIDI port.

Since the creation of a MIDI package takes several lines of code in C, MIOS32
provides some help function for more comfortable (and better readable) usage.

These are:

s32 MIOS32_MIDI_SendEvent(mios32_midi_port_t port, u8 evnt0, u8 evnt1, u8 evnt2);
s32 MIOS32_MIDI_SendNoteOff(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 vel);
s32 MIOS32_MIDI_SendNoteOn(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 vel);
s32 MIOS32_MIDI_SendPolyPressure(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 val);
s32 MIOS32_MIDI_SendCC(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 cc_number, u8 val);
s32 MIOS32_MIDI_SendProgramChange(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 prg);
s32 MIOS32_MIDI_SendAftertouch(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 val);
s32 MIOS32_MIDI_SendPitchBend(mios32_midi_port_t port, mios32_midi_chn_t chn, u16 val);

s32 MIOS32_MIDI_SendSysEx(mios32_midi_port_t port, u8 *stream, u32 count);
s32 MIOS32_MIDI_SendMTC(mios32_midi_port_t port, u8 val);
s32 MIOS32_MIDI_SendSongPosition(mios32_midi_port_t port, u16 val);
s32 MIOS32_MIDI_SendSongSelect(mios32_midi_port_t port, u8 val);
s32 MIOS32_MIDI_SendTuneRequest(mios32_midi_port_t port);
s32 MIOS32_MIDI_SendClock(mios32_midi_port_t port);
s32 MIOS32_MIDI_SendTick(mios32_midi_port_t port);
s32 MIOS32_MIDI_SendStart(mios32_midi_port_t port);
s32 MIOS32_MIDI_SendStop(mios32_midi_port_t port);
s32 MIOS32_MIDI_SendContinue(mios32_midi_port_t port);
s32 MIOS32_MIDI_SendActiveSense(mios32_midi_port_t port);
s32 MIOS32_MIDI_SendReset(mios32_midi_port_t port);


So, if the application should send a CC event, you could either create
the package "from scratch" with:
-------------------------------------------------------------------------------
  mios32_midi_package_t package;

  package.type  = CC;
  package.event = CC;
  package.chn = Chn1;
  package.cc_number = 7; // CC#7 controls volume
  package.value = 127; // set maximum volume (=127)
  MIOS32_MIDI_SendPackage(DEFAULT, package);
-------------------------------------------------------------------------------

Or you could simply write:
-------------------------------------------------------------------------------
  MIOS32_MIDI_SendCC(DEFAULT, Chn1, 7, 127); // CC#7 set to maximum volume
-------------------------------------------------------------------------------


The example in app.c demonstrates the usage: whenever a Note On Event has been
received on Channel #1, we will send the velocity value as a CC#1 (Modulation
Wheel) event over MIDI Channel #1


Hint: MIDI Notes can be sent with the virtual keyboard of MIOS Studio, or
alternatively by connecting a MIDI keyboard to MIDI IN1 or IN2 of your
core module.
Outgoing MIDI events can be displayed with the "MIDI IN" monitor of MIOS Studio

===============================================================================
