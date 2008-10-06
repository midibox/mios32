// $Id$
/*
 * Header file for MIDI layer
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_MIDI_H
#define _MIOS32_MIDI_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


typedef enum {
  USB0 = 0x00,
  USB1 = 0x01,
  USB2 = 0x02,
  USB3 = 0x03,
  USB4 = 0x04,
  USB5 = 0x05,
  USB6 = 0x06,
  USB7 = 0x07,


  UART0 = 0x10,
  UART1 = 0x11,

  IIC0 = 0x20,
  IIC1 = 0x21,
  IIC2 = 0x22,
  IIC3 = 0x23
} mios32_midi_port_t;


typedef enum {
  NoteOff       = 0x8,
  NoteOn        = 0x9,
  PolyPressure  = 0xa,
  CC            = 0xb,
  ProgramChange = 0xc,
  Aftertouch    = 0xd,
  PitchBend     = 0xe
} mios32_midi_event_t;


typedef enum {
  Chn1,
  Chn2,
  Chn3,
  Chn4,
  Chn5,
  Chn6,
  Chn7,
  Chn8,
  Chn9,
  Chn10,
  Chn11,
  Chn12,
  Chn13,
  Chn14,
  Chn15,
  Chn16
} mios32_midi_chn_t;


typedef union {
  struct {
    unsigned ALL:32;
  };
  struct {
    unsigned type:8;
    unsigned evnt0:8;
    unsigned evnt1:8;
    unsigned evnt2:8;
  };
  struct {
    unsigned cin:4;
    unsigned cable:4;
    mios32_midi_chn_t chn:4;
    mios32_midi_event_t event:4;
    unsigned value1:8;
    unsigned value2:8;
  };
  struct {
    unsigned cin:4;
    unsigned cable:4;
    mios32_midi_chn_t chn:4;
    mios32_midi_event_t event:4;
    unsigned note:8;
    unsigned velocity:8;
  };
  struct {
    unsigned cin:4;
    unsigned cable:4;
    mios32_midi_chn_t chn:4;
    mios32_midi_event_t event:4;
    unsigned cc_number:8;
    unsigned value:8;
  };
  struct {
    unsigned cin:4;
    unsigned cable:4;
    mios32_midi_chn_t chn:4;
    mios32_midi_event_t event:4;
    unsigned program_change:8;
    unsigned dummy:8;
  };
} mios32_midi_package_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_MIDI_Init(u32 mode);

extern s32 MIOS32_MIDI_SendPackage(mios32_midi_port_t port, mios32_midi_package_t package);

extern s32 MIOS32_MIDI_SendEvent(mios32_midi_port_t port, u8 evnt0, u8 evnt1, u8 evnt2);
extern s32 MIOS32_MIDI_SendNoteOff(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 vel);
extern s32 MIOS32_MIDI_SendNoteOn(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 vel);
extern s32 MIOS32_MIDI_SendPolyPressure(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 val);
extern s32 MIOS32_MIDI_SendPolyPressure(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 val);
extern s32 MIOS32_MIDI_SendProgramChange(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 prg);
extern s32 MIOS32_MIDI_SendAftertouch(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 val);
extern s32 MIOS32_MIDI_SendPitchBend(mios32_midi_port_t port, mios32_midi_chn_t chn, u16 val);

extern s32 MIOS32_MIDI_SendSysEx(mios32_midi_port_t port, u8 *stream, u32 count);
extern s32 MIOS32_MIDI_SendMTC(mios32_midi_port_t port, u8 val);
extern s32 MIOS32_MIDI_SendSongPosition(mios32_midi_port_t port, u16 val);
extern s32 MIOS32_MIDI_SendSongSelect(mios32_midi_port_t port, u8 val);
extern s32 MIOS32_MIDI_SendTuneRequest(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_SendClock(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_SendTick(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_SendStart(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_SendStop(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_SendContinue(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_SendActiveSense(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_SendReset(mios32_midi_port_t port);


extern s32 MIOS32_MIDI_Receive_Handler(void *callback_event, void *callback_sysex);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////



#endif /* _MIOS32_MIDI_H */
