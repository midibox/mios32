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

#include <usb_lib.h>


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


// ISTR events
// mask defining which events has to be handled by the device application software
//#define IMR_MSK (CNTR_CTRM  | CNTR_SOFM  | CNTR_RESETM )
#define IMR_MSK (CNTR_CTRM | CNTR_RESETM )
// TK: disabled SOF interrupt, since it isn't really used and disturbs debugging


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef enum _RESUME_STATE
{
  RESUME_EXTERNAL,
  RESUME_INTERNAL,
  RESUME_LATER,
  RESUME_WAIT,
  RESUME_START,
  RESUME_ON,
  RESUME_OFF,
  RESUME_ESOF
} RESUME_STATE;

typedef enum _DEVICE_STATE
{
  UNCONNECTED,
  ATTACHED,
  POWERED,
  SUSPENDED,
  ADDRESSED,
  CONFIGURED
} DEVICE_STATE;


/////////////////////////////////////////////////////////////////////////////
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


/////////////////////////////////////////////////////////////////////////////
// USB Standard Device Descriptor
/////////////////////////////////////////////////////////////////////////////
#define MIOS32_USB_SIZ_DEVICE_DESC 18
static const u8 MIOS32_USB_DeviceDescriptor[MIOS32_USB_SIZ_DEVICE_DESC] = {
  (u8)(MIOS32_USB_SIZ_DEVICE_DESC&0xff), // Device Descriptor length
  DSCR_DEVICE,			// Decriptor type
  (u8)(0x0200 & 0xff),		// Specification Version (BCD, LSB)
  (u8)(0x0200 >> 8),		// Specification Version (BCD, MSB)
#if 1
  0x02,				// Device class "Communication"   -- required for MacOS to find the COM device. Audio Device works fine in parallel to this
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
  0x01,				// Manufacturer string index
  0x02,				// Product string index
  0x03,				// Serial number string index
  0x01 				// Number of configurations
};


/////////////////////////////////////////////////////////////////////////////
// USB Config Descriptor
/////////////////////////////////////////////////////////////////////////////

// TODO: generate the config descriptor via software
// this has to be done in *MIOS32_USB_CB_GetConfigDescriptor()
// Problem: it would increase stack or static RAM consumption

static const u8 MIOS32_USB_ConfigDescriptor[MIOS32_USB_SIZ_CONFIG_DESC] = {
  // Configuration Descriptor
  9,				// Descriptor length
  DSCR_CONFIG,			// Descriptor type
  (MIOS32_USB_SIZ_CONFIG_DESC) & 0xff,  // Config + End Points length (LSB)
  (MIOS32_USB_SIZ_CONFIG_DESC) >> 8,    // Config + End Points length (LSB)
  MIOS32_USB_NUM_INTERFACES,    // Number of interfaces
  0x01,				// Configuration Value
  0x00,				// Configuration string
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
  0x02,				// Interface descriptor string index

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
  0x02,				// Interface descriptor string index

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
  0x01,				// Out Endpoint 1
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
  0x81,				// In Endpoint 1
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



  ///////////////////////////////////////////////////////////////////////////
  // CDC
  ///////////////////////////////////////////////////////////////////////////
#ifdef MIOS32_USE_USB_COM

  /*Interface Descriptor*/
  0x09,   /* bLength: Interface Descriptor size */
  DSCR_INTRFC,  /* bDescriptorType: Interface */
  /* Interface descriptor type */
  MIOS32_USB_COM_CC_INTERFACE_IX,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x01,   /* bNumEndpoints: One endpoints used */
  0x02,   /* bInterfaceClass: Communication Interface Class */
  0x02,   /* bInterfaceSubClass: Abstract Control Model */
  0x01,   /* bInterfaceProtocol: Common AT commands */
  0x00,   /* iInterface: */

  /*Header Functional Descriptor*/
  0x05,   /* bLength: Endpoint Descriptor size */
  CS_INTERFACE, /* bDescriptorType: CS_INTERFACE */
  0x00,   /* bDescriptorSubtype: Header Func Desc */
  0x10,   /* bcdCDC: spec release number */
  0x01,

  /*Call Managment Functional Descriptor*/
  0x05,   /* bFunctionLength */
  CS_INTERFACE, /* bDescriptorType: CS_INTERFACE */
  0x01,   /* bDescriptorSubtype: Call Management Func Desc */
  0x00,   /* bmCapabilities: D0+D1 */
  MIOS32_USB_COM_CD_INTERFACE_IX,   /* bDataInterface */

  /*ACM Functional Descriptor*/
  0x04,   /* bFunctionLength */
  CS_INTERFACE, /* bDescriptorType: CS_INTERFACE */
  0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,   /* bmCapabilities */

  /*Union Functional Descriptor*/
  0x05,   /* bFunctionLength */
  CS_INTERFACE, /* bDescriptorType: CS_INTERFACE */
  0x06,   /* bDescriptorSubtype: Union func desc */
  MIOS32_USB_COM_CC_INTERFACE_IX,   /* bMasterInterface: Communication class interface */
  MIOS32_USB_COM_CD_INTERFACE_IX,   /* bSlaveInterface0: Data Class Interface */

  /*Endpoint 2 Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  DSCR_ENDPNT,  /* bDescriptorType: Endpoint */
  0x82,   /* bEndpointAddress: (IN2) */
  0x03,   /* bmAttributes: Interrupt */
  MIOS32_USB_COM_INT_IN_SIZE,      /* wMaxPacketSize: */
  0x00,
  0xFF,   /* bInterval: */


  /*Data class interface descriptor*/
  0x09,   /* bLength: Endpoint Descriptor size */
  DSCR_INTRFC,  /* bDescriptorType: */
  MIOS32_USB_COM_CD_INTERFACE_IX,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0x0A,   /* bInterfaceClass: CDC */
  0x00,   /* bInterfaceSubClass: */
  0x00,   /* bInterfaceProtocol: */
  0x00,   /* iInterface: */

  /*Endpoint 3 Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  DSCR_ENDPNT,  /* bDescriptorType: Endpoint */
  0x03,   /* bEndpointAddress: (OUT3) */
  0x02,   /* bmAttributes: Bulk */
  MIOS32_USB_COM_DATA_OUT_SIZE,             /* wMaxPacketSize: */
  0x00,
  0x00,   /* bInterval: ignore for Bulk transfer */

  /*Endpoint 4 Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  DSCR_ENDPNT,  /* bDescriptorType: Endpoint */
  0x84,   /* bEndpointAddress: (IN4) */
  0x02,   /* bmAttributes: Bulk */
  MIOS32_USB_COM_DATA_IN_SIZE,             /* wMaxPacketSize: */
  0x00,
  0x00,    /* bInterval */

#endif /* MIOS32_USE_USB_COM */
};


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static void MIOS32_USB_CB_Init(void);
static void MIOS32_USB_CB_Reset(void);
static void MIOS32_USB_CB_SetConfiguration(void);
static void MIOS32_USB_CB_SetDeviceAddress (void);
static void MIOS32_USB_CB_Status_In(void);
static void MIOS32_USB_CB_Status_Out(void);
static RESULT MIOS32_USB_CB_Data_Setup(u8 RequestNo);
static RESULT MIOS32_USB_CB_NoData_Setup(u8 RequestNo);
static u8 *MIOS32_USB_CB_GetDeviceDescriptor(u16 Length);
static u8 *MIOS32_USB_CB_GetConfigDescriptor(u16 Length);
static u8 *MIOS32_USB_CB_GetStringDescriptor(u16 Length);
static RESULT MIOS32_USB_CB_Get_Interface_Setting(u8 Interface, u8 AlternateSetting);


/////////////////////////////////////////////////////////////////////////////
// USB callback vectors
/////////////////////////////////////////////////////////////////////////////

void (*pEpInt_IN[7])(void) = {
#ifndef MIOS32_DONT_USE_USB_MIDI
  MIOS32_USB_MIDI_EP1_IN_Callback,   // IN EP1
#else
  NOP_Process,                       // IN EP1
#endif

#ifdef MIOS32_USE_USB_COM
  NOP_Process,                       // IN EP2
  NOP_Process,                       // IN EP3
  MIOS32_USB_COM_EP4_IN_Callback,    // IN EP4
#else
  NOP_Process,                       // IN EP2
  NOP_Process,                       // IN EP3
  NOP_Process,                       // IN EP4
#endif

  NOP_Process,                       // IN EP5
  NOP_Process,                       // IN EP6
  NOP_Process                        // IN EP7
};

void (*pEpInt_OUT[7])(void) = {
#ifndef MIOS32_DONT_USE_USB_MIDI
  MIOS32_USB_MIDI_EP1_OUT_Callback,  // OUT EP1
#else
  NOP_Process,                       // OUT EP1
#endif
#ifdef MIOS32_USE_USB_COM
  NOP_Process,                       // OUT EP2
  MIOS32_USB_COM_EP3_OUT_Callback,   // OUT EP3
  NOP_Process,                       // OUT EP4
#else
  NOP_Process,                       // OUT EP2
  NOP_Process,                       // OUT EP3
  NOP_Process,                       // OUT EP4
#endif
  NOP_Process,                       // OUT EP5
  NOP_Process,                       // OUT EP6
  NOP_Process                        // OUT EP7
};

DEVICE Device_Table = {
  MIOS32_USB_EP_NUM,
  1
};

DEVICE_PROP Device_Property = {
  MIOS32_USB_CB_Init,
  MIOS32_USB_CB_Reset,
  MIOS32_USB_CB_Status_In,
  MIOS32_USB_CB_Status_Out,
  MIOS32_USB_CB_Data_Setup,
  MIOS32_USB_CB_NoData_Setup,
  MIOS32_USB_CB_Get_Interface_Setting,
  MIOS32_USB_CB_GetDeviceDescriptor,
  MIOS32_USB_CB_GetConfigDescriptor,
  MIOS32_USB_CB_GetStringDescriptor,
  0,
  0x40 /*MAX PACKET SIZE*/
};

USER_STANDARD_REQUESTS User_Standard_Requests = {
  NOP_Process, // MIOS32_USB_CB_GetConfiguration,
  MIOS32_USB_CB_SetConfiguration,
  NOP_Process, // MIOS32_USB_CB_GetInterface,
  NOP_Process, // MIOS32_USB_CB_SetInterface,
  NOP_Process, // MIOS32_USB_CB_GetStatus,
  NOP_Process, // MIOS32_USB_CB_ClearFeature,
  NOP_Process, // MIOS32_USB_CB_SetEndPointFeature,
  NOP_Process, // MIOS32_USB_CB_SetDeviceFeature,
  MIOS32_USB_CB_SetDeviceAddress
};


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// global interrupt status (also used by USB driver, therefore extern)
volatile u16 wIstr;

// USB device status
static vu32 bDeviceState = UNCONNECTED;


/////////////////////////////////////////////////////////////////////////////
//! Initializes USB interface
//! \param[in] mode
//!   <UL>
//!     <LI>if 0, USB peripheral won't be initialized if this has already been done before
//!     <LI>if 1, USB peripheral re-initialisation will be forced
//!   </UL>
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_Init(u32 mode)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);

  // currently only mode 0 and 1 supported
  if( mode >= 2 )
    return -1; // unsupported mode

  // change connection state to disconnected
#ifndef MIOS32_DONT_USE_USB_MIDI
  MIOS32_USB_MIDI_ChangeConnectionState(0);
#endif
#ifdef MIOS32_USE_USB_COM
  MIOS32_USB_COM_ChangeConnectionState(0);
#endif

  // if mode == 0: don't initialize USB if not required (important for BSL)
  u8 usb_configured = 0;
  if( mode == 0 && MIOS32_USB_IsInitialized() ) {
    usb_configured = 1;
  } else {

#ifdef MIOS32_BOARD_STM32_PRIMER
    // configure USB disconnect pin
    // STM32 Primer: pin B12
    // first we hold it low for ca. 50 mS to force a re-enumeration
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    u32 delay;
    for(delay=0; delay<200000; ++delay) // produces a delay of ca. 50 mS @ 72 MHz (measured with scope)
      GPIOA->BRR = GPIO_Pin_12; // force pin to 0 (without this dummy code, an "empty" for loop could be removed by the compiler)

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
#else
    // using a "dirty" method to force a re-enumeration:
    // force DPM (Pin PA12) low for ca. 10 mS before USB Tranceiver will be enabled
    // this overrules the external Pull-Up at PA12, and at least Windows & MacOS will enumerate again
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    u32 delay;
    for(delay=0; delay<200000; ++delay) // produces a delay of ca. 50 mS @ 72 MHz (measured with scope)
      GPIOA->BRR = GPIO_Pin_12; // force pin to 0 (without this dummy code, an "empty" for loop could be removed by the compiler)
#endif
  }

  // remaining initialisation done in STM32 USB driver
  USB_Init();

  if( usb_configured ) {
    pInformation->Current_Feature = MIOS32_USB_ConfigDescriptor[7];
    pInformation->Current_Configuration = 1;
    pUser_Standard_Requests->User_SetConfiguration();
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Interrupt handler for USB
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
void USB_LP_CAN1_RX0_IRQHandler(void)
{
  wIstr = _GetISTR();

  if( wIstr & ISTR_RESET & wInterrupt_Mask ) {
    _SetISTR((u16)CLR_RESET);
    Device_Property.Reset();
  }

  if( wIstr & ISTR_SOF & wInterrupt_Mask ) {
    _SetISTR((u16)CLR_SOF);
  }

  if( wIstr & ISTR_CTR & wInterrupt_Mask ) {
    // servicing of the endpoint correct transfer interrupt
    // clear of the CTR flag into the sub
    CTR_LP();
  }
}


/////////////////////////////////////////////////////////////////////////////
//! Allows to query, if the USB interface has already been initialized.<BR>
//! This function is used by the bootloader to avoid a reconnection, it isn't
//! relevant for typical applications!
//! \return 1 if USB already initialized, 0 if not initialized
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_IsInitialized(void)
{
  // we assume that initialisation has been done when endpoint 0 contains a value
  return GetEPType(ENDP0) ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Hooks of STM32 USB library
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////

// init routine
static void MIOS32_USB_CB_Init(void)
{
  u32 delay;
  u16 wRegVal;

  pInformation->Current_Configuration = 0;

  // Connect the device if USB not already initialized

  // CNTR_PWDN = 0
  if( !MIOS32_USB_IsInitialized() ) {
    wRegVal = CNTR_FRES;
    _SetCNTR(wRegVal);

    // according to the reference manual, we have to wait at least for tSTARTUP = 1 uS before releasing reset
    for(delay=0; delay<10; ++delay) GPIOA->BRR = 0; // should be more than sufficient - add some dummy code here to ensure that the compiler doesn't optimize the empty for loop away
    
    // CNTR_FRES = 0
    wInterrupt_Mask = 0;
    _SetCNTR(wInterrupt_Mask);

    // Clear pending interrupts
    _SetISTR(0);

    // Configure USB clock
    // USBCLK = PLLCLK / 1.5
    RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
    // Enable USB clock
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);

    // Set interrupt mask
    // wInterrupt_Mask = CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM;
    wInterrupt_Mask = CNTR_RESETM;
    _SetCNTR(wInterrupt_Mask);

    // USB interrupts initialization
    // clear pending interrupts
    _SetISTR(0);
    wInterrupt_Mask = IMR_MSK;

    // set interrupts mask
    _SetCNTR(wInterrupt_Mask);
  } else {
#ifndef MIOS32_DONT_USE_USB_MIDI
    // release ENDP1 Rx/Tx
    SetEPTxStatus(ENDP1, EP_TX_NAK);
    SetEPRxValid(ENDP1);
#endif
    // clear pending interrupts
    _SetISTR(0);
    // set interrupts mask
    wInterrupt_Mask = IMR_MSK;
    _SetCNTR(wInterrupt_Mask);
  }

  bDeviceState = UNCONNECTED;

#ifndef MIOS32_DONT_USE_USB_MIDI
  // propagate connection state to USB MIDI driver
  MIOS32_USB_MIDI_ChangeConnectionState(0); // not connected
#endif
#ifdef MIOS32_USE_USB_COM
  // propagate connection state to USB MIDI driver
  MIOS32_USB_COM_ChangeConnectionState(0); // not connected
#endif

  // enable USB interrupts (unfortunately shared with CAN Rx0, as either CAN or USB can be used, but not at the same time)
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = MIOS32_IRQ_USB_PRIORITY; // defined in mios32_irq.h
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

// reset routine
static void MIOS32_USB_CB_Reset(void)
{
  // Set MIOS32 Device as not configured
  pInformation->Current_Configuration = 0;

  // Current Feature initialization
  pInformation->Current_Feature = MIOS32_USB_ConfigDescriptor[7];

  // Set MIOS32 Device with the default Interface
  pInformation->Current_Interface = 0;
  SetBTABLE(MIOS32_USB_BTABLE_ADDRESS);

  // Initialize Endpoint 0
  SetEPType(ENDP0, EP_CONTROL);
  SetEPTxStatus(ENDP0, EP_TX_STALL);
  SetEPRxAddr(ENDP0, MIOS32_USB_ENDP0_RXADDR);
  SetEPTxAddr(ENDP0, MIOS32_USB_ENDP0_TXADDR);
  Clear_Status_Out(ENDP0);
  SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
  SetEPRxValid(ENDP0);

#ifndef MIOS32_DONT_USE_USB_MIDI
  // Initialize Endpoint 1
  SetEPType(ENDP1, EP_BULK);

  SetEPTxAddr(ENDP1, MIOS32_USB_ENDP1_TXADDR);
  SetEPTxCount(ENDP1, MIOS32_USB_MIDI_DATA_OUT_SIZE);
  SetEPTxStatus(ENDP1, EP_TX_NAK);

  SetEPRxAddr(ENDP1, MIOS32_USB_ENDP1_RXADDR);
  SetEPRxCount(ENDP1, MIOS32_USB_MIDI_DATA_IN_SIZE);
  SetEPRxValid(ENDP1);
#endif


#ifdef MIOS32_USE_USB_COM
  // Initialize Endpoint 2
  SetEPType(ENDP2, EP_INTERRUPT);
  SetEPTxAddr(ENDP2, MIOS32_USB_ENDP2_TXADDR);
  SetEPRxStatus(ENDP2, EP_RX_DIS);
  SetEPTxStatus(ENDP2, EP_TX_NAK);

  // Initialize Endpoint 3
  SetEPType(ENDP3, EP_BULK);
  SetEPRxAddr(ENDP3, MIOS32_USB_ENDP3_RXADDR);
  SetEPRxCount(ENDP3, MIOS32_USB_COM_DATA_OUT_SIZE);
  SetEPRxStatus(ENDP3, EP_RX_VALID);
  SetEPTxStatus(ENDP3, EP_TX_DIS);

  // Initialize Endpoint 4
  SetEPType(ENDP4, EP_BULK);
  SetEPTxAddr(ENDP4, MIOS32_USB_ENDP4_TXADDR);
  SetEPTxStatus(ENDP4, EP_TX_NAK);
  SetEPRxStatus(ENDP4, EP_RX_DIS);
#endif

  // Set this device to response on default address
  SetDeviceAddress(0);

#ifndef MIOS32_DONT_USE_USB_MIDI
  // propagate connection state to USB MIDI driver
  MIOS32_USB_MIDI_ChangeConnectionState(0); // not connected
#endif
#ifdef MIOS32_USE_USB_COM
  // propagate connection state to USB COM driver
  MIOS32_USB_COM_ChangeConnectionState(0); // not connected
#endif

  bDeviceState = ATTACHED;
}

// update the device state to configured.
static void MIOS32_USB_CB_SetConfiguration(void)
{
  DEVICE_INFO *pInfo = &Device_Info;

  if (pInfo->Current_Configuration != 0) {
#ifndef MIOS32_DONT_USE_USB_MIDI
    // propagate connection state to USB MIDI driver
    MIOS32_USB_MIDI_ChangeConnectionState(1); // connected
#endif
#ifdef MIOS32_USE_USB_COM
    // propagate connection state to USB COM driver
    MIOS32_USB_COM_ChangeConnectionState(1); // connected
#endif

    bDeviceState = CONFIGURED;
  }
}

// update the device state to addressed
static void MIOS32_USB_CB_SetDeviceAddress (void)
{
  bDeviceState = ADDRESSED;
}

// status IN routine
static void MIOS32_USB_CB_Status_In(void)
{
#ifdef MIOS32_USE_USB_COM
  MIOS32_USB_COM_CB_StatusIn();
#endif
}

// status OUT routine
static void MIOS32_USB_CB_Status_Out(void)
{
}

static RESULT MIOS32_USB_CB_Data_Setup(u8 RequestNo)
{
#ifdef MIOS32_USE_USB_COM
  RESULT res;
  if( (res=MIOS32_USB_COM_CB_Data_Setup(RequestNo)) != USB_UNSUPPORT )
    return res;
#endif
  return USB_UNSUPPORT;
}

// handles the non data class specific requests.
static RESULT MIOS32_USB_CB_NoData_Setup(u8 RequestNo)
{
#ifdef MIOS32_USE_USB_COM
  RESULT res;
  if( (res=MIOS32_USB_COM_CB_NoData_Setup(RequestNo)) != USB_UNSUPPORT )
    return res;
#endif

  return USB_UNSUPPORT;
}

// gets the device descriptor.
static u8 *MIOS32_USB_CB_GetDeviceDescriptor(u16 Length)
{
  ONE_DESCRIPTOR desc = {(u8 *)MIOS32_USB_DeviceDescriptor, MIOS32_USB_SIZ_DEVICE_DESC};
  return Standard_GetDescriptorData(Length, &desc);
}

// gets the configuration descriptor.
static u8 *MIOS32_USB_CB_GetConfigDescriptor(u16 Length)
{
  ONE_DESCRIPTOR desc = {(u8 *)MIOS32_USB_ConfigDescriptor, MIOS32_USB_SIZ_CONFIG_DESC};
  return Standard_GetDescriptorData(Length, &desc);
}

// gets the string descriptors according to the needed index
static u8 *MIOS32_USB_CB_GetStringDescriptor(u16 Length)
{
  const u8 vendor_str[] = MIOS32_USB_VENDOR_STR;
  const u8 product_str[] = MIOS32_USB_PRODUCT_STR;

  u8 buffer[200];
  u16 len;
  int i;

  switch( pInformation->USBwValue0 ) {
    case 0: // Language
      // buffer[0] and [1] initialized below
      buffer[2] = 0x09;        // CharSet
      buffer[3] = 0x04;        // U.S.
      len = 4;
      break;

    case 1: // Vendor
      // buffer[0] and [1] initialized below
      for(i=0, len=2; vendor_str[i] != '\0' && len<200; ++i) {
	buffer[len++] = vendor_str[i];
	buffer[len++] = 0;
      }
      break;

    case 2: // Product
      // buffer[0] and [1] initialized below
      for(i=0, len=2; product_str[i] != '\0' && len<200; ++i) {
	buffer[len++] = product_str[i];
	buffer[len++] = 0;
      }
      break;

    case 3: { // Serial Number
        u8 serial_number_str[40];
	if( MIOS32_SYS_SerialNumberGet((char *)serial_number_str) >= 0 ) {
	  for(i=0, len=2; serial_number_str[i] != '\0' && len<200; ++i) {
	    buffer[len++] = serial_number_str[i];
	    buffer[len++] = 0;
	  }
	} else
	  return NULL;
      }
      break;
    default: // string ID not supported
      return NULL;
  }

  buffer[0] = len; // Descriptor Length
  buffer[1] = DSCR_STRING; // Descriptor Type
  ONE_DESCRIPTOR desc = {(u8 *)buffer, len};
  return Standard_GetDescriptorData(Length, &desc);
}

// test the interface and the alternate setting according to the supported one.
static RESULT MIOS32_USB_CB_Get_Interface_Setting(u8 Interface, u8 AlternateSetting)
{
  if( AlternateSetting > 0 ) {
    return USB_UNSUPPORT;
  } else if( Interface >= MIOS32_USB_NUM_INTERFACES ) {
    return USB_UNSUPPORT;
  }

  return USB_SUCCESS;
}

//! \}

#endif /* MIOS32_DONT_USE_USB */
