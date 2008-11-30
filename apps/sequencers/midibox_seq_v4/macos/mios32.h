// $Id$
/*
 * Header file for MIOS32 compatibility layer of MacOS
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_H
#define _MIOS32_H

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// include local config file
// (see MIOS32_CONFIG.txt for available switches)
/////////////////////////////////////////////////////////////////////////////

#include "mios32_config.h"


/////////////////////////////////////////////////////////////////////////////
// include C headers
/////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// the default MIDI port for MIDI output
#ifndef MIOS32_MIDI_DEFAULT_PORT
#define MIOS32_MIDI_DEFAULT_PORT USB0
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// comaptible with stm32f10x_type.h
typedef signed long  s32;
typedef signed short s16;
typedef signed char  s8;

typedef signed long  const sc32;  /* Read Only */
typedef signed short const sc16;  /* Read Only */
typedef signed char  const sc8;   /* Read Only */

typedef volatile signed long  vs32;
typedef volatile signed short vs16;
typedef volatile signed char  vs8;

typedef volatile signed long  const vsc32;  /* Read Only */
typedef volatile signed short const vsc16;  /* Read Only */
typedef volatile signed char  const vsc8;   /* Read Only */

typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;

typedef unsigned long  const uc32;  /* Read Only */
typedef unsigned short const uc16;  /* Read Only */
typedef unsigned char  const uc8;   /* Read Only */

typedef volatile unsigned long  vu32;
typedef volatile unsigned short vu16;
typedef volatile unsigned char  vu8;

typedef volatile unsigned long  const vuc32;  /* Read Only */
typedef volatile unsigned short const vuc16;  /* Read Only */
typedef volatile unsigned char  const vuc8;   /* Read Only */

#define U8_MAX     ((u8)255)
#define S8_MAX     ((s8)127)
#define S8_MIN     ((s8)-128)
#define U16_MAX    ((u16)65535u)
#define S16_MAX    ((s16)32767)
#define S16_MIN    ((s16)-32768)
#define U32_MAX    ((u32)4294967295uL)
#define S32_MAX    ((s32)2147483647)
#define S32_MIN    ((s32)-2147483648)


typedef enum {
  DEFAULT = 0x00,

  USB0 = 0x10,
  USB1 = 0x11,
  USB2 = 0x12,
  USB3 = 0x13,
  USB4 = 0x14,
  USB5 = 0x15,
  USB6 = 0x16,
  USB7 = 0x17,


  UART0 = 0x20,
  UART1 = 0x21,

  IIC0 = 0x30,
  IIC1 = 0x31,
  IIC2 = 0x32,
  IIC3 = 0x33,
  IIC4 = 0x34,
  IIC5 = 0x35,
  IIC6 = 0x36,
  IIC7 = 0x37
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

extern s32 MIOS32_LCD_Init(u32 mode);
extern s32 MIOS32_LCD_DeviceSet(u8 device);
extern u8  MIOS32_LCD_DeviceGet(void);
extern s32 MIOS32_LCD_CursorSet(u16 column, u16 line);
extern s32 MIOS32_LCD_Clear(void);
extern s32 MIOS32_LCD_PrintChar(char c);
extern s32 MIOS32_LCD_PrintString(char *str);
extern s32 MIOS32_LCD_PrintFormattedString(char *format, ...);

extern s32 MIOS32_LCD_SpecialCharInit(u8 num, u8 table[8]);
extern s32 MIOS32_LCD_SpecialCharsInit(u8 table[64]);

extern s32 MIOS32_COM_SendChar(u8 port, char c);

extern s32 MIOS32_DOUT_PinSet(u32 pin, u32 value);
extern s32 MIOS32_DOUT_SRSet(u32 sr, u8 value);

extern s32 MIOS32_BOARD_LED_Init(u32 leds);
extern s32 MIOS32_BOARD_LED_Set(u32 leds, u32 value);
extern u32 MIOS32_BOARD_LED_Get(void);

extern s32 MIOS32_MIDI_CheckAvailable(mios32_midi_port_t port);
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

#ifdef __cplusplus
}
#endif

#endif /* _MIOS32_H */
