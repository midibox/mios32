// $Id$
/*
 * MIDI Port functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIDI_PORT_H
#define _MIDI_PORT_H

#include <mios32.h>

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////

// number of IN ports (can be overruled from mios32_config.h)
#ifndef MIDI_PORT_NUM_IN_PORTS_USB
#define MIDI_PORT_NUM_IN_PORTS_USB MIOS32_USB_MIDI_NUM_PORTS
#endif

#ifndef MIDI_PORT_NUM_IN_PORTS_UART
#define MIDI_PORT_NUM_IN_PORTS_UART MIOS32_UART_NUM
#endif

#ifndef MIDI_PORT_NUM_IN_PORTS_OSC
#define MIDI_PORT_NUM_IN_PORTS_OSC 4
#endif

#ifndef MIDI_PORT_NUM_IN_PORTS_SPIM
#define MIDI_PORT_NUM_IN_PORTS_SPIM MIOS32_SPI_MIDI_NUM_PORTS
#endif


// number of OUT ports (can be overruled from mios32_config.h)
#ifndef MIDI_PORT_NUM_OUT_PORTS_USB
#define MIDI_PORT_NUM_OUT_PORTS_USB MIOS32_USB_MIDI_NUM_PORTS
#endif

#ifndef MIDI_PORT_NUM_OUT_PORTS_UART
#define MIDI_PORT_NUM_OUT_PORTS_UART MIOS32_UART_NUM
#endif

#ifndef MIDI_PORT_NUM_OUT_PORTS_OSC
#define MIDI_PORT_NUM_OUT_PORTS_OSC 4
#endif

#ifndef MIDI_PORT_NUM_OUT_PORTS_SPIM
#define MIDI_PORT_NUM_OUT_PORTS_SPIM MIOS32_SPI_MIDI_NUM_PORTS
#endif


// number of CLK ports (can be overruled from mios32_config.h)
#ifndef MIDI_PORT_NUM_CLK_PORTS_USB
#define MIDI_PORT_NUM_CLK_PORTS_USB MIOS32_USB_MIDI_NUM_PORTS
#endif

#ifndef MIDI_PORT_NUM_CLK_PORTS_UART
#define MIDI_PORT_NUM_CLK_PORTS_UART MIOS32_UART_NUM
#endif

#ifndef MIDI_PORT_NUM_CLK_PORTS_OSC
#define MIDI_PORT_NUM_CLK_PORTS_OSC 4
#endif

#ifndef MIDI_PORT_NUM_CLK_PORTS_SPIM
#define MIDI_PORT_NUM_CLK_PORTS_SPIM MIOS32_SPI_MIDI_NUM_PORTS
#endif



// keep these constants consistent with the functions in midio_port.c !!!
#define MIDI_PORT_NUM_IN_PORTS  (1 + MIDI_PORT_NUM_IN_PORTS_USB  + MIDI_PORT_NUM_IN_PORTS_UART  + MIDI_PORT_NUM_IN_PORTS_OSC  + MIDI_PORT_NUM_IN_PORTS_SPIM)
#define MIDI_PORT_NUM_OUT_PORTS (1 + MIDI_PORT_NUM_OUT_PORTS_USB + MIDI_PORT_NUM_OUT_PORTS_UART + MIDI_PORT_NUM_OUT_PORTS_OSC + MIDI_PORT_NUM_OUT_PORTS_SPIM)
#define MIDI_PORT_NUM_CLK_PORTS (    MIDI_PORT_NUM_CLK_PORTS_USB + MIDI_PORT_NUM_CLK_PORTS_UART + MIDI_PORT_NUM_CLK_PORTS_OSC + MIDI_PORT_NUM_CLK_PORTS_SPIM)


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u8 ALL;
  struct {
    u8 MIDI_CLOCK:1;
    u8 ACTIVE_SENSE:1;
  };
} midi_port_mon_filter_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIDI_PORT_Init(u32 mode);

extern s32 MIDI_PORT_InNumGet(void);
extern s32 MIDI_PORT_OutNumGet(void);
extern s32 MIDI_PORT_ClkNumGet(void);

extern char *MIDI_PORT_InNameGet(u8 port_ix);
extern char *MIDI_PORT_OutNameGet(u8 port_ix);
extern char *MIDI_PORT_ClkNameGet(u8 port_ix);

extern mios32_midi_port_t MIDI_PORT_InPortGet(u8 port_ix);
extern mios32_midi_port_t MIDI_PORT_OutPortGet(u8 port_ix);
extern mios32_midi_port_t MIDI_PORT_ClkPortGet(u8 port_ix);

extern u8 MIDI_PORT_InIxGet(mios32_midi_port_t port);
extern u8 MIDI_PORT_OutIxGet(mios32_midi_port_t port);
extern u8 MIDI_PORT_ClkIxGet(mios32_midi_port_t port);

extern s32 MIDI_PORT_InCheckAvailable(mios32_midi_port_t port);
extern s32 MIDI_PORT_OutCheckAvailable(mios32_midi_port_t port);
extern s32 MIDI_PORT_ClkCheckAvailable(mios32_midi_port_t port);

extern mios32_midi_package_t MIDI_PORT_OutPackageGet(mios32_midi_port_t port);
extern mios32_midi_package_t MIDI_PORT_InPackageGet(mios32_midi_port_t port);

extern s32 MIDI_PORT_MonFilterSet(midi_port_mon_filter_t filter);
extern midi_port_mon_filter_t MIDI_PORT_MonFilterGet(void);

extern s32 MIDI_PORT_Period1mS(void);

extern s32 MIDI_PORT_NotifyMIDITx(mios32_midi_port_t port, mios32_midi_package_t package);
extern s32 MIDI_PORT_NotifyMIDIRx(mios32_midi_port_t port, mios32_midi_package_t package);

extern s32 MIDI_PORT_EventNameGet(mios32_midi_package_t package, char *label, u8 num_chars);

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif /* _MIDI_PORT_H */
