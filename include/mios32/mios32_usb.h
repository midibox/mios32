// $Id$
/*
 * Header file for USB Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_USB_H
#define _MIOS32_USB_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


// Following settings allow to customize the USB device descriptor
#ifndef MIOS32_USB_VENDOR_ID
#define MIOS32_USB_VENDOR_ID    0x16c0        // sponsored by voti.nl! see http://www.voti.nl/pids
#endif
#ifndef MIOS32_USB_VENDOR_STR
#define MIOS32_USB_VENDOR_STR   "midibox.org" // you will see this in the USB device description
#endif
#ifndef MIOS32_USB_PRODUCT_STR
#define MIOS32_USB_PRODUCT_STR  "MIOS32"      // you will see this in the MIDI device list
#endif
#ifndef MIOS32_USB_PRODUCT_ID
#define MIOS32_USB_PRODUCT_ID   0x03ff        // ==1023; 1020-1029 reserved for T.Klose, 1000 - 1009 free for lab use
#endif
#ifndef MIOS32_USB_VERSION_ID
#define MIOS32_USB_VERSION_ID   0x0100        // v1.00
#endif


// internal defines which are used by MIOS32 USB MIDI/COM (don't touch)
#define MIOS32_USB_EP_NUM   5

// buffer table base address
#define MIOS32_USB_BTABLE_ADDRESS      0x000

// EP0 rx/tx buffer base address
#define MIOS32_USB_ENDP0_RXADDR        0x040
#define MIOS32_USB_ENDP0_TXADDR        0x080

// EP1 Rx/Tx buffer base address for MIDI driver
#define MIOS32_USB_ENDP1_TXADDR        0x0c0
#define MIOS32_USB_ENDP2_RXADDR        0x100

// EP3/4/5 buffer base addresses for COM driver
#define MIOS32_USB_ENDP3_RXADDR        0x140
#define MIOS32_USB_ENDP4_TXADDR        0x180
#define MIOS32_USB_ENDP5_TXADDR        0x1c0



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_USB_Init(u32 mode);
extern s32 MIOS32_USB_IsInitialized(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern void (*pEpInt_IN[7])(void);
extern void (*pEpInt_OUT[7])(void);

#endif /* _MIOS32_USB_H */
