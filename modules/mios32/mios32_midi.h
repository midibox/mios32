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
    unsigned ptype:8;
    unsigned evnt0:8;
    unsigned evnt1:8;
    unsigned evnt2:8;
  };
} midi_package_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_MIDI_Init(u32 mode);

extern s32 MIOS32_MIDI_PackageSend(u8 port, midi_package_t package);
extern s32 MIOS32_MIDI_PackageReceive_Handler(void *callback);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////



#endif /* _MIOS32_MIDI_H */
