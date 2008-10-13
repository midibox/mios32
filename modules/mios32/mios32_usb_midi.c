// $Id$
/*
 * USB MIDI driver for MIOS32
 *
 * Based on driver included in STM32 USB library
 * Some code copied and modified from Virtual_COM_Port demo
 *
 * ==========================================================================
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
#if !defined(MIOS32_DONT_USE_USB_MIDI)

#include <usb_lib.h>

#include <FreeRTOS.h>
#include <portmacro.h>


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// size of IN/OUT pipe
#define MIOS32_USB_MIDI_DATA_IN_SIZE           64
#define MIOS32_USB_MIDI_DATA_OUT_SIZE          64

// (have to be adapted depending on descriptor lengths :-( )
#define MIOS32_USB_MIDI_SIZ_DEVICE_DESC        18
#define MIOS32_USB_MIDI_SIZ_CLASS_DESC         (7+MIOS32_USB_MIDI_NUM_PORTS*(6+6+9+9)+9+(4+MIOS32_USB_MIDI_NUM_PORTS)+9+(4+MIOS32_USB_MIDI_NUM_PORTS))
#define MIOS32_USB_MIDI_SIZ_CONFIG_DESC        (9+9+MIOS32_USB_MIDI_USE_AC_INTERFACE*(9+9)+MIOS32_USB_MIDI_SIZ_CLASS_DESC)
#define MIOS32_USB_MIDI_SIZ_STRING_LANGID      4
#define MIOS32_USB_MIDI_SIZ_STRING_VENDOR      24
#define MIOS32_USB_MIDI_SIZ_STRING_PRODUCT     14

#define DSCR_DEVICE	1	// Descriptor type: Device
#define DSCR_CONFIG	2	// Descriptor type: Configuration
#define DSCR_STRING	3	// Descriptor type: String
#define DSCR_INTRFC	4	// Descriptor type: Interface
#define DSCR_ENDPNT	5	// Descriptor type: Endpoint

#define CS_INTERFACE	0x24	// Class-specific type: Interface
#define CS_ENDPOINT	0x25	// Class-specific type: Endpoint

// number of endpoints
#define EP_NUM              (4)

// buffer table base address
#define BTABLE_ADDRESS      (0x00)

// EP0 rx/tx buffer base address
#define ENDP0_RXADDR        (0x40)
#define ENDP0_TXADDR        (0x80)

// EP1 Rx/Tx buffer base address
#define ENDP1_TXADDR        (0xc0)
#define ENDP1_RXADDR        (0x100)

// ISTR events
// mask defining which events has to be handled by the device application software
#define IMR_MSK (CNTR_CTRM  | CNTR_SOFM  | CNTR_RESETM )


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
// USB Standard Device Descriptor
/////////////////////////////////////////////////////////////////////////////
const u8 MIOS32_USB_MIDI_DeviceDescriptor[MIOS32_USB_MIDI_SIZ_DEVICE_DESC] = {
  // MIDI Adapter Device Descriptor
  (u8)(MIOS32_USB_MIDI_SIZ_DEVICE_DESC&0xff), // Device Descriptor length
  DSCR_DEVICE,			// Decriptor type
  (u8)(0x0200 & 0xff),		// Specification Version (BCD, LSB)
  (u8)(0x0200 >> 8),		// Specification Version (BCD, MSB)
  0x00,				// Device class
  0x00,				// Device sub-class
  0x00,				// Device sub-sub-class
  0x40,				// Maximum packet size
  (u8)(0x16c0 & 0xff),		// Vendor ID 16C0 --- sponsored by voti.nl! see http://www.voti.nl/pids
  (u8)(0x16c0 >> 8),		// Vendor ID (MSB)
  (u8)(1023 & 0xff),		// Product ID (LSB)
  (u8)(1023 >> 8),		// Product ID (MSB)
  (u8)(0x0001 & 0xff),		// Product version ID (LSB)
  (u8)(0x0001 >> 8),  		// Product version ID (MSB)
  0x01,				// Manufacturer string index
  0x02,				// Product string index
  0x00,				// Serial number string index
  0x01				// Number of configurations
};

const u8 MIOS32_USB_MIDI_ConfigDescriptor[MIOS32_USB_MIDI_SIZ_CONFIG_DESC] = {
  // Configuration Descriptor
  9,				// Descriptor length
  DSCR_CONFIG,			// Descriptor type
  (u8)(MIOS32_USB_MIDI_SIZ_CONFIG_DESC&0xff), // Config + End Points length (LSB)
  (u8)(MIOS32_USB_MIDI_SIZ_CONFIG_DESC>>8),   // Config + End Points length (MSB)
#if MIOS32_USB_MIDI_USE_AC_INTERFACE
  0x02,				// Number of interfaces
#else
  0x01,				// Number of interfaces
#endif
  0x01,				// Interface number
  0x02,				// Configuration string
  0x80,				// Attributes (b7 - buspwr, b6 - selfpwr, b5 - rwu)
  0x10,				// Power requirement (div 2 ma)

#if MIOS32_USB_MIDI_USE_AC_INTERFACE
  // Standard AC Interface Descriptor
  9,				// Descriptor length
  DSCR_INTRFC,			// Descriptor type
  0x00,				// Zero-based index of this interface
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
#if MIOS32_USB_MIDI_USE_AC_INTERFACE
  0x01,				// Zero-based index of this interface
#else
  0x00,				// Zero-based index of this interface
#endif
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
};


/////////////////////////////////////////////////////////////////////////////
// USB String Descriptors
/////////////////////////////////////////////////////////////////////////////
const u8 MIOS32_USB_MIDI_StringLangID[MIOS32_USB_MIDI_SIZ_STRING_LANGID] = {
  4,                            // String descriptor length
  DSCR_STRING,			// Descriptor Type
  0x09, 0x04			// Charset
};

const u8 MIOS32_USB_MIDI_StringVendor[MIOS32_USB_MIDI_SIZ_STRING_VENDOR] = {
  11*2+2,                       // String descriptor length
  DSCR_STRING,			// Descriptor Type
  'm',00,
  'i',00,
  'd',00,
  'i',00,
  'b',00,
  'o',00,
  'x',00,
  '.',00,
  'o',00,
  'r',00,
  'g',00
};

const u8 MIOS32_USB_MIDI_StringProduct[MIOS32_USB_MIDI_SIZ_STRING_PRODUCT] = {
  6*2+2,                       // String descriptor length
  DSCR_STRING,			// Descriptor Type
  'M',00,
  'I',00,
  'O',00,
  'S',00,
  '3',00,
  '2',00
};


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

void MIOS32_USB_MIDI_TxBufferHandler(void);
void MIOS32_USB_MIDI_RxBufferHandler(void);

void MIOS32_USB_MIDI_EP1_IN_Callback(void);
void MIOS32_USB_MIDI_EP1_OUT_Callback(void);

void MIOS32_USB_MIDI_CB_Init(void);
void MIOS32_USB_MIDI_CB_Reset(void);
void MIOS32_USB_MIDI_CB_SetConfiguration(void);
void MIOS32_USB_MIDI_CB_SetDeviceAddress (void);
void MIOS32_USB_MIDI_CB_Status_In(void);
void MIOS32_USB_MIDI_CB_Status_Out(void);
RESULT MIOS32_USB_MIDI_CB_Data_Setup(u8 RequestNo);
RESULT MIOS32_USB_MIDI_CB_NoData_Setup(u8 RequestNo);
u8 *MIOS32_USB_MIDI_CB_GetDeviceDescriptor(u16 Length);
u8 *MIOS32_USB_MIDI_CB_GetConfigDescriptor(u16 Length);
u8 *MIOS32_USB_MIDI_CB_GetStringDescriptor(u16 Length);
RESULT MIOS32_USB_MIDI_CB_Get_Interface_Setting(u8 Interface, u8 AlternateSetting);


/////////////////////////////////////////////////////////////////////////////
// USB callback vectors
/////////////////////////////////////////////////////////////////////////////

void (*pEpInt_IN[7])(void) = {
  MIOS32_USB_MIDI_EP1_IN_Callback,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process
};

void (*pEpInt_OUT[7])(void) = {
  MIOS32_USB_MIDI_EP1_OUT_Callback,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process
};

DEVICE Device_Table = {
  EP_NUM,
  1
};

DEVICE_PROP Device_Property = {
  MIOS32_USB_MIDI_CB_Init,
  MIOS32_USB_MIDI_CB_Reset,
  MIOS32_USB_MIDI_CB_Status_In,
  MIOS32_USB_MIDI_CB_Status_Out,
  MIOS32_USB_MIDI_CB_Data_Setup,
  MIOS32_USB_MIDI_CB_NoData_Setup,
  MIOS32_USB_MIDI_CB_Get_Interface_Setting,
  MIOS32_USB_MIDI_CB_GetDeviceDescriptor,
  MIOS32_USB_MIDI_CB_GetConfigDescriptor,
  MIOS32_USB_MIDI_CB_GetStringDescriptor,
  0,
  0x40 /*MAX PACKET SIZE*/
};

USER_STANDARD_REQUESTS User_Standard_Requests = {
  NOP_Process, // MIOS32_USB_MIDI_CB_GetConfiguration,
  MIOS32_USB_MIDI_CB_SetConfiguration,
  NOP_Process, // MIOS32_USB_MIDI_CB_GetInterface,
  NOP_Process, // MIOS32_USB_MIDI_CB_SetInterface,
  NOP_Process, // MIOS32_USB_MIDI_CB_GetStatus,
  NOP_Process, // MIOS32_USB_MIDI_CB_ClearFeature,
  NOP_Process, // MIOS32_USB_MIDI_CB_SetEndPointFeature,
  NOP_Process, // MIOS32_USB_MIDI_CB_SetDeviceFeature,
  MIOS32_USB_MIDI_CB_SetDeviceAddress
};

ONE_DESCRIPTOR Device_Descriptor = {
  (u8*)MIOS32_USB_MIDI_DeviceDescriptor,
  MIOS32_USB_MIDI_SIZ_DEVICE_DESC
};

ONE_DESCRIPTOR Config_Descriptor = {
  (u8*)MIOS32_USB_MIDI_ConfigDescriptor,
  MIOS32_USB_MIDI_SIZ_CONFIG_DESC
};

ONE_DESCRIPTOR String_Descriptor[4] = {
  {(u8*)MIOS32_USB_MIDI_StringLangID, MIOS32_USB_MIDI_SIZ_STRING_LANGID},
  {(u8*)MIOS32_USB_MIDI_StringVendor, MIOS32_USB_MIDI_SIZ_STRING_VENDOR},
  {(u8*)MIOS32_USB_MIDI_StringProduct, MIOS32_USB_MIDI_SIZ_STRING_PRODUCT},
  0
};


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// global interrupt status (also used by USB driver, therefore extern)
volatile u16 wIstr;

// USB device status
static vu32 bDeviceState = UNCONNECTED;

// Rx buffer
static u32 rx_buffer[MIOS32_USB_MIDI_RX_BUFFER_SIZE];
static volatile u16 rx_buffer_tail;
static volatile u16 rx_buffer_head;
static volatile u16 rx_buffer_size;
static volatile u8 rx_buffer_new_data;

// Tx buffer
static u32 tx_buffer[MIOS32_USB_MIDI_TX_BUFFER_SIZE];
static volatile u16 tx_buffer_tail;
static volatile u16 tx_buffer_head;
static volatile u16 tx_buffer_size;
static volatile u8 tx_buffer_busy;

// optional non-blocking mode
static u8 non_blocking_mode = 0;

// transfer possible?
static u8 transfer_possible = 0;


/////////////////////////////////////////////////////////////////////////////
// Initializes the USB interface
// IN: <mode>: 0: MIOS32_MIDI_Send* works in blocking mode - function will
//                (shortly) stall if the output buffer is full
//             1: MIOS32_MIDI_Send* works in non-blocking mode - function will
//                return -2 if buffer is full, the caller has to loop if this
//                value is returned until the transfer was successful
//                A common method is to release the RTOS task for 1 mS
//                so that other tasks can be executed until the sender can
//                continue
// OUT: returns < 0 if initialisation failed (e.g. clock not initialised!)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_Init(u32 mode)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  u8 i;

  switch( mode ) {
    case 0:
      non_blocking_mode = 0;
      break;
    case 1:
      non_blocking_mode = 1;
      break;
    default:
      return -1; // unsupported mode
  }

  // clear buffer counters and busy/wait signals
  rx_buffer_tail = rx_buffer_head = rx_buffer_size = 0;
  rx_buffer_new_data = 0; // no data received yet
  tx_buffer_tail = tx_buffer_head = tx_buffer_size = 0;
  tx_buffer_busy = 0; // buffer is busy so long no USB connection detected


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

  // transfer not possible yet - will change once USB goes into CONFIGURED state
  transfer_possible = 0;

  // remaining initialisation done in STM32 USB driver
  USB_Init();

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the connection status of the USB MIDI interface
// IN: -
// OUT: 1: interface available
//      0: interface not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_CheckAvailable(void)
{
  return transfer_possible ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function puts a new MIDI package into the Tx buffer
// IN: MIDI package in <package>
// OUT: 0: no error
//      -1: USB not connected
//      -2: if non-blocking mode activated: buffer is full
//          caller should retry until buffer is free again
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_MIDIPackageSend(mios32_midi_package_t package)
{
  // device available?
  if( !transfer_possible )
    return -1;

  // buffer full?
  if( tx_buffer_size >= (MIOS32_USB_MIDI_TX_BUFFER_SIZE-1) ) {
    // call USB handler, so that we are able to get the buffer free again on next execution
    // (this call simplifies polling loops!)
    MIOS32_USB_MIDI_Handler();

    // device still available?
    // (ensures that polling loop terminates if cable has been disconnected)
    if( !transfer_possible )
      return -1;

    // notify that buffer was full (request retry)
    if( non_blocking_mode )
      return -2;
  }

  // put package into buffer - this operation should be atomic!
  vPortEnterCritical(); // port specific FreeRTOS function to disable IRQs (nested)
  tx_buffer[tx_buffer_head++] = package.ALL;
  if( tx_buffer_head >= MIOS32_USB_MIDI_TX_BUFFER_SIZE )
    tx_buffer_head = 0;
  ++tx_buffer_size;
  vPortExitCritical(); // port specific FreeRTOS function to enable IRQs (nested)

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function checks for a new package
// IN: pointer to MIDI package in <package> (received package will be put into the given variable)
// OUT: returns -1 if no package in buffer
//      otherwise returns number of packages which are still in the buffer
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_MIDIPackageReceive(mios32_midi_package_t *package)
{
  u8 i;

  // package received?
  if( !rx_buffer_size )
    return -1;

  // get package - this operation should be atomic!
  vPortEnterCritical(); // port specific FreeRTOS function to disable IRQs (nested)
  package->ALL = rx_buffer[rx_buffer_tail];
  if( ++rx_buffer_tail >= MIOS32_USB_MIDI_RX_BUFFER_SIZE )
    rx_buffer_tail = 0;
  --rx_buffer_size;
  vPortExitCritical(); // port specific FreeRTOS function to enable IRQs (nested)

  return rx_buffer_size;
}


/////////////////////////////////////////////////////////////////////////////
// This handler should be called from a RTOS task to react on
// USB interrupt notifications
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_Handler(void)
{
  // MEMO: could also be called from USB_LP_CAN_RX0_IRQHandler
  // if IRQ vector configured for USB

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


  // do we need to react on the remaining events?
  // they are currently masked out via IMR_MSK


  // now check if something has to be sent or received
  MIOS32_USB_MIDI_RxBufferHandler();
  MIOS32_USB_MIDI_TxBufferHandler();

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This handler sends the new packages through the IN pipe if the buffer 
// is not empty
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_MIDI_TxBufferHandler(void)
{
  // send buffered packages if
  //   - last transfer finished
  //   - new packages are in the buffer
  //   - the device is configured

  if( !tx_buffer_busy && tx_buffer_size && transfer_possible ) {
    u32 *pma_addr = (u32 *)(PMAAddr + (ENDP1_TXADDR<<1));
    s16 count = (tx_buffer_size > (MIOS32_USB_MIDI_DATA_IN_SIZE/4)) ? (MIOS32_USB_MIDI_DATA_IN_SIZE/4) : tx_buffer_size;

    // notify that new package is sent
    tx_buffer_busy = 1;

    // send to IN pipe
    SetEPTxCount(ENDP1, 4*count);

    // atomic operation to avoid conflict with other interrupts
    vPortEnterCritical(); // port specific FreeRTOS function to disable IRQs (nested)
    tx_buffer_size -= count;

    // copy into PMA buffer (16bit word with, only 32bit addressable)
    do {
      *pma_addr++ = tx_buffer[tx_buffer_tail] & 0xffff;
      *pma_addr++ = (tx_buffer[tx_buffer_tail]>>16) & 0xffff;
      if( ++tx_buffer_tail >= MIOS32_USB_MIDI_TX_BUFFER_SIZE )
	tx_buffer_tail = 0;
    } while( --count );

    vPortExitCritical(); // port specific FreeRTOS function to enable IRQs (nested)

    // send buffer
    SetEPTxValid(ENDP1);
  }
}


/////////////////////////////////////////////////////////////////////////////
// This handler receives new packages if the Tx buffer is not full
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_MIDI_RxBufferHandler(void)
{
  s16 count;

  // check if we can receive new data and get packages to be received from OUT pipe
  if( rx_buffer_new_data && (count=GetEPRxCount(ENDP1)>>2) ) {

    // check if buffer is free
    if( count < (MIOS32_USB_MIDI_RX_BUFFER_SIZE-rx_buffer_size) ) {
      u32 *pma_addr = (u32 *)(PMAAddr + (ENDP1_RXADDR<<1));

      // copy received packages into receive buffer
      // this operation should be atomic
      vPortEnterCritical(); // port specific FreeRTOS function to disable IRQs (nested)
      do {
	u16 pl = *pma_addr++;
	u16 ph = *pma_addr++;
	rx_buffer[rx_buffer_head] = (ph << 16) | pl;
	if( ++rx_buffer_head >= MIOS32_USB_MIDI_RX_BUFFER_SIZE )
	  rx_buffer_head = 0;
	++rx_buffer_size;
      } while( --count >= 0 );

      // notify, that data has been put into buffer
      rx_buffer_new_data = 0;

      vPortExitCritical(); // port specific FreeRTOS function to enable IRQs (nested)

      // release OUT pipe
      SetEPRxValid(ENDP1);
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// Called by STM32 USB driver to check for IN streams
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_MIDI_EP1_IN_Callback(void)
{
  // package has been sent
  tx_buffer_busy = 0;

  // check for next package
  MIOS32_USB_MIDI_TxBufferHandler();
}

/////////////////////////////////////////////////////////////////////////////
// Called by STM32 USB driver to check for OUT streams
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_MIDI_EP1_OUT_Callback(void)
{
  // put package into buffer
  rx_buffer_new_data = 1;
  MIOS32_USB_MIDI_RxBufferHandler();
}


/////////////////////////////////////////////////////////////////////////////
// Hooks of STM32 USB library
/////////////////////////////////////////////////////////////////////////////

// init routine
void MIOS32_USB_MIDI_CB_Init(void)
{
  u32 delay;
  u16 wRegVal;

  pInformation->Current_Configuration = 0;

  // Connect the device

  // CNTR_PWDN = 0
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
  wInterrupt_Mask = CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM;
  _SetCNTR(wInterrupt_Mask);

  // USB interrupts initialization
  // clear pending interrupts
  _SetISTR(0);
  wInterrupt_Mask = IMR_MSK;

  // set interrupts mask
  _SetCNTR(wInterrupt_Mask);

  bDeviceState = UNCONNECTED;

  transfer_possible = 0;
}

// reset routine
void MIOS32_USB_MIDI_CB_Reset(void)
{
  // Set MIOS32 Device as not configured
  pInformation->Current_Configuration = 0;

  // Current Feature initialization
  pInformation->Current_Feature = MIOS32_USB_MIDI_ConfigDescriptor[7];

  // Set MIOS32 Device with the default Interface
  pInformation->Current_Interface = 0;
  SetBTABLE(BTABLE_ADDRESS);

  // Initialize Endpoint 0
  SetEPType(ENDP0, EP_CONTROL);
  SetEPTxStatus(ENDP0, EP_TX_STALL);
  SetEPRxAddr(ENDP0, ENDP0_RXADDR);
  SetEPTxAddr(ENDP0, ENDP0_TXADDR);
  Clear_Status_Out(ENDP0);
  SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
  SetEPRxValid(ENDP0);

  // Initialize Endpoint 1
  SetEPType(ENDP1, EP_BULK);

  SetEPTxAddr(ENDP1, ENDP1_TXADDR);
  SetEPTxCount(ENDP1, MIOS32_USB_MIDI_DATA_OUT_SIZE);
  SetEPTxStatus(ENDP1, EP_TX_NAK);

  SetEPRxAddr(ENDP1, ENDP1_RXADDR);
  SetEPRxCount(ENDP1, MIOS32_USB_MIDI_DATA_IN_SIZE);
  SetEPRxValid(ENDP1);

  // Set this device to response on default address
  SetDeviceAddress(0);

  // clear buffer counters and busy/wait signals again (e.g., so that no invalid data will be sent out)
  rx_buffer_tail = rx_buffer_head = rx_buffer_size = 0;
  rx_buffer_new_data = 0; // no data received yet
  tx_buffer_tail = tx_buffer_head = tx_buffer_size = 0;
  tx_buffer_busy = 0; // buffer not busy anymore

  bDeviceState = ATTACHED;
}

// update the device state to configured.
void MIOS32_USB_MIDI_CB_SetConfiguration(void)
{
  DEVICE_INFO *pInfo = &Device_Info;

  if (pInfo->Current_Configuration != 0) {
    bDeviceState = CONFIGURED;

    transfer_possible = 1;
  }
}

// update the device state to addressed
void MIOS32_USB_MIDI_CB_SetDeviceAddress (void)
{
  bDeviceState = ADDRESSED;
}

// status IN routine
void MIOS32_USB_MIDI_CB_Status_In(void)
{
}

// status OUT routine
void MIOS32_USB_MIDI_CB_Status_Out(void)
{
}

// handles the data class specific requests
RESULT MIOS32_USB_MIDI_CB_Data_Setup(u8 RequestNo)
{
  return USB_UNSUPPORT;
}

// handles the non data class specific requests.
RESULT MIOS32_USB_MIDI_CB_NoData_Setup(u8 RequestNo)
{
  return USB_UNSUPPORT;
}

// gets the device descriptor.
u8 *MIOS32_USB_MIDI_CB_GetDeviceDescriptor(u16 Length)
{
  return Standard_GetDescriptorData(Length, &Device_Descriptor);
}

// gets the configuration descriptor.
u8 *MIOS32_USB_MIDI_CB_GetConfigDescriptor(u16 Length)
{
  return Standard_GetDescriptorData(Length, &Config_Descriptor);
}

// gets the string descriptors according to the needed index
u8 *MIOS32_USB_MIDI_CB_GetStringDescriptor(u16 Length)
{
  u8 wValue0 = pInformation->USBwValue0;

  if (wValue0 > 4) {
    return NULL;
  } else {
    return Standard_GetDescriptorData(Length, &String_Descriptor[wValue0]);
  }
}

// test the interface and the alternate setting according to the supported one.
RESULT MIOS32_USB_MIDI_CB_Get_Interface_Setting(u8 Interface, u8 AlternateSetting)
{
  if (AlternateSetting > 0) {
    return USB_UNSUPPORT;
  } else if (Interface > 1) {
    return USB_UNSUPPORT;
  }

  return USB_SUCCESS;
}

#endif /* MIOS32_DONT_USE_USB_MIDI */
