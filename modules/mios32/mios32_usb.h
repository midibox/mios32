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

// number of ports defined in mios32_usb_desc.h (-> MIOS32_USB_DESC_NUM_PORTS)


// buffer size (should be at least >= MIOS32_USB_DESC_DATA_*_SIZE/4)
#ifndef MIOS32_USB_RX_BUFFER_SIZE
#define MIOS32_USB_RX_BUFFER_SIZE   64 // packages
#endif

#ifndef MIOS32_USB_TX_BUFFER_SIZE
#define MIOS32_USB_TX_BUFFER_SIZE   64 // packages
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_USB_Init(u32 mode);

extern s32 MIOS32_USB_MIDIPackageSend(u32 package);
extern s32 MIOS32_USB_MIDIPackageReceive(u32 *package);

extern s32 MIOS32_USB_Handler(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_USB_H */
