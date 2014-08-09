// $Id$
//! \defgroup MIOS32_USB
//!
//! USB driver for MIOS32
//! 
//! Based on driver included in STM32 USB library
//! Some code copied and modified from Virtual_COM_Port demo
//! 
//! Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
//! 
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_USB)

#include <usbd_core.h>
#include <usbd_def.h>
#include <usbd_desc.h>
#include <usbd_req.h>
#include <usbd_conf.h>
#include <usbh_core.h>
#include <usbh_conf.h>
#include <usb_otg.h>
#include <usb_dcd_int.h>
#include <usb_hcd_int.h>
#include <usb_regs.h>

#include <string.h>


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define DSCR_DEVICE	1	// Descriptor type: Device
#define DSCR_CONFIG	2	// Descriptor type: Configuration
#define DSCR_STRING	3	// Descriptor type: String
#define DSCR_INTRFC	4	// Descriptor type: Interface
#define DSCR_ENDPNT	5	// Descriptor type: Endpoint

#define CS_INTERFACE	0x24	// Class-specific type: Interface
#define CS_ENDPOINT	0x25	// Class-specific type: Endpoint

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

// also used in mios32_usb_midi.c
__ALIGN_BEGIN USB_OTG_CORE_HANDLE  USB_OTG_dev __ALIGN_END;
uint32_t USB_rx_buffer[MIOS32_USB_MIDI_DATA_OUT_SIZE/4];

#ifndef MIOS32_DONT_USE_USB_HOST
__ALIGN_BEGIN USBH_HOST USB_Host __ALIGN_END;
extern const USBH_Class_cb_TypeDef MIOS32_MIDI_USBH_Callbacks; // implemented in mios32_usb_midi.c
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Descriptor Handling
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


// a lot confusing settings, which could also be generated via software!
#ifndef MIOS32_DONT_USE_USB_MIDI
# if MIOS32_USB_MIDI_USE_AC_INTERFACE
#  define MIOS32_USB_MIDI_NUM_INTERFACES        2
#  define MIOS32_USB_MIDI_AC_INTERFACE_IX       0x00
#  define MIOS32_USB_MIDI_AS_INTERFACE_IX       0x01
#  define MIOS32_USB_MIDI_INTERFACE_OFFSET      2
# else
#  define MIOS32_USB_MIDI_NUM_INTERFACES        1
#  define MIOS32_USB_MIDI_INTERFACE_NUM         0x00
#  define MIOS32_USB_MIDI_INTERFACE_OFFSET      1
# endif
# define MIOS32_USB_MIDI_SIZ_CLASS_DESC         (7+MIOS32_USB_MIDI_NUM_PORTS*(6+6+9+9)+9+(4+MIOS32_USB_MIDI_NUM_PORTS)+9+(4+MIOS32_USB_MIDI_NUM_PORTS))
# define MIOS32_USB_MIDI_SIZ_CONFIG_DESC        (9+MIOS32_USB_MIDI_USE_AC_INTERFACE*(9+9)+MIOS32_USB_MIDI_SIZ_CLASS_DESC)

# define MIOS32_USB_MIDI_SIZ_CLASS_DESC_SINGLE_USB  (7+1*(6+6+9+9)+9+(4+1)+9+(4+1))
# define MIOS32_USB_MIDI_SIZ_CONFIG_DESC_SINGLE_USB (9+MIOS32_USB_MIDI_USE_AC_INTERFACE*(9+9)+MIOS32_USB_MIDI_SIZ_CLASS_DESC)

#else
# define MIOS32_USB_MIDI_NUM_INTERFACES         0
# define MIOS32_USB_MIDI_INTERFACE_OFFSET       0
# define MIOS32_USB_MIDI_SIZ_CONFIG_DESC        0
#endif

#ifdef MIOS32_USE_USB_COM
# define MIOS32_USB_COM_NUM_INTERFACES          2
# define MIOS32_USB_COM_SIZ_CONFIG_DESC         58
# define MIOS32_USB_COM_CC_INTERFACE_IX         (MIOS32_USB_MIDI_INTERFACE_OFFSET + 0x00)
# define MIOS32_USB_COM_CD_INTERFACE_IX         (MIOS32_USB_MIDI_INTERFACE_OFFSET + 0x01)
# define MIOS32_USB_COM_INTERFACE_OFFSET        (MIOS32_USB_MIDI_INTERFACE_OFFSET + 2)
#else
# define MIOS32_USB_COM_NUM_INTERFACES          0
# define MIOS32_USB_COM_SIZ_CONFIG_DESC         0
# define MIOS32_USB_COM_INTERFACE_OFFSET        (MIOS32_USB_MIDI_INTERFACE_OFFSET + 0)
#endif

#define MIOS32_USB_NUM_INTERFACES              (MIOS32_USB_MIDI_NUM_INTERFACES + MIOS32_USB_COM_NUM_INTERFACES)
#define MIOS32_USB_SIZ_CONFIG_DESC             (9 + MIOS32_USB_MIDI_SIZ_CONFIG_DESC + MIOS32_USB_COM_SIZ_CONFIG_DESC)
#define MIOS32_USB_SIZ_CONFIG_DESC_SINGLE_USB  (9 + MIOS32_USB_MIDI_SIZ_CONFIG_DESC_SINGLE_USB)


/////////////////////////////////////////////////////////////////////////////
// USB Standard Device Descriptor
/////////////////////////////////////////////////////////////////////////////
#define MIOS32_USB_SIZ_DEVICE_DESC 18
static const __ALIGN_BEGIN u8 MIOS32_USB_DeviceDescriptor[MIOS32_USB_SIZ_DEVICE_DESC] = {
  (u8)(MIOS32_USB_SIZ_DEVICE_DESC&0xff), // Device Descriptor length
  DSCR_DEVICE,			// Decriptor type
  (u8)(0x0200 & 0xff),		// Specification Version (BCD, LSB)
  (u8)(0x0200 >> 8),		// Specification Version (BCD, MSB)
#ifdef MIOS32_USE_USB_COM
  0x02,				// Device class "Communication"   -- required for MacOS to find the COM device. Audio Device works fine in parallel to this
  // Update: by default disabled, since USB MIDI fails on MacOS Lion!!!
#else
  0x00,				// Device class "Composite"
#endif
  0x00,				// Device sub-class
  0x00,				// Device sub-sub-class
  0x40,				// Maximum packet size
  (u8)((MIOS32_USB_VENDOR_ID) & 0xff),  // Vendor ID (LSB)
  (u8)((MIOS32_USB_VENDOR_ID) >> 8),    // Vendor ID (MSB)
  (u8)((MIOS32_USB_PRODUCT_ID) & 0xff),	// Product ID (LSB)
  (u8)((MIOS32_USB_PRODUCT_ID) >> 8),	// Product ID (MSB)
  (u8)((MIOS32_USB_VERSION_ID) & 0xff),	// Product version ID (LSB)
  (u8)((MIOS32_USB_VERSION_ID) >> 8),  	// Product version ID (MSB)
  USBD_IDX_MFC_STR,		// Manufacturer string index
  USBD_IDX_PRODUCT_STR,		// Product string index
  USBD_IDX_SERIAL_STR,		// Serial number string index
  0x01 				// Number of configurations
};


/* USB Standard Device Descriptor */
static const __ALIGN_BEGIN u8 USBD_LangIDDesc[4] __ALIGN_END =
{
     4,
     USB_DESC_TYPE_STRING,       
     0x09, // CharSet
     0x04, // U.S.
};


/////////////////////////////////////////////////////////////////////////////
// USB Config Descriptor
/////////////////////////////////////////////////////////////////////////////

// TODO: generate the config descriptor via software
// this has to be done in *MIOS32_USB_CB_GetConfigDescriptor()
// Problem: it would increase stack or static RAM consumption

static const __ALIGN_BEGIN u8 MIOS32_USB_ConfigDescriptor[MIOS32_USB_SIZ_CONFIG_DESC] = {
  // Configuration Descriptor
  9,				// Descriptor length
  DSCR_CONFIG,			// Descriptor type
  (MIOS32_USB_SIZ_CONFIG_DESC) & 0xff,  // Config + End Points length (LSB)
  (MIOS32_USB_SIZ_CONFIG_DESC) >> 8,    // Config + End Points length (LSB)
  MIOS32_USB_NUM_INTERFACES,    // Number of interfaces
  0x01,				// Configuration Value
  USBD_IDX_CONFIG_STR,		// Configuration string
  0x80,				// Attributes (b7 - buspwr, b6 - selfpwr, b5 - rwu)
  0x32,				// Power requirement (div 2 ma)


  ///////////////////////////////////////////////////////////////////////////
  // USB MIDI
  ///////////////////////////////////////////////////////////////////////////
#ifndef MIOS32_DONT_USE_USB_MIDI

#if MIOS32_USB_MIDI_USE_AC_INTERFACE
  // Standard AC Interface Descriptor
  9,				// Descriptor length
  DSCR_INTRFC,			// Descriptor type
  MIOS32_USB_MIDI_AC_INTERFACE_IX, // Zero-based index of this interface
  0x00,				// Alternate setting
  0x00,				// Number of end points 
  0x01,				// Interface class  (AUDIO)
  0x01,				// Interface sub class  (AUDIO_CONTROL)
  0x00,				// Interface sub sub class
  USBD_IDX_PRODUCT_STR,		// Interface descriptor string index

  // MIDI Adapter Class-specific AC Interface Descriptor
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type
  0x01,				// Header subtype
  0x00,				// Revision of class specification - 1.0 (LSB)
  0x01,				// Revision of class specification (MSB)
  9,				// Total size of class-specific descriptors (LSB)
  0,				// Total size of class-specific descriptors (MSB)
  0x01,				// Number of streaming interfaces
  0x01,				// MIDI Streaming Interface 1 belongs to this AudioControl Interface
#endif

  // Standard MS Interface Descriptor
  9,				// Descriptor length
  DSCR_INTRFC,			// Descriptor type
  MIOS32_USB_MIDI_AS_INTERFACE_IX, // Zero-based index of this interface
  0x00,				// Alternate setting
  0x02,				// Number of end points 
  0x01,				// Interface class  (AUDIO)
  0x03,				// Interface sub class  (MIDISTREAMING)
  0x00,				// Interface sub sub class
  USBD_IDX_PRODUCT_STR,		// Interface descriptor string index

  // Class-specific MS Interface Descriptor
  7,				// Descriptor length
  CS_INTERFACE,			// Descriptor type
#if MIOS32_USB_MIDI_USE_AC_INTERFACE
  0x01,				// Zero-based index of this interface
#else
  0x00,				// Zero-based index of this interface
#endif
  0x00,				// revision of this class specification (LSB)
  0x01,				// revision of this class specification (MSB)
  (u8)(MIOS32_USB_MIDI_SIZ_CLASS_DESC & 0xff), // Total size of class-specific descriptors (LSB)
  (u8)(MIOS32_USB_MIDI_SIZ_CLASS_DESC >> 8),   // Total size of class-specific descriptors (MSB)


#if MIOS32_USB_MIDI_NUM_PORTS >= 1
  // MIDI IN Jack Descriptor (Embedded)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x01,				// EMBEDDED
  0x01,				// ID of this jack
  0x00,				// unused

  // MIDI Adapter MIDI IN Jack Descriptor (External)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x02,				// EXTERNAL
  0x02,				// ID of this jack
  0x00,				// unused

  // MIDI Adapter MIDI OUT Jack Descriptor (Embedded)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x01,				// EMBEDDED
  0x03,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x02,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused

  // MIDI Adapter MIDI OUT Jack Descriptor (External)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x02,				// EXTERNAL
  0x04,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x01,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused
#endif


#if MIOS32_USB_MIDI_NUM_PORTS >= 2
  // Second MIDI IN Jack Descriptor (Embedded)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x01,				// EMBEDDED
  0x05,				// ID of this jack
  0x00,				// unused

  // Second MIDI Adapter MIDI IN Jack Descriptor (External)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x02,				// EXTERNAL
  0x06,				// ID of this jack
  0x00,				// unused

  // Second MIDI Adapter MIDI OUT Jack Descriptor (Embedded)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x01,				// EMBEDDED
  0x07,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x06,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused

  // Second MIDI Adapter MIDI OUT Jack Descriptor (External)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x02,				// EXTERNAL
  0x08,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x05,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused
#endif


#if MIOS32_USB_MIDI_NUM_PORTS >= 3
  // Third MIDI IN Jack Descriptor (Embedded)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x01,				// EMBEDDED
  0x09,				// ID of this jack
  0x00,				// unused

  // Third MIDI Adapter MIDI IN Jack Descriptor (External)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x02,				// EXTERNAL
  0x0a,				// ID of this jack
  0x00,				// unused

  // Third MIDI Adapter MIDI OUT Jack Descriptor (Embedded)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x01,				// EMBEDDED
  0x0b,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x0a,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused

  // Third MIDI Adapter MIDI OUT Jack Descriptor (External)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x02,				// EXTERNAL
  0x0c,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x09,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused
#endif


#if MIOS32_USB_MIDI_NUM_PORTS >= 4
  // Fourth MIDI IN Jack Descriptor (Embedded)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x01,				// EMBEDDED
  0x0d,				// ID of this jack
  0x00,				// unused

  // Fourth MIDI Adapter MIDI IN Jack Descriptor (External)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x02,				// EXTERNAL
  0x0e,				// ID of this jack
  0x00,				// unused

  // Fourth MIDI Adapter MIDI OUT Jack Descriptor (Embedded)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x01,				// EMBEDDED
  0x0f,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x0e,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused

  // Fourth MIDI Adapter MIDI OUT Jack Descriptor (External)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x02,				// EXTERNAL
  0x10,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x0d,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused
#endif


#if MIOS32_USB_MIDI_NUM_PORTS >= 5
  // Fifth MIDI IN Jack Descriptor (Embedded)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x01,				// EMBEDDED
  0x11,				// ID of this jack
  0x00,				// unused

  // Fifth MIDI Adapter MIDI IN Jack Descriptor (External)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x02,				// EXTERNAL
  0x12,				// ID of this jack
  0x00,				// unused

  // Fifth MIDI Adapter MIDI OUT Jack Descriptor (Embedded)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x01,				// EMBEDDED
  0x13,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x12,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused

  // Fifth MIDI Adapter MIDI OUT Jack Descriptor (External)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x02,				// EXTERNAL
  0x14,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x11,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused
#endif


#if MIOS32_USB_MIDI_NUM_PORTS >= 6
  // Sixth MIDI IN Jack Descriptor (Embedded)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x01,				// EMBEDDED
  0x15,				// ID of this jack
  0x00,				// unused

  // Sixth MIDI Adapter MIDI IN Jack Descriptor (External)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x02,				// EXTERNAL
  0x16,				// ID of this jack
  0x00,				// unused

  // Sixth MIDI Adapter MIDI OUT Jack Descriptor (Embedded)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x01,				// EMBEDDED
  0x17,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x16,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused

  // Sixth MIDI Adapter MIDI OUT Jack Descriptor (External)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x02,				// EXTERNAL
  0x18,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x15,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused
#endif


#if MIOS32_USB_MIDI_NUM_PORTS >= 7
  // Seventh MIDI IN Jack Descriptor (Embedded)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x01,				// EMBEDDED
  0x19,				// ID of this jack
  0x00,				// unused

  // Seventh MIDI Adapter MIDI IN Jack Descriptor (External)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x02,				// EXTERNAL
  0x1a,				// ID of this jack
  0x00,				// unused

  // Seventh MIDI Adapter MIDI OUT Jack Descriptor (Embedded)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x01,				// EMBEDDED
  0x1b,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x1a,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused

  // Seventh MIDI Adapter MIDI OUT Jack Descriptor (External)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x02,				// EXTERNAL
  0x1c,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x19,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused
#endif


#if MIOS32_USB_MIDI_NUM_PORTS >= 8
  // Eighth MIDI IN Jack Descriptor (Embedded)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x01,				// EMBEDDED
  0x1d,				// ID of this jack
  0x00,				// unused

  // Eighth MIDI Adapter MIDI IN Jack Descriptor (External)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x02,				// EXTERNAL
  0x1e,				// ID of this jack
  0x00,				// unused

  // Eighth MIDI Adapter MIDI OUT Jack Descriptor (Embedded)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x01,				// EMBEDDED
  0x1f,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x1e,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused

  // Eighth MIDI Adapter MIDI OUT Jack Descriptor (External)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x02,				// EXTERNAL
  0x20,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x1d,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused
#endif


  // Standard Bulk OUT Endpoint Descriptor
  9,				// Descriptor length
  DSCR_ENDPNT,			// Descriptor type
  0x02,				// Out Endpoint 2
  0x02,				// Bulk, not shared
  (u8)(MIOS32_USB_MIDI_DATA_IN_SIZE&0xff),	// num of bytes per packet (LSB)
  (u8)(MIOS32_USB_MIDI_DATA_IN_SIZE>>8),	// num of bytes per packet (MSB)
  0x00,				// ignore for bulk
  0x00,				// unused
  0x00,				// unused

  // Class-specific MS Bulk Out Endpoint Descriptor
  4+MIOS32_USB_MIDI_NUM_PORTS,	// Descriptor length
  CS_ENDPOINT,			// Descriptor type (CS_ENDPOINT)
  0x01,				// MS_GENERAL
  MIOS32_USB_MIDI_NUM_PORTS,	// number of embedded MIDI IN Jacks
  0x01,				// ID of embedded MIDI In Jack
#if MIOS32_USB_MIDI_NUM_PORTS >= 2
  0x05,				// ID of embedded MIDI In Jack
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 3
  0x09,				// ID of embedded MIDI In Jack
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 4
  0x0d,				// ID of embedded MIDI In Jack
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 5
  0x11,				// ID of embedded MIDI In Jack
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 6
  0x15,				// ID of embedded MIDI In Jack
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 7
  0x19,				// ID of embedded MIDI In Jack
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 8
  0x1d,				// ID of embedded MIDI In Jack
#endif

  // Standard Bulk IN Endpoint Descriptor
  9,				// Descriptor length
  DSCR_ENDPNT,			// Descriptor type
  MIOS32_USB_MIDI_DATA_IN_EP,	// In Endpoint 1
  0x02,				// Bulk, not shared
  (u8)(MIOS32_USB_MIDI_DATA_OUT_SIZE&0xff),	// num of bytes per packet (LSB)
  (u8)(MIOS32_USB_MIDI_DATA_OUT_SIZE>>8),	// num of bytes per packet (MSB)
  0x00,				// ignore for bulk
  0x00,				// unused
  0x00,				// unused

  // Class-specific MS Bulk In Endpoint Descriptor
  4+MIOS32_USB_MIDI_NUM_PORTS,	// Descriptor length
  CS_ENDPOINT,			// Descriptor type (CS_ENDPOINT)
  0x01,				// MS_GENERAL
  MIOS32_USB_MIDI_NUM_PORTS,	// number of embedded MIDI Out Jacks
  0x03,				// ID of embedded MIDI Out Jack
#if MIOS32_USB_MIDI_NUM_PORTS >= 2
  0x07,				// ID of embedded MIDI Out Jack
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 3
  0x0b,				// ID of embedded MIDI Out Jack
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 4
  0x0f,				// ID of embedded MIDI Out Jack
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 5
  0x13,				// ID of embedded MIDI Out Jack
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 6
  0x17,				// ID of embedded MIDI Out Jack
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 7
  0x1b,				// ID of embedded MIDI Out Jack
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 8
  0x1f,				// ID of embedded MIDI Out Jack
#endif

#endif /* MIOS32_DONT_USE_USB_MIDI */
};




/////////////////////////////////////////////////////////////////////////////
// Alternative config descriptor with only a single USB MIDI port
// Workaround for some windows installations where multiple MIDI ports don't work
/////////////////////////////////////////////////////////////////////////////

#if !defined(MIOS32_DONT_USE_USB_MIDI) && MIOS32_USB_MIDI_NUM_PORTS > 1
static const __ALIGN_BEGIN u8 MIOS32_USB_ConfigDescriptor_SingleUSB[MIOS32_USB_SIZ_CONFIG_DESC_SINGLE_USB] = {
  // Configuration Descriptor
  9,				// Descriptor length
  DSCR_CONFIG,			// Descriptor type
  (MIOS32_USB_SIZ_CONFIG_DESC_SINGLE_USB) & 0xff,  // Config + End Points length (LSB)
  (MIOS32_USB_SIZ_CONFIG_DESC_SINGLE_USB) >> 8,    // Config + End Points length (LSB)
  MIOS32_USB_NUM_INTERFACES,    // Number of interfaces
  0x01,				// Configuration Value
  USBD_IDX_PRODUCT_STR,		// Configuration string
  0x80,				// Attributes (b7 - buspwr, b6 - selfpwr, b5 - rwu)
  0x32,				// Power requirement (div 2 ma)


  ///////////////////////////////////////////////////////////////////////////
  // USB MIDI
  ///////////////////////////////////////////////////////////////////////////
#if MIOS32_USB_MIDI_USE_AC_INTERFACE
  // Standard AC Interface Descriptor
  9,				// Descriptor length
  DSCR_INTRFC,			// Descriptor type
  MIOS32_USB_MIDI_AC_INTERFACE_IX, // Zero-based index of this interface
  0x00,				// Alternate setting
  0x00,				// Number of end points 
  0x01,				// Interface class  (AUDIO)
  0x01,				// Interface sub class  (AUDIO_CONTROL)
  0x00,				// Interface sub sub class
  USBD_IDX_PRODUCT_STR,		// Interface descriptor string index

  // MIDI Adapter Class-specific AC Interface Descriptor
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type
  0x01,				// Header subtype
  0x00,				// Revision of class specification - 1.0 (LSB)
  0x01,				// Revision of class specification (MSB)
  9,				// Total size of class-specific descriptors (LSB)
  0,				// Total size of class-specific descriptors (MSB)
  0x01,				// Number of streaming interfaces
  0x01,				// MIDI Streaming Interface 1 belongs to this AudioControl Interface
#endif

  // Standard MS Interface Descriptor
  9,				// Descriptor length
  DSCR_INTRFC,			// Descriptor type
  MIOS32_USB_MIDI_AS_INTERFACE_IX, // Zero-based index of this interface
  0x00,				// Alternate setting
  0x02,				// Number of end points 
  0x01,				// Interface class  (AUDIO)
  0x03,				// Interface sub class  (MIDISTREAMING)
  0x00,				// Interface sub sub class
  USBD_IDX_PRODUCT_STR,		// Interface descriptor string index

  // Class-specific MS Interface Descriptor
  7,				// Descriptor length
  CS_INTERFACE,			// Descriptor type
#if MIOS32_USB_MIDI_USE_AC_INTERFACE
  0x01,				// Zero-based index of this interface
#else
  0x00,				// Zero-based index of this interface
#endif
  0x00,				// revision of this class specification (LSB)
  0x01,				// revision of this class specification (MSB)
  (u8)(MIOS32_USB_MIDI_SIZ_CLASS_DESC_SINGLE_USB & 0xff), // Total size of class-specific descriptors (LSB)
  (u8)(MIOS32_USB_MIDI_SIZ_CLASS_DESC_SINGLE_USB >> 8),   // Total size of class-specific descriptors (MSB)


  // MIDI IN Jack Descriptor (Embedded)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x01,				// EMBEDDED
  0x01,				// ID of this jack
  0x00,				// unused

  // MIDI Adapter MIDI IN Jack Descriptor (External)
  6,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x02,				// MIDI_IN_JACK subtype
  0x02,				// EXTERNAL
  0x02,				// ID of this jack
  0x00,				// unused

  // MIDI Adapter MIDI OUT Jack Descriptor (Embedded)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x01,				// EMBEDDED
  0x03,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x02,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused

  // MIDI Adapter MIDI OUT Jack Descriptor (External)
  9,				// Descriptor length
  CS_INTERFACE,			// Descriptor type (CS_INTERFACE)
  0x03,				// MIDI_OUT_JACK subtype
  0x02,				// EXTERNAL
  0x04,				// ID of this jack
  0x01,				// number of input pins of this jack
  0x01,				// ID of the entity to which this pin is connected
  0x01,				// Output Pin number of the entity to which this input pin is connected
  0x00,				// unused

  // Standard Bulk OUT Endpoint Descriptor
  9,				// Descriptor length
  DSCR_ENDPNT,			// Descriptor type
  MIOS32_USB_MIDI_DATA_OUT_EP,	// Out Endpoint 2
  0x02,				// Bulk, not shared
  (u8)(MIOS32_USB_MIDI_DATA_IN_SIZE&0xff),	// num of bytes per packet (LSB)
  (u8)(MIOS32_USB_MIDI_DATA_IN_SIZE>>8),	// num of bytes per packet (MSB)
  0x00,				// ignore for bulk
  0x00,				// unused
  0x00,				// unused

  // Class-specific MS Bulk Out Endpoint Descriptor
  4+1,                     	// Descriptor length
  CS_ENDPOINT,			// Descriptor type (CS_ENDPOINT)
  0x01,				// MS_GENERAL
  1,                       	// number of embedded MIDI IN Jacks
  0x01,				// ID of embedded MIDI In Jack

  // Standard Bulk IN Endpoint Descriptor
  9,				// Descriptor length
  DSCR_ENDPNT,			// Descriptor type
  MIOS32_USB_MIDI_DATA_IN_EP,	// In Endpoint 1
  0x02,				// Bulk, not shared
  (u8)(MIOS32_USB_MIDI_DATA_OUT_SIZE&0xff),	// num of bytes per packet (LSB)
  (u8)(MIOS32_USB_MIDI_DATA_OUT_SIZE>>8),	// num of bytes per packet (MSB)
  0x00,				// ignore for bulk
  0x00,				// unused
  0x00,				// unused

  // Class-specific MS Bulk In Endpoint Descriptor
  4+1,                      	// Descriptor length
  CS_ENDPOINT,			// Descriptor type (CS_ENDPOINT)
  0x01,				// MS_GENERAL
  1,                        	// number of embedded MIDI Out Jacks
  0x03,				// ID of embedded MIDI Out Jack
};
#endif /* MIOS32_DONT_USE_USB_MIDI */


/**
* @brief  USBD_USR_DeviceDescriptor 
*         return the device descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
static uint8_t *  USBD_USR_DeviceDescriptor( uint8_t speed , uint16_t *length)
{
  *length = sizeof(MIOS32_USB_DeviceDescriptor);
  return (uint8_t *)MIOS32_USB_DeviceDescriptor;
}

/**
* @brief  USBD_USR_LangIDStrDescriptor 
*         return the LangID string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
static uint8_t *  USBD_USR_LangIDStrDescriptor( uint8_t speed , uint16_t *length)
{
  *length =  sizeof(USBD_LangIDDesc);  
  return (uint8_t *)USBD_LangIDDesc;
}


/**
* @brief  USBD_USR_ProductStrDescriptor 
*         return the product string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
static uint8_t *  USBD_USR_ProductStrDescriptor( uint8_t speed , uint16_t *length)
{
  const u8 product_str[] = MIOS32_USB_PRODUCT_STR;
  int len;

  // buffer[0] and [1] initialized below
  // check for user defined product string
  char *product_str_ptr = (char *)product_str;
#ifdef MIOS32_SYS_ADDR_USB_DEV_NAME
  char *product_str_user = (char *)MIOS32_SYS_ADDR_USB_DEV_NAME;
  int j;
  u8 valid_str = 1;
  for(j=0, len=0; j<MIOS32_SYS_USB_DEV_NAME_LEN && valid_str && product_str_user[j]; ++j, ++len) {
    if( product_str_user[j] < 0x20 || product_str_user[j] >= 0x80 )
      valid_str = 0;
  }
  if( valid_str && len )
    product_str_ptr = product_str_user;
#endif

  USBD_GetString ((uint8_t*)product_str_ptr, USBD_StrDesc, length);

  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_ManufacturerStrDescriptor 
*         return the manufacturer string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
static uint8_t *  USBD_USR_ManufacturerStrDescriptor( uint8_t speed , uint16_t *length)
{
  const uint8_t vendor_str[] = MIOS32_USB_VENDOR_STR;
  USBD_GetString ((uint8_t*)vendor_str, USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_SerialStrDescriptor 
*         return the serial number string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
static uint8_t *  USBD_USR_SerialStrDescriptor( uint8_t speed , uint16_t *length)
{
  const u8 serial_number_dummy_str[] = "42";

  u8 serial_number_str[40];
  if( MIOS32_SYS_SerialNumberGet((char *)serial_number_str) >= 0 ) {
    USBD_GetString ((uint8_t*)serial_number_str, USBD_StrDesc, length);
  } else {
    USBD_GetString ((uint8_t*)serial_number_dummy_str, USBD_StrDesc, length);
  }

  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_ConfigStrDescriptor 
*         return the configuration string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
static uint8_t *  USBD_USR_ConfigStrDescriptor( uint8_t speed , uint16_t *length)
{
  return USBD_USR_ProductStrDescriptor(speed, length);
}


/**
* @brief  USBD_USR_InterfaceStrDescriptor 
*         return the interface string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
static uint8_t *  USBD_USR_InterfaceStrDescriptor( uint8_t speed , uint16_t *length)
{
  return USBD_USR_ProductStrDescriptor(speed, length);
}


static const USBD_DEVICE USR_desc =
{
  USBD_USR_DeviceDescriptor,
  USBD_USR_LangIDStrDescriptor, 
  USBD_USR_ManufacturerStrDescriptor,
  USBD_USR_ProductStrDescriptor,
  USBD_USR_SerialStrDescriptor,
  USBD_USR_ConfigStrDescriptor,
  USBD_USR_InterfaceStrDescriptor,  
};




/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// User Device hooks for different device states
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/**
* @brief  USBD_USR_Init 
*         Displays the message on LCD for host lib initialization
* @param  None
* @retval None
*/
static void USBD_USR_Init(void)
{   
}

/**
* @brief  USBD_USR_DeviceReset 
*         Displays the message on LCD on device Reset Event
* @param  speed : device speed
* @retval None
*/
static void USBD_USR_DeviceReset(uint8_t speed )
{
}


/**
* @brief  USBD_USR_DeviceConfigured
*         Displays the message on LCD on device configuration Event
* @param  None
* @retval Staus
*/
static void USBD_USR_DeviceConfigured (void)
{
#ifndef MIOS32_DONT_USE_USB_MIDI
  MIOS32_USB_MIDI_ChangeConnectionState(1);
#endif
}


/**
* @brief  USBD_USR_DeviceConnected
*         Displays the message on LCD on device connection Event
* @param  None
* @retval Staus
*/
static void USBD_USR_DeviceConnected (void)
{
}


/**
* @brief  USBD_USR_DeviceDisonnected
*         Displays the message on LCD on device disconnection Event
* @param  None
* @retval Staus
*/
static void USBD_USR_DeviceDisconnected (void)
{
#ifndef MIOS32_DONT_USE_USB_MIDI
  MIOS32_USB_MIDI_ChangeConnectionState(0);
#endif
}

/**
* @brief  USBD_USR_DeviceSuspended 
*         Displays the message on LCD on device suspend Event
* @param  None
* @retval None
*/
static void USBD_USR_DeviceSuspended(void)
{
  /* Users can do their application actions here for the USB-Reset */
}


/**
* @brief  USBD_USR_DeviceResumed 
*         Displays the message on LCD on device resume Event
* @param  None
* @retval None
*/
static void USBD_USR_DeviceResumed(void)
{
  /* Users can do their application actions here for the USB-Reset */
}


static const USBD_Usr_cb_TypeDef USBD_USR_Callbacks =
{
  USBD_USR_Init,
  USBD_USR_DeviceReset,
  USBD_USR_DeviceConfigured,
  USBD_USR_DeviceSuspended,
  USBD_USR_DeviceResumed,
  USBD_USR_DeviceConnected,
  USBD_USR_DeviceDisconnected,
};



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// User Host hooks for different device states
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#ifndef MIOS32_DONT_USE_USB_HOST

/**
 * @brief  USBH_USR_Init
 *         Displays the message on LCD for host lib initialization
 * @param  None
 * @retval None
 */
static void USBH_USR_Init(void)
{
}

/**
 * @brief  USBH_USR_DeviceAttached
 *         Displays the message on LCD on device attached
 * @param  None
 * @retval None
 */
static void USBH_USR_DeviceAttached(void)
{  
}

/**
 * @brief  USBH_USR_UnrecoveredError
 * @param  None
 * @retval None
 */
static void USBH_USR_UnrecoveredError (void)
{
}

/**
 * @brief  USBH_DisconnectEvent
 *         Device disconnect event
 * @param  None
 * @retval None
 */
static void USBH_USR_DeviceDisconnected (void)
{
  MIOS32_USB_MIDI_ChangeConnectionState(0);
}

/**
 * @brief  USBH_USR_ResetUSBDevice
 *         Reset USB Device
 * @param  None
 * @retval None
 */
static void USBH_USR_ResetDevice(void)
{
}


/**
 * @brief  USBH_USR_DeviceSpeedDetected
 *         Displays the message on LCD for device speed
 * @param  Devicespeed : Device Speed
 * @retval None
 */
static void USBH_USR_DeviceSpeedDetected(uint8_t DeviceSpeed)
{
}

/**
 * @brief  USBH_USR_Device_DescAvailable
 *         Displays the message on LCD for device descriptor
 * @param  DeviceDesc : device descriptor
 * @retval None
 */
static void USBH_USR_Device_DescAvailable(void *DeviceDesc)
{
}

/**
 * @brief  USBH_USR_DeviceAddressAssigned
 *         USB device is successfully assigned the Address
 * @param  None
 * @retval None
 */
static void USBH_USR_DeviceAddressAssigned(void)
{
}


/**
 * @brief  USBH_USR_Conf_Desc
 *         Displays the message on LCD for configuration descriptor
 * @param  ConfDesc : Configuration descriptor
 * @retval None
 */
static void USBH_USR_Configuration_DescAvailable(USBH_CfgDesc_TypeDef * cfgDesc,
						 USBH_InterfaceDesc_TypeDef *itfDesc,
						 USBH_EpDesc_TypeDef *epDesc)
{
}

/**
 * @brief  USBH_USR_Manufacturer_String
 *         Displays the message on LCD for Manufacturer String
 * @param  ManufacturerString : Manufacturer String of Device
 * @retval None
 */
static void USBH_USR_Manufacturer_String(void *ManufacturerString)
{
#ifdef MIOS32_MIDI_USBH_DEBUG
  // Debug Output via UART0
  mios32_midi_port_t prev_port = MIOS32_MIDI_DebugPortGet();
  MIOS32_MIDI_DebugPortSet(UART0);
  MIOS32_MIDI_SendDebugMessage("[USBH_USR] Manufacturer: %s", ManufacturerString);
  MIOS32_MIDI_DebugPortSet(prev_port);
#endif
}

/**
 * @brief  USBH_USR_Product_String
 *         Displays the message on LCD for Product String
 * @param  ProductString : Product String of Device
 * @retval None
 */
static void USBH_USR_Product_String(void *ProductString)
{
#ifdef MIOS32_MIDI_USBH_DEBUG
  // Debug Output via UART0
  mios32_midi_port_t prev_port = MIOS32_MIDI_DebugPortGet();
  MIOS32_MIDI_DebugPortSet(UART0);
  MIOS32_MIDI_SendDebugMessage("[USBH_USR] Product: %s", ProductString);
  MIOS32_MIDI_DebugPortSet(prev_port);
#endif
}

/**
 * @brief  USBH_USR_SerialNum_String
 *         Displays the message on LCD for SerialNum_String
 * @param  SerialNumString : SerialNum_String of device
 * @retval None
 */
static void USBH_USR_SerialNum_String(void *SerialNumString)
{
#ifdef MIOS32_MIDI_USBH_DEBUG
  // Debug Output via UART0
  mios32_midi_port_t prev_port = MIOS32_MIDI_DebugPortGet();
  MIOS32_MIDI_DebugPortSet(UART0);
  MIOS32_MIDI_SendDebugMessage("[USBH_USR] Serial Number: %s", SerialNumString);
  MIOS32_MIDI_DebugPortSet(prev_port);
#endif
} 

/**
 * @brief  EnumerationDone
 *         User response request is displayed to ask for
 *         application jump to class
 * @param  None
 * @retval None
 */
static void USBH_USR_EnumerationDone(void)
{
} 

/**
 * @brief  USBH_USR_DeviceNotSupported
 *         Device is not supported
 * @param  None
 * @retval None
 */
static void USBH_USR_DeviceNotSupported(void)
{
}  


/**
 * @brief  USBH_USR_UserInput
 *         User Action for application state entry
 * @param  None
 * @retval USBH_USR_Status : User response for key button
 */
static USBH_USR_Status USBH_USR_UserInput(void)
{
  return USBH_USR_RESP_OK;
}

/**
 * @brief  USBH_USR_OverCurrentDetected
 *         Device Overcurrent detection event
 * @param  None
 * @retval None
 */
static void USBH_USR_OverCurrentDetected (void)
{
}

/**
* @brief  USBH_USR_MSC_Application 
*         Demo application for mass storage
* @param  None
* @retval Staus
*/
static int USBH_USR_Application(void)
{
  return (0);
}

/**
 * @brief  USBH_USR_DeInit
 *         Deinit User state and associated variables
 * @param  None
 * @retval None
 */
static void USBH_USR_DeInit(void)
{
}


static const USBH_Usr_cb_TypeDef USBH_USR_Callbacks =
{
  USBH_USR_Init,
  USBH_USR_DeInit,
  USBH_USR_DeviceAttached,
  USBH_USR_ResetDevice,
  USBH_USR_DeviceDisconnected,
  USBH_USR_OverCurrentDetected,
  USBH_USR_DeviceSpeedDetected,
  USBH_USR_Device_DescAvailable,
  USBH_USR_DeviceAddressAssigned,
  USBH_USR_Configuration_DescAvailable,
  USBH_USR_Manufacturer_String,
  USBH_USR_Product_String,
  USBH_USR_SerialNum_String,
  USBH_USR_EnumerationDone,
  USBH_USR_UserInput,
  USBH_USR_Application,
  USBH_USR_DeviceNotSupported,
  USBH_USR_UnrecoveredError
};


#endif /* MIOS32_DONT_USE_USB_HOST */


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// BSP Layer
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/**
* @brief  USB_OTG_BSP_Init
*         Initilizes BSP configurations
* @param  None
* @retval None
*/

void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA , ENABLE);

  /* Configure SOF VBUS ID DM DP Pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_11 | GPIO_Pin_12;

  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_PinAFConfig(GPIOA,GPIO_PinSource8,GPIO_AF_OTG1_FS) ;
  GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_OTG1_FS) ;
  GPIO_PinAFConfig(GPIOA,GPIO_PinSource11,GPIO_AF_OTG1_FS) ;
  GPIO_PinAFConfig(GPIOA,GPIO_PinSource12,GPIO_AF_OTG1_FS) ;

  /* ID pin has to be an input for Host/Device switching during runtime */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_OTG1_FS) ;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE) ;

  EXTI_ClearITPendingBit(EXTI_Line0);
}

/**
* @brief  USB_OTG_BSP_EnableInterrupt
*         Enabele USB Global interrupt
* @param  None
* @retval None
*/
void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE *pdev)
{
  MIOS32_IRQ_Install(OTG_FS_IRQn, MIOS32_IRQ_USB_PRIORITY);
}

/**
  * @brief  This function handles OTG_FS Handler.
  * @param  None
  * @retval None
  */
void OTG_FS_IRQHandler(void)
{
#ifndef MIOS32_DONT_USE_USB_HOST
  if( USB_OTG_IsHostMode(&USB_OTG_dev) ) {
    USBH_OTG_ISR_Handler(&USB_OTG_dev);
  } else {
    USBD_OTG_ISR_Handler(&USB_OTG_dev);
  }

  STM32_USBO_OTG_ISR_Handler(&USB_OTG_dev);
#else
  USBD_OTG_ISR_Handler(&USB_OTG_dev);
#endif
}

/**
* @brief  USB_OTG_BSP_uDelay
*         This function provides delay time in micro sec
* @param  usec : Value of delay required in micro sec
* @retval None
*/
void USB_OTG_BSP_uDelay (const uint32_t usec)
{
  MIOS32_DELAY_Wait_uS(usec);
}


/**
* @brief  USB_OTG_BSP_mDelay
*          This function provides delay time in milli sec
* @param  msec : Value of delay required in milli sec
* @retval None
*/
void USB_OTG_BSP_mDelay (const uint32_t msec)
{
  USB_OTG_BSP_uDelay(msec * 1000);
}


/**
  * @brief  USB_OTG_BSP_ConfigVBUS
  *         Configures the IO for the Vbus and OverCurrent
  * @param  None
  * @retval None
  */
void  USB_OTG_BSP_ConfigVBUS(USB_OTG_CORE_HANDLE *pdev)
{
#ifndef MIOS32_DONT_USE_USB_HOST
  GPIO_InitTypeDef GPIO_InitStructure; 
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);  
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  /* By Default, DISABLE is needed on output of the Power Switch */
  GPIO_SetBits(GPIOC, GPIO_Pin_0);
  
  USB_OTG_BSP_mDelay(200);   /* Delay is need for stabilising the Vbus Low in Reset Condition, when Vbus=1 and Reset-button is pressed by user */
#endif
}

/**
  * @brief  BSP_Drive_VBUS
  *         Drives the Vbus signal through IO
  * @param  state : VBUS states
  * @retval None
  */

void USB_OTG_BSP_DriveVBUS(USB_OTG_CORE_HANDLE *pdev, uint8_t state)
{
#ifndef MIOS32_DONT_USE_USB_HOST
  /*
  On-chip 5 V VBUS generation is not supported. For this reason, a charge pump 
  or, if 5 V are available on the application board, a basic power switch, must 
  be added externally to drive the 5 V VBUS line. The external charge pump can 
  be driven by any GPIO output. When the application decides to power on VBUS 
  using the chosen GPIO, it must also set the port power bit in the host port 
  control and status register (PPWR bit in OTG_FS_HPRT).
  
  Bit 12 PPWR: Port power
  The application uses this field to control power to this port, and the core 
  clears this bit on an overcurrent condition.
  */
  if (0 == state)
  { 
    /* DISABLE is needed on output of the Power Switch */
    GPIO_SetBits(GPIOC, GPIO_Pin_0);
  }
  else
  {
    /*ENABLE the Power Switch by driving the Enable LOW */
    GPIO_ResetBits(GPIOC, GPIO_Pin_0);
  }
#endif
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// USB Device Class Layer
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/**
  * @brief  MIOS32_USB_CLASS_Init
  *         Initilaize the CDC interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  MIOS32_USB_CLASS_Init (void  *pdev, 
				       uint8_t cfgidx)
{
#ifndef MIOS32_DONT_USE_USB_MIDI
  // Open Endpoints
  DCD_EP_Open(pdev, MIOS32_USB_MIDI_DATA_OUT_EP, MIOS32_USB_MIDI_DATA_OUT_SIZE, USB_OTG_EP_BULK);
  DCD_EP_Open(pdev, MIOS32_USB_MIDI_DATA_IN_EP, MIOS32_USB_MIDI_DATA_IN_SIZE, USB_OTG_EP_BULK);

  // configuration for next transfer
  DCD_EP_PrepareRx(&USB_OTG_dev,
		   MIOS32_USB_MIDI_DATA_OUT_EP,
		   (uint8_t*)(USB_rx_buffer),
		   MIOS32_USB_MIDI_DATA_OUT_SIZE);
#endif

  return USBD_OK;
}

/**
  * @brief  MIOS32_USB_CLASS_Init
  *         DeInitialize the CDC layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  MIOS32_USB_CLASS_DeInit (void  *pdev, 
					 uint8_t cfgidx)
{
#ifndef MIOS32_DONT_USE_USB_MIDI
  // Close Endpoints
  DCD_EP_Close(pdev, MIOS32_USB_MIDI_DATA_OUT_EP);
  DCD_EP_Close(pdev, MIOS32_USB_MIDI_DATA_IN_EP);
#endif
  
  return USBD_OK;
}

/**
  * @brief  MIOS32_USB_CLASS_Setup
  *         Handle the CDC specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  MIOS32_USB_CLASS_Setup (void  *pdev, 
					USB_SETUP_REQ *req)
{
  // not relevant for USB MIDI

  return USBD_OK;
}

/**
  * @brief  MIOS32_USB_CLASS_EP0_RxReady
  *         Data received on control endpoint
  * @param  pdev: device device instance
  * @retval status
  */
static uint8_t  MIOS32_USB_CLASS_EP0_RxReady (void  *pdev)
{ 
  // not relevant for USB MIDI
  
  return USBD_OK;
}

/**
  * @brief  MIOS32_USB_CLASS_DataIn
  *         Data sent on non-control IN endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  MIOS32_USB_CLASS_DataIn (void *pdev, uint8_t epnum)
{
#ifndef MIOS32_DONT_USE_USB_MIDI
  if( epnum == (MIOS32_USB_MIDI_DATA_IN_EP & 0x7f) )
    MIOS32_USB_MIDI_EP1_IN_Callback(epnum, 0); // parameters not relevant for STM32F4
#endif
  
  return USBD_OK;
}

/**
  * @brief  MIOS32_USB_CLASS_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  MIOS32_USB_CLASS_DataOut (void *pdev, uint8_t epnum)
{      
#ifndef MIOS32_DONT_USE_USB_MIDI
  if( epnum == MIOS32_USB_MIDI_DATA_OUT_EP )
    MIOS32_USB_MIDI_EP2_OUT_Callback(epnum, 0); // parameters not relevant for STM32F4
#endif

  return USBD_OK;
}

/**
  * @brief  MIOS32_USB_CLASS_GetCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *MIOS32_USB_CLASS_GetCfgDesc (uint8_t speed, uint16_t *length)
{
#if !defined(MIOS32_DONT_USE_USB_MIDI) && MIOS32_USB_MIDI_NUM_PORTS > 1
  if( MIOS32_USB_ForceSingleUSB() ) {
    *length = sizeof (MIOS32_USB_ConfigDescriptor_SingleUSB);
    return (uint8_t *)MIOS32_USB_ConfigDescriptor_SingleUSB;
  }
#endif
  *length = sizeof (MIOS32_USB_ConfigDescriptor);
  return (uint8_t *)MIOS32_USB_ConfigDescriptor;
}


static uint8_t *MIOS32_USB_CLASS_GetUsrStrDesc(uint8_t speed, uint8_t index, uint16_t *length)
{
  return USBD_USR_ProductStrDescriptor(speed, length);
}


/* CDC interface class callbacks structure */
static const USBD_Class_cb_TypeDef MIOS32_USB_CLASS_cb = 
{
  MIOS32_USB_CLASS_Init,
  MIOS32_USB_CLASS_DeInit,
  MIOS32_USB_CLASS_Setup,
  NULL,                 /* EP0_TxSent, */
  MIOS32_USB_CLASS_EP0_RxReady,
  MIOS32_USB_CLASS_DataIn,
  MIOS32_USB_CLASS_DataOut,
  NULL, // MIOS32_USB_CLASS_SOF // not used
  NULL,
  NULL,     
  MIOS32_USB_CLASS_GetCfgDesc,
  MIOS32_USB_CLASS_GetUsrStrDesc,
};




/////////////////////////////////////////////////////////////////////////////
//! Initializes USB interface
//! \param[in] mode
//!   <UL>
//!     <LI>if 0, USB peripheral won't be initialized if this has already been done before
//!     <LI>if 1, USB peripheral re-initialisation will be forced
//!     <LI>if 2, USB peripheral re-initialisation will be forced, STM32 driver hooks won't be overwritten.<BR>
//!         This mode can be used for a local USB driver which installs it's own hooks during runtime.<BR>
//!         The application can switch back to MIOS32 drivers (e.g. MIOS32_USB_MIDI) by calling MIOS32_USB_Init(1)
//!   </UL>
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_Init(u32 mode)
{
  // currently only mode 0..2 supported
  if( mode >= 3 )
    return -1; // unsupported mode

  u8 usb_is_initialized = MIOS32_USB_IsInitialized();

#ifndef MIOS32_DONT_USE_USB_HOST  
  /* Init Host Library */
  USBH_Init(&USB_OTG_dev, 
            USB_OTG_FS_CORE_ID,
            &USB_Host,
            (USBH_Class_cb_TypeDef *)&MIOS32_MIDI_USBH_Callbacks, 
            (USBH_Usr_cb_TypeDef *)&USBH_USR_Callbacks);
#endif

  // change connection state to disconnected
  USBD_USR_DeviceDisconnected();

  if( mode == 0 && usb_is_initialized ) {
    // if mode == 0: no reconnection, important for BSL!

#if 0
    // init USB device and driver
    USBD_Init(&USB_OTG_dev,
	      USB_OTG_FS_CORE_ID,
	      (USBD_DEVICE *)&USR_desc,
	      (USBD_Class_cb_TypeDef *)&MIOS32_USB_CLASS_cb,
	      (USBD_Usr_cb_TypeDef *)&USBD_USR_Callbacks);
#else

    // don't run complete driver init sequence to ensure that the connection doesn't get lost!

    // phys interface re-initialisation (just to ensure)
    USB_OTG_BSP_Init(&USB_OTG_dev);

    // USBD_Init sets these pointer in the handle
    USB_OTG_dev.dev.class_cb = (USBD_Class_cb_TypeDef *)&MIOS32_USB_CLASS_cb;
    USB_OTG_dev.dev.usr_cb = (USBD_Usr_cb_TypeDef *)&USBD_USR_Callbacks;
    USB_OTG_dev.dev.usr_device = (USBD_DEVICE *)&USR_desc;

    // some additional handle init stuff which doesn't hurt
    USB_OTG_SelectCore(&USB_OTG_dev, USB_OTG_FS_CORE_ID);

    // enable interrupts
    USB_OTG_EnableGlobalInt(&USB_OTG_dev);
    USB_OTG_EnableDevInt(&USB_OTG_dev);
    USB_OTG_BSP_EnableInterrupt(&USB_OTG_dev);
#endif

    // select configuration
    USB_OTG_dev.dev.device_config = 1;
    USB_OTG_dev.dev.device_status = USB_OTG_CONFIGURED;

    // init endpoints
    MIOS32_USB_CLASS_Init(&USB_OTG_dev, 1);

    // assume that device is (still) configured
    USBD_USR_DeviceConfigured();
  } else {
    // init USB device and driver
    USBD_Init(&USB_OTG_dev,
	      USB_OTG_FS_CORE_ID,
	      (USBD_DEVICE *)&USR_desc,
	      (USBD_Class_cb_TypeDef *)&MIOS32_USB_CLASS_cb,
	      (USBD_Usr_cb_TypeDef *)&USBD_USR_Callbacks);

    // disconnect device
    DCD_DevDisconnect(&USB_OTG_dev);

    // wait 50 mS
    USB_OTG_BSP_mDelay(50);

    // connect device
    DCD_DevConnect(&USB_OTG_dev);
  }

#ifndef MIOS32_DONT_USE_USB_HOST
  // switch to host or device mode depending on the ID pin
  if( MIOS32_SYS_STM_PINGET(GPIOA, GPIO_Pin_10) ) {
    USB_OTG_SetCurrentMode(&USB_OTG_dev, DEVICE_MODE);
  } else {
    USB_OTG_DriveVbus(&USB_OTG_dev, 1);
    USB_OTG_SetCurrentMode(&USB_OTG_dev, HOST_MODE);
  }
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Allows to query, if the USB interface has already been initialized.<BR>
//! This function is used by the bootloader to avoid a reconnection, it isn't
//! relevant for typical applications!
//! \return 1 if USB already initialized, 0 if not initialized
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_IsInitialized(void)
{
  // we assume that initialisation has been done when B-Session valid flag is set
  __IO USB_OTG_GREGS *GREGS = (USB_OTG_GREGS *)(USB_OTG_FS_BASE_ADDR + USB_OTG_CORE_GLOBAL_REGS_OFFSET);
  return (GREGS->GOTGCTL & (1 << 19)) ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
//! \returns != 0 if a single USB port has been forced in the bootloader
//! config section
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_ForceSingleUSB(void)
{
  u8 *single_usb_confirm = (u8 *)MIOS32_SYS_ADDR_SINGLE_USB_CONFIRM;
  u8 *single_usb = (u8 *)MIOS32_SYS_ADDR_SINGLE_USB;
  if( *single_usb_confirm == 0x42 && *single_usb < 0x80 )
    return *single_usb;

  return 0;
}

//! \}

#endif /* MIOS32_DONT_USE_USB */
