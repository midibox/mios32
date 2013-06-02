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


// ISTR events
// mask defining which events has to be handled by the device application software
//#define IMR_MSK (CNTR_CTRM  | CNTR_SOFM  | CNTR_RESETM)
#define IMR_MSK (CNTR_CTRM | CNTR_RESETM)
// TK: disabled SOF interrupt, since it isn't really used and disturbs debugging


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

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
// Global Variables used by STM32 USB Driver
// (unfortunately no unique names are used...)
/////////////////////////////////////////////////////////////////////////////

/*  Points to the DEVICE_INFO/DEVICE_PROP_USER_STANDARD_REQUESTS structure of current device */
/*  The purpose of this register is to speed up the execution */
// TK: in addition, this allows to change USB driver during runtime
DEVICE_INFO *pInformation;
DEVICE Device_Table;
DEVICE_PROP *pProperty;
USER_STANDARD_REQUESTS *pUser_Standard_Requests;

// stored in RAM, vectors can be changed on-the-fly
void (*pEpInt_IN[7])(void) = {
  NOP_Process, NOP_Process, NOP_Process, NOP_Process, NOP_Process, NOP_Process, NOP_Process
};

void (*pEpInt_OUT[7])(void) = {
  NOP_Process, NOP_Process, NOP_Process, NOP_Process, NOP_Process, NOP_Process, NOP_Process
};


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
static const u8 MIOS32_USB_DeviceDescriptor[MIOS32_USB_SIZ_DEVICE_DESC] = {
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
  MIOS32_USB_MIDI_DATA_OUT_EP,	// Out Endpoint 2
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

  /*Endpoint 5 Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  DSCR_ENDPNT,  /* bDescriptorType: Endpoint */
  0x85,   /* bEndpointAddress: (IN2) */
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
// Alternative config descriptor with only a single USB MIDI port
// Workaround for some windows installations where multiple MIDI ports don't work
/////////////////////////////////////////////////////////////////////////////

#if !defined(MIOS32_DONT_USE_USB_MIDI) && MIOS32_USB_MIDI_NUM_PORTS > 1
static const u8 MIOS32_USB_ConfigDescriptor_SingleUSB[MIOS32_USB_SIZ_CONFIG_DESC_SINGLE_USB] = {
  // Configuration Descriptor
  9,				// Descriptor length
  DSCR_CONFIG,			// Descriptor type
  (MIOS32_USB_SIZ_CONFIG_DESC_SINGLE_USB) & 0xff,  // Config + End Points length (LSB)
  (MIOS32_USB_SIZ_CONFIG_DESC_SINGLE_USB) >> 8,    // Config + End Points length (LSB)
  MIOS32_USB_NUM_INTERFACES,    // Number of interfaces
  0x01,				// Configuration Value
  0x00,				// Configuration string
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


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

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

static const DEVICE My_Device_Table = {
  MIOS32_USB_EP_NUM,
  1
};

static const DEVICE_PROP My_Device_Property = {
  0, // MIOS32_USB_CB_Init,
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

static const USER_STANDARD_REQUESTS My_User_Standard_Requests = {
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

// USB Device informations
static DEVICE_INFO My_Device_Info;

// USB device status
static vu32 bDeviceState = UNCONNECTED;


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
  u32 delay;

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);

  // currently only mode 0..2 supported
  if( mode >= 3 )
    return -1; // unsupported mode

  // clear all USB interrupt requests
#ifdef STM32F10X_CL
#else
  MIOS32_IRQ_Disable();
  _SetCNTR(0); // Interrupt Mask
  MIOS32_IRQ_Enable();
#endif

  // if mode != 2: install MIOS32 hooks
  // a local driver can install it's own hooks and call MIOS32_USB_Init(2) to force re-enumeration
  if( mode != 2 ) {
    pInformation = &My_Device_Info; // Note: usually no need to duplicate this for external drivers

    // following hooks/pointers should be replaced by external drivers
    memcpy(&Device_Table, (DEVICE *)&My_Device_Table, sizeof(Device_Table));
    pProperty = (DEVICE_PROP *)&My_Device_Property;
    pUser_Standard_Requests = (USER_STANDARD_REQUESTS *)&My_User_Standard_Requests;

#ifndef MIOS32_DONT_USE_USB_MIDI
    pEpInt_IN[0]  = (void*)MIOS32_USB_MIDI_EP1_IN_Callback;  // IN  EP1
    pEpInt_OUT[1] = (void*)MIOS32_USB_MIDI_EP2_OUT_Callback; // OUT EP2
#endif

#ifdef MIOS32_USE_USB_COM
    pEpInt_IN[3]  = (void*)MIOS32_USB_COM_EP4_IN_Callback;  // IN  EP4
    pEpInt_OUT[2] = (void*)MIOS32_USB_COM_EP3_OUT_Callback; // OUT EP3
#endif
  }

  // change connection state to disconnected
#ifndef MIOS32_DONT_USE_USB_MIDI
  MIOS32_USB_MIDI_ChangeConnectionState(0);
#endif
#ifdef MIOS32_USE_USB_COM
  MIOS32_USB_COM_ChangeConnectionState(0);
#endif

  // we don't use USB_Init() anymore for more flexibility
  // e.g. changing USB driver during runtime via MIOS32_USB_Init(2)

  pInformation->ControlState = 2;
  pInformation->Current_Configuration = 0;

  // if mode == 0: don't initialize USB if not required (important for BSL)
  if( mode == 0 && MIOS32_USB_IsInitialized() ) {
#ifdef STM32F10X_CL
    // Perform OTG Device initialization procedure (including EP0 init) again
    // this is unfortunately required since the OTG driver has to initialize internal variables after reset
    OTG_DEV_Init();

    // Init EP1 IN again
    OTG_DEV_EP_Init(EP1_IN, OTG_DEV_EP_TYPE_BULK, MIOS32_USB_MIDI_DATA_IN_SIZE);
  
    // Init EP2 OUT again
    OTG_DEV_EP_Init(EP2_OUT, OTG_DEV_EP_TYPE_BULK, MIOS32_USB_MIDI_DATA_OUT_SIZE);
#else
#ifndef MIOS32_DONT_USE_USB_MIDI
    // release ENDP1 Rx/Tx
    SetEPTxStatus(ENDP1, EP_TX_NAK);
    SetEPRxValid(ENDP2);
#endif
#endif

    pInformation->Current_Feature = MIOS32_USB_ConfigDescriptor[7];
    pInformation->Current_Configuration = 1;
    pUser_Standard_Requests->User_SetConfiguration();

  } else {
#ifdef STM32F10X_CL
    // Select USBCLK source
    RCC_OTGFSCLKConfig(RCC_OTGFSCLKSource_PLLVCO_Div3);

    // Enable the USB clock
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_OTG_FS, ENABLE) ;

    // Perform OTG Device initialization procedure (including EP0 init)
    OTG_DEV_Init();

    // dissconnect device
    USB_DevDisconnect();

    // wait 50 mS
    USB_OTG_BSP_uDelay(50000);
    
    // connect device
    USB_DevConnect();
#else
    // force USB reset and power-down (this will also release the USB pins for direct GPIO control)
    _SetCNTR(CNTR_FRES | CNTR_PDWN);

#ifdef MIOS32_BOARD_STM32_PRIMER
    // configure USB disconnect pin
    // STM32 Primer: pin B12
    // first we hold it low for ca. 50 mS to force a re-enumeration
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

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

    for(delay=0; delay<200000; ++delay) // produces a delay of ca. 50 mS @ 72 MHz (measured with scope)
      GPIOA->BRR = GPIO_Pin_12; // force pin to 0 (without this dummy code, an "empty" for loop could be removed by the compiler)
#endif

    // release power-down, still hold reset
    _SetCNTR(CNTR_PDWN);

    // according to the reference manual, we have to wait at least for tSTARTUP = 1 uS before releasing reset
    for(delay=0; delay<10; ++delay) GPIOA->BRR = 0; // should be more than sufficient - add some dummy code here to ensure that the compiler doesn't optimize the empty for loop away

    // CNTR_FRES = 0
    _SetCNTR(0);

    // Clear pending interrupts
    _SetISTR(0);

    // Configure USB clock
    // USBCLK = PLLCLK / 1.5
    RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
    // Enable USB clock
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
#endif /* STM32F10X_CL */

  }

  // don't set interrupt mask on custom driver installation
  if( mode != 2 ) {
#ifdef STM32F10X_CL
    OTGD_FS_EnableGlobalInt();
#else
    // clear pending interrupts (again)
    _SetISTR(0);

    // set interrupts mask
    _SetCNTR(IMR_MSK); // Interrupt mask
#endif
  }

  bDeviceState = UNCONNECTED;

#ifdef STM32F10X_CL
  // Enable the USB interrupts
  MIOS32_IRQ_Install(OTG_FS_IRQn, MIOS32_IRQ_USB_PRIORITY);
#else
  // enable USB interrupts (unfortunately shared with CAN Rx0, as either CAN or USB can be used, but not at the same time)
  MIOS32_IRQ_Install(USB_LP_CAN1_RX0_IRQn, MIOS32_IRQ_USB_PRIORITY);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Interrupt handler for USB
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
#ifdef STM32F10X_CL
u32 OTG_FS_IRQHandler(void)
{
  // based on STM example code
  USB_OTG_GINTSTS_TypeDef gintr_status;
  u32 retval = 0;

  if (USBD_FS_IsDeviceMode()) /* ensure that we are in device mode */
  {
    gintr_status.d32 = OTGD_FS_ReadCoreItr();

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    
    /* If there is no interrupt pending exit the interrupt routine */
    if (!gintr_status.d32)
    {
      return 0;
    }

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Early Suspend interrupt */ 
#ifdef INTR_ERLYSUSPEND
    if (gintr_status.b.erlysuspend)
    {
      retval |= OTGD_FS_Handle_EarlySuspend_ISR();
    }
#endif /* INTR_ERLYSUSPEND */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* End of Periodic Frame interrupt */
#ifdef INTR_EOPFRAME    
    if (gintr_status.b.eopframe)
    {
      retval |= OTGD_FS_Handle_EOPF_ISR();
    }
#endif /* INTR_EOPFRAME */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* Non Periodic Tx FIFO Emty interrupt */
#ifdef INTR_NPTXFEMPTY    
    if (gintr_status.b.nptxfempty)
    {
      retval |= OTGD_FS_Handle_NPTxFE_ISR();
    }
#endif /* INTR_NPTXFEMPTY */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Wakeup or RemoteWakeup interrupt */
#ifdef INTR_WKUPINTR    
    if (gintr_status.b.wkupintr)
    {   
      retval |= OTGD_FS_Handle_Wakeup_ISR();
    }
#endif /* INTR_WKUPINTR */   
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* Suspend interrupt */
#ifdef INTR_USBSUSPEND
    if (gintr_status.b.usbsuspend)
    { 
#if 0
      /* check if SUSPEND is possible */
      if (fSuspendEnabled)
      {
        Suspend();
      }
      else
      {
        /* if not possible then resume after xx ms */
        Resume(RESUME_LATER); /* This case shouldn't happen in OTG Device mode because 
        there's no ESOF interrupt to increment the ResumeS.bESOFcnt in the Resume state machine */
      }
#endif
            
      retval |= OTGD_FS_Handle_USBSuspend_ISR();
    }
#endif /* INTR_USBSUSPEND */

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* Start of Frame interrupt */
#ifdef INTR_SOFINTR    
    if (gintr_status.b.sofintr)
    {
#if 0
      /* Update the frame number variable */
      bIntPackSOF++;
#endif
      
      retval |= OTGD_FS_Handle_Sof_ISR();
    }
#endif /* INTR_SOFINTR */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* Receive FIFO Queue Status Level interrupt */
#ifdef INTR_RXSTSQLVL
    if (gintr_status.b.rxstsqlvl)
    {
      retval |= OTGD_FS_Handle_RxStatusQueueLevel_ISR();
    }
#endif /* INTR_RXSTSQLVL */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* Enumeration Done interrupt */
#ifdef INTR_ENUMDONE
    if (gintr_status.b.enumdone)
    {
      retval |= OTGD_FS_Handle_EnumDone_ISR();
    }
#endif /* INTR_ENUMDONE */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* Reset interrutp */
#ifdef INTR_USBRESET
    if (gintr_status.b.usbreset)
    {
      retval |= OTGD_FS_Handle_UsbReset_ISR();
    }    
#endif /* INTR_USBRESET */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* IN Endpoint interrupt */
#ifdef INTR_INEPINTR
    if (gintr_status.b.inepint)
    {
      retval |= OTGD_FS_Handle_InEP_ISR();
    }
#endif /* INTR_INEPINTR */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* OUT Endpoint interrupt */
#ifdef INTR_OUTEPINTR
    if (gintr_status.b.outepintr)
    {
      retval |= OTGD_FS_Handle_OutEP_ISR();
    }
#endif /* INTR_OUTEPINTR */    
 
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Mode Mismatch interrupt */
#ifdef INTR_MODEMISMATCH
    if (gintr_status.b.modemismatch)
    {
      retval |= OTGD_FS_Handle_ModeMismatch_ISR();
    }
#endif /* INTR_MODEMISMATCH */  

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Global IN Endpoints NAK Effective interrupt */
#ifdef INTR_GINNAKEFF
    if (gintr_status.b.ginnakeff)
    {
      retval |= OTGD_FS_Handle_GInNakEff_ISR();
    }
#endif /* INTR_GINNAKEFF */  

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Global OUT Endpoints NAK effective interrupt */
#ifdef INTR_GOUTNAKEFF
    if (gintr_status.b.goutnakeff)
    {
      retval |= OTGD_FS_Handle_GOutNakEff_ISR();
    }
#endif /* INTR_GOUTNAKEFF */  

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Isochrounous Out packet Dropped interrupt */
#ifdef INTR_ISOOUTDROP
    if (gintr_status.b.isooutdrop)
    {
      retval |= OTGD_FS_Handle_IsoOutDrop_ISR();
    }
#endif /* INTR_ISOOUTDROP */  

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Endpoint Mismatch error interrupt */
#ifdef INTR_EPMISMATCH
    if (gintr_status.b.epmismatch)
    {
      retval |= OTGD_FS_Handle_EPMismatch_ISR();
    }
#endif /* INTR_EPMISMATCH */  

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Incomplete Isochrous IN tranfer error interrupt */
#ifdef INTR_INCOMPLISOIN
    if (gintr_status.b.incomplisoin)
    {
      retval |= OTGD_FS_Handle_IncomplIsoIn_ISR();
    }
#endif /* INTR_INCOMPLISOIN */  

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Incomplete Isochrous OUT tranfer error interrupt */
#ifdef INTR_INCOMPLISOOUT
    if (gintr_status.b.outepintr)
    {
      retval |= OTGD_FS_Handle_IncomplIsoOut_ISR();
    }
#endif /* INTR_INCOMPLISOOUT */  
  
  }
  return retval;
}
#else
void USB_LP_CAN1_RX0_IRQHandler(void)
{
  u16 wIstr = _GetISTR();

  if( wIstr & ISTR_RESET ) {
    _SetISTR((u16)CLR_RESET);
    pProperty->Reset();
  }

  if( wIstr & ISTR_SOF ) {
    _SetISTR((u16)CLR_SOF);
  }

  if( wIstr & ISTR_CTR ) {
    // servicing of the endpoint correct transfer interrupt
    // clear of the CTR flag into the sub
    CTR_LP();
  }
}
#endif


/////////////////////////////////////////////////////////////////////////////
//! Allows to query, if the USB interface has already been initialized.<BR>
//! This function is used by the bootloader to avoid a reconnection, it isn't
//! relevant for typical applications!
//! \return 1 if USB already initialized, 0 if not initialized
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_IsInitialized(void)
{
#ifdef STM32F10X_CL
  // we assume that initialisation has been done when B-Session valid flag is set
  __IO USB_OTG_GREGS *GREGS = (USB_OTG_GREGS *)(USB_OTG_FS_BASE_ADDR + USB_OTG_CORE_GLOBAL_REGS_OFFSET);
  return (GREGS->GOTGCTL & (1 << 19));
#else
  // we assume that initialisation has been done when endpoint 0 contains a value
  return GetEPType(ENDP0) ? 1 : 0;
#endif
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


/////////////////////////////////////////////////////////////////////////////
//! Hooks of STM32 USB library
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////

// reset routine
static void MIOS32_USB_CB_Reset(void)
{
  // Set MIOS32 Device as not configured
  pInformation->Current_Configuration = 0;

  // Current Feature initialization
  pInformation->Current_Feature = MIOS32_USB_ConfigDescriptor[7];

  // Set MIOS32 Device with the default Interface
  pInformation->Current_Interface = 0;

#ifdef STM32F10X_CL   
  // EP0 is already configured in DFU_Init() by USB_SIL_Init() function
#else 
  SetBTABLE(MIOS32_USB_BTABLE_ADDRESS);

  // Initialize Endpoint 0
  SetEPType(ENDP0, EP_CONTROL);
  SetEPTxStatus(ENDP0, EP_TX_STALL);
  SetEPRxAddr(ENDP0, MIOS32_USB_ENDP0_RXADDR);
  SetEPTxAddr(ENDP0, MIOS32_USB_ENDP0_TXADDR);
  Clear_Status_Out(ENDP0);
  SetEPRxCount(ENDP0, pProperty->MaxPacketSize);
  SetEPRxValid(ENDP0);
#endif
  

#ifndef MIOS32_DONT_USE_USB_MIDI
#if MIOS32_USB_MIDI_DATA_OUT_EP != 0x02
# error "Please adapt the code for different endpoints"
#endif
#if MIOS32_USB_MIDI_DATA_IN_EP != 0x81
# error "Please adapt the code for different endpoints"
#endif

# ifdef STM32F10X_CL   
  // Init EP1 IN
  OTG_DEV_EP_Init(EP1_IN, OTG_DEV_EP_TYPE_BULK, MIOS32_USB_MIDI_DATA_IN_SIZE);
  
  // Init EP2 OUT
  OTG_DEV_EP_Init(EP2_OUT, OTG_DEV_EP_TYPE_BULK, MIOS32_USB_MIDI_DATA_OUT_SIZE);
# else
  // Initialize Endpoint 1/2
  SetEPType(ENDP1, EP_BULK);
  SetEPType(ENDP2, EP_BULK);

  SetEPTxAddr(ENDP1, MIOS32_USB_ENDP1_TXADDR);
  SetEPTxCount(ENDP1, MIOS32_USB_MIDI_DATA_OUT_SIZE);
  SetEPTxStatus(ENDP1, EP_TX_NAK);

  SetEPRxAddr(ENDP2, MIOS32_USB_ENDP2_RXADDR);
  SetEPRxCount(ENDP2, MIOS32_USB_MIDI_DATA_IN_SIZE);
  SetEPRxValid(ENDP2);
# endif
#endif


#ifdef MIOS32_USE_USB_COM
# ifdef STM32F10X_CL   
  // TODO...
# else
  // Initialize Endpoint 5
  SetEPType(ENDP5, EP_INTERRUPT);
  SetEPTxAddr(ENDP5, MIOS32_USB_ENDP5_TXADDR);
  SetEPRxStatus(ENDP5, EP_RX_DIS);
  SetEPTxStatus(ENDP5, EP_TX_NAK);

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
# endif
#endif

  // Set this device to response on default address
#ifndef STM32F10X_CL   
  SetDeviceAddress(0);
#endif

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
  if (pInformation->Current_Configuration != 0) {
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
#if !defined(MIOS32_DONT_USE_USB_MIDI) && MIOS32_USB_MIDI_NUM_PORTS > 1
  if( MIOS32_USB_ForceSingleUSB() ) {
    ONE_DESCRIPTOR desc = {(u8 *)MIOS32_USB_ConfigDescriptor_SingleUSB, MIOS32_USB_SIZ_CONFIG_DESC_SINGLE_USB};
    return Standard_GetDescriptorData(Length, &desc);
  }
#endif

  ONE_DESCRIPTOR desc = {(u8 *)MIOS32_USB_ConfigDescriptor, MIOS32_USB_SIZ_CONFIG_DESC};
  return Standard_GetDescriptorData(Length, &desc);
}

// gets the string descriptors according to the needed index
static u8 *MIOS32_USB_CB_GetStringDescriptor(u16 Length)
{
  const u8 vendor_str[] = MIOS32_USB_VENDOR_STR;
  const u8 product_str[] = MIOS32_USB_PRODUCT_STR;

  u8 buffer[200];
  u16 len = 0;
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

    case 2: { // Product
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

      for(i=0, len=2; product_str_ptr[i] != '\0' && len<200; ++i) {
	buffer[len++] = product_str_ptr[i];
	buffer[len++] = 0;
      }
    } break;

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


#ifdef STM32F10X_CL
/*******************************************************************************
* Function Name  : USB_OTG_BSP_uDelay.
* Description    : provide delay (usec).
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void USB_OTG_BSP_uDelay (const uint32_t usec)
{
#if defined(MIOS32_DONT_USE_DELAY)
# error "MIOS32_DELAY must be enabled for this derivative"
#else
  MIOS32_DELAY_Wait_uS(usec);
#endif  
}
#endif


//! \}

#endif /* MIOS32_DONT_USE_USB */
