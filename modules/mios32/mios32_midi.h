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


// aliases for the MIOS32_MIDI_SendEvent function for more comfortable usage
#define MIOS32_MIDI_SendNoteOff(port, chn, note, vel)          MIOS32_MIDI_SendEvent(port, 0x80 | (chn), note, vel)
#define MIOS32_MIDI_SendNoteOn(port, chn, note, vel)           MIOS32_MIDI_SendEvent(port, 0x90 | (chn), note, vel)
#define MIOS32_MIDI_SendPolyPressure(port, chn, note, val)     MIOS32_MIDI_SendEvent(port, 0xa0 | (chn), note, val)
#define MIOS32_MIDI_SendCC(port, chn, cc, val)                 MIOS32_MIDI_SendEvent(port, 0xb0 | (chn), cc,   val)
#define MIOS32_MIDI_SendProgramChange(port, chn, prg)          MIOS32_MIDI_SendEvent(port, 0xc0 | (chn), prg,  0x00)
#define MIOS32_MIDI_SendAftertouch(port, chn, val)             MIOS32_MIDI_SendEvent(port, 0xd0 | (chn), val,  0x00)
#define MIOS32_MIDI_SendPitchBend(port, chn, val)              MIOS32_MIDI_SendEvent(port, 0xe0 | (chn), val & 0x7f, val >> 7)

#define MIOS32_MIDI_SendMTC(port, val)                         MIOS32_MIDI_SendSpecialEvent(port, 0x2, 0xf1, val, 0x00)
#define MIOS32_MIDI_SendSongPosition(port, val)                MIOS32_MIDI_SendSpecialEvent(port, 0x3, 0xf2, val & 0x7f, val >> 7)
#define MIOS32_MIDI_SendSongSelect(port, val)                  MIOS32_MIDI_SendSpecialEvent(port, 0x2, 0xf3, val, 0x00)
#define MIOS32_MIDI_SendTuneRequest(port)                      MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xf6, 0x00, 0x00)
#define MIOS32_MIDI_SendClock(port)                            MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xf8, 0x00, 0x00)
#define MIOS32_MIDI_SendTick(port)                             MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xf9, 0x00, 0x00)
#define MIOS32_MIDI_SendStart(port)                            MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfa, 0x00, 0x00)
#define MIOS32_MIDI_SendStop(port)                             MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfb, 0x00, 0x00)
#define MIOS32_MIDI_SendContinue(port)                         MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfc, 0x00, 0x00)
#define MIOS32_MIDI_SendActiveSense(port)                      MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfe, 0x00, 0x00)
#define MIOS32_MIDI_SendReset(port)                            MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xff, 0x00, 0x00)


// MIDI port IDs
#define MIOS32_MIDI_PORT_USB0         0x00
#define MIOS32_MIDI_PORT_USB1         0x01
#define MIOS32_MIDI_PORT_USB2         0x02
#define MIOS32_MIDI_PORT_USB3         0x03
#define MIOS32_MIDI_PORT_USB4         0x04
#define MIOS32_MIDI_PORT_USB5         0x05
#define MIOS32_MIDI_PORT_USB6         0x06
#define MIOS32_MIDI_PORT_USB7         0x07
#define MIOS32_MIDI_PORT_USB8         0x08
#define MIOS32_MIDI_PORT_USB9         0x09
#define MIOS32_MIDI_PORT_USB10        0x0a
#define MIOS32_MIDI_PORT_USB11        0x0b
#define MIOS32_MIDI_PORT_USB12        0x0c
#define MIOS32_MIDI_PORT_USB13        0x0d
#define MIOS32_MIDI_PORT_USB14        0x0e
#define MIOS32_MIDI_PORT_USB15        0x0f

#define MIOS32_MIDI_PORT_UART0        0x10
#define MIOS32_MIDI_PORT_UART1        0x11
#define MIOS32_MIDI_PORT_UART2        0x12
#define MIOS32_MIDI_PORT_UART3        0x13
#define MIOS32_MIDI_PORT_UART4        0x14
#define MIOS32_MIDI_PORT_UART5        0x15
#define MIOS32_MIDI_PORT_UART6        0x16
#define MIOS32_MIDI_PORT_UART7        0x17
#define MIOS32_MIDI_PORT_UART8        0x18
#define MIOS32_MIDI_PORT_UART9        0x19
#define MIOS32_MIDI_PORT_UART10       0x1a
#define MIOS32_MIDI_PORT_UART11       0x1b
#define MIOS32_MIDI_PORT_UART12       0x1c
#define MIOS32_MIDI_PORT_UART13       0x1d
#define MIOS32_MIDI_PORT_UART14       0x1e
#define MIOS32_MIDI_PORT_UART15       0x1f

#define MIOS32_MIDI_PORT_IIC0         0x20
#define MIOS32_MIDI_PORT_IIC1         0x21
#define MIOS32_MIDI_PORT_IIC2         0x22
#define MIOS32_MIDI_PORT_IIC3         0x23
#define MIOS32_MIDI_PORT_IIC4         0x24
#define MIOS32_MIDI_PORT_IIC5         0x25
#define MIOS32_MIDI_PORT_IIC6         0x26
#define MIOS32_MIDI_PORT_IIC7         0x27
#define MIOS32_MIDI_PORT_IIC8         0x28
#define MIOS32_MIDI_PORT_IIC9         0x29
#define MIOS32_MIDI_PORT_IIC10        0x2a
#define MIOS32_MIDI_PORT_IIC11        0x2b
#define MIOS32_MIDI_PORT_IIC12        0x2c
#define MIOS32_MIDI_PORT_IIC13        0x2d
#define MIOS32_MIDI_PORT_IIC14        0x2e
#define MIOS32_MIDI_PORT_IIC15        0x2f


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

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
} mios32_midi_package_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_MIDI_Init(u32 mode);

extern s32 MIOS32_MIDI_SendPackage(u8 port, mios32_midi_package_t package);
extern s32 MIOS32_MIDI_SendEvent(u8 port, u8 evnt0, u8 evnt1, u8 evnt2);
extern s32 MIOS32_MIDI_SendSysEx(u8 port, u8 *stream, u32 count);

extern s32 MIOS32_MIDI_Receive_Handler(void *callback_event, void *callback_sysex);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////



#endif /* _MIOS32_MIDI_H */
