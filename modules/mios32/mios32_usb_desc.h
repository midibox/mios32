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

#ifndef _MIOS32_USB_DESC_H
#define _MIOS32_USB_DESC_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// for the case that own descriptor tables should be embedded into the project
#if !defined(MIOS32_DONT_USE_USB_DESC)

// 1 to stay compatible to USB MIDI spec, 0 as workaround for some windows versions...
#ifndef MIOS32_USB_DESC_USE_AC_INTERFACE
#define MIOS32_USB_DESC_USE_AC_INTERFACE 1
#endif

// allowed numbers: 1..8
#ifndef MIOS32_USB_DESC_NUM_PORTS
#define MIOS32_USB_DESC_NUM_PORTS 1
#endif

// used by mios32_usb.c (have to be adapted depending on descriptor lengths :-( )
#define MIOS32_USB_DESC_DATA_IN_SIZE           64
#define MIOS32_USB_DESC_DATA_OUT_SIZE          64

#define MIOS32_USB_DESC_SIZ_DEVICE_DESC        18
#define MIOS32_USB_DESC_SIZ_CLASS_DESC         (7+MIOS32_USB_DESC_NUM_PORTS*(6+6+9+9)+9+(4+MIOS32_USB_DESC_NUM_PORTS)+9+(4+MIOS32_USB_DESC_NUM_PORTS))
#define MIOS32_USB_DESC_SIZ_CONFIG_DESC        (9+9+MIOS32_USB_DESC_USE_AC_INTERFACE*(9+9)+MIOS32_USB_DESC_SIZ_CLASS_DESC)
#define MIOS32_USB_DESC_SIZ_STRING_LANGID      4
#define MIOS32_USB_DESC_SIZ_STRING_VENDOR      24
#define MIOS32_USB_DESC_SIZ_STRING_PRODUCT     14

#endif /* MIOS32_DONT_USE_USB_DESC */


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern const u8 MIOS32_USB_DESC_DeviceDescriptor[MIOS32_USB_DESC_SIZ_DEVICE_DESC];
extern const u8 MIOS32_USB_DESC_ConfigDescriptor[MIOS32_USB_DESC_SIZ_CONFIG_DESC];

extern const u8 MIOS32_USB_DESC_StringLangID[MIOS32_USB_DESC_SIZ_STRING_LANGID];
extern const u8 MIOS32_USB_DESC_StringVendor[MIOS32_USB_DESC_SIZ_STRING_VENDOR];
extern const u8 MIOS32_USB_DESC_StringProduct[MIOS32_USB_DESC_SIZ_STRING_PRODUCT];


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* __USB_DESC_H */
