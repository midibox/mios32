// $Id$
/*
 * Header file for USB MIDI Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_USB_MIDI_H
#define _MIOS32_USB_MIDI_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


// Following settings allow to customize the USB device descriptor
#ifndef MIOS32_USB_MIDI_VENDOR_ID
#define MIOS32_USB_MIDI_VENDOR_ID    0x16c0        // sponsored by voti.nl! see http://www.voti.nl/pids
#endif
#ifndef MIOS32_USB_MIDI_VENDOR_STR
#define MIOS32_USB_MIDI_VENDOR_STR   "midibox.org" // you will see this in the USB device description
#endif
#ifndef MIOS32_USB_MIDI_PRODUCT_STR
#define MIOS32_USB_MIDI_PRODUCT_STR  "MIOS32"      // you will see this in the MIDI device list
#endif
#ifndef MIOS32_USB_MIDI_PRODUCT_ID
#define MIOS32_USB_MIDI_PRODUCT_ID   1023          // 1020-1029 reserved for T.Klose, 1000 - 1009 free for lab use
#endif
#ifndef MIOS32_USB_MIDI_VERSION_ID
#define MIOS32_USB_MIDI_VERSION_ID   0x0100        // v1.00
#endif


// 1 to stay compatible to USB MIDI spec, 0 as workaround for some windows versions...
#ifndef MIOS32_USB_MIDI_USE_AC_INTERFACE
#define MIOS32_USB_MIDI_USE_AC_INTERFACE 1
#endif

// allowed numbers: 1..8
#ifndef MIOS32_USB_MIDI_NUM_PORTS
#define MIOS32_USB_MIDI_NUM_PORTS 1
#endif

// buffer size (should be at least >= MIOS32_USB_MIDI_DESC_DATA_*_SIZE/4)
#ifndef MIOS32_USB_MIDI_RX_BUFFER_SIZE
#define MIOS32_USB_MIDI_RX_BUFFER_SIZE   64 // packages
#endif

#ifndef MIOS32_USB_MIDI_TX_BUFFER_SIZE
#define MIOS32_USB_MIDI_TX_BUFFER_SIZE   64 // packages
#endif


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_USB_MIDI_Init(u32 mode);

extern s32 MIOS32_USB_MIDI_CheckAvailable(void);

extern s32 MIOS32_USB_MIDI_MIDIPackageSend(mios32_midi_package_t package);
extern s32 MIOS32_USB_MIDI_MIDIPackageReceive(mios32_midi_package_t *package);

extern s32 MIOS32_USB_MIDI_Handler(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _MIOS32_USB_MIDI_H */
