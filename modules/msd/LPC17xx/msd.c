// $Id$
//!
//! MSD Driver for MIOS32 running on a LPC17xx derivative
//! Based on Code from Bertrik Sikken (bertrik@sikken.nl)
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */
/*
	LPCUSB, an USB device driver for LPC microcontrollers	
	Copyright (C) 2006 Bertrik Sikken (bertrik@sikken.nl)

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	3. The name of the author may not be used to endorse or promote products
	   derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
	IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
	IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
	NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
	THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <msd.h>
#include <string.h>
#include <usbapi.h>

#include "msc_bot.h"
#include "msc_scsi.h"
#include "blockdev.h"


#define MAX_PACKET_SIZE	64

#define LE_WORD(x)		((x)&0xFF),((x)>>8)


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static BOOL HandleClassRequest(TSetupPacket *pSetup, int *piLen, u8 **ppbData);
static BOOL HandleCustomRequest(TSetupPacket *pSetup, int *piLen, U8 **ppbData);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 abClassReqData[4];
static u8 lun_available;


static const u8 abDescriptors[] = {

// device descriptor	
	0x12,
	DESC_DEVICE,			
	LE_WORD(0x0200),		// bcdUSB
	0x00,					// bDeviceClass
	0x00,					// bDeviceSubClass
	0x00,					// bDeviceProtocol
	MAX_PACKET_SIZE0,		// bMaxPacketSize
	LE_WORD(0x0483),		// idVendor
	LE_WORD(0x5720),		// idProduct
	LE_WORD(0x0200),		// bcdDevice
	0x01,					// iManufacturer
	0x02,					// iProduct
	0x03,					// iSerialNumber
	0x01,					// bNumConfigurations

// configuration descriptor
	0x09,
	DESC_CONFIGURATION,
	LE_WORD(32),			// wTotalLength
	0x01,					// bNumInterfaces
	0x01,					// bConfigurationValue
	0x00,					// iConfiguration
	0xC0,					// bmAttributes
	0x32,					// bMaxPower

// interface
	0x09,
	DESC_INTERFACE,
	0x00,					// bInterfaceNumber
	0x00,					// bAlternateSetting
	0x02,					// bNumEndPoints
	0x08,					// bInterfaceClass = mass storage
	0x06,					// bInterfaceSubClass = transparent SCSI
	0x50,					// bInterfaceProtocol = BOT
	0x00,					// iInterface
// EP
	0x07,
	DESC_ENDPOINT,
	MSC_BULK_IN_EP,			// bEndpointAddress
	0x02,					// bmAttributes = bulk
	LE_WORD(MAX_PACKET_SIZE),// wMaxPacketSize
	0x00,					// bInterval
// EP
	0x07,
	DESC_ENDPOINT,
	MSC_BULK_OUT_EP,		// bEndpointAddress
	0x02,					// bmAttributes = bulk
	LE_WORD(MAX_PACKET_SIZE),// wMaxPacketSize
	0x00,					// bInterval

// terminating zero
	0
};


/////////////////////////////////////////////////////////////////////////////
//! Initializes the USB Device Driver for a Mass Storage Device
//!
//! Should be called during runtime once a SD Card has been connected.<BR>
//! It is possible to switch back to the original device driver provided
//! by MIOS32 (e.g. MIOS32_USB_MIDI) by calling MIOS32_USB_Init(1)
//!
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MSD_Init(u32 mode)
{
  lun_available = 0;
  msd_memory_rd_led_ctr = 0;
  msd_memory_wr_led_ctr = 0;

  // initialise the SD card
  BlockDevInit();
  SCSIReset();

  // initialise stack
  USBInit();

  MIOS32_IRQ_Disable();
	
  // disconnect from bus
  USBHwConnect(FALSE);
  MIOS32_DELAY_Wait_uS(10000);

  // enable bulk-in interrupts on NAKs
  // these are required to get the BOT protocol going again after a STALL
  USBHwNakIntEnable(INACK_BI);

  // register descriptors
  USBRegisterDescriptors(abDescriptors);

  // register class request handler
  USBRegisterRequestHandler(REQTYPE_TYPE_CLASS, HandleClassRequest, abClassReqData);

  // register custom request handler
  USBRegisterCustomReqHandler(HandleCustomRequest);

  // register endpoint handlers
  USBHwRegisterEPIntHandler(MSC_BULK_IN_EP, MSCBotBulkIn);
  USBHwRegisterEPIntHandler(MSC_BULK_OUT_EP, MSCBotBulkOut);

  // enable_USB_interrupt
  MIOS32_IRQ_Install(USB_IRQn, MIOS32_IRQ_USB_PRIORITY);

  // connect to bus
  USBHwConnect(TRUE);

  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Should be called periodically each millisecond so long this driver is
//! active (and only then!) to handle USBtransfers.
//!
//! Take care that no other task accesses SD Card while this function is
//! processed!
//!
//! Ensure that this function isn't called when a MIOS32 USB driver like
//! MIOS32_USB_MIDI is running!
//!
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MSD_Periodic_mS(void)
{
  // handle LED counters
  if( msd_memory_rd_led_ctr < 65535 )
    ++msd_memory_rd_led_ctr;

  if( msd_memory_wr_led_ctr < 65535 )
    ++msd_memory_wr_led_ctr;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the connection status of the USB MIDI interface
//! \return 1: interface available
//! \return 0: interface not available
/////////////////////////////////////////////////////////////////////////////
s32 MSD_CheckAvailable(void)
{
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
//! The logical unit is available whenever MSD_Init() is called, or the USB
//! cable has been reconnected.
//!
//! It will be disabled when the host unmounts the file system (like if the
//! SD Card would be removed.
//!
//! When this happens, the application can either call MIOS32_USB_Init(1)
//! again, e.g. to switch to USB MIDI, or it can make the LUN available
//! again by calling MSD_LUN_AvailableSet(0, 1)
//! \param[in] lun Logical Unit number (0)
//! \param[in] available 0 or 1
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MSD_LUN_AvailableSet(u8 lun, u8 available)
{
  if( lun >= 1 )
    return -1;

  if( available )
    lun_available |= (1 << lun);
  else
    lun_available &= ~(1 << lun);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! \return 1 if device is mounted by host
//! \return 0 if device is not mounted by host
/////////////////////////////////////////////////////////////////////////////
s32 MSD_LUN_AvailableGet(u8 lun)
{
  if( lun >= 1 )
    return 0;

  return (lun_available & (1 << lun)) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns status of "Read LED"
//! \param[in] lag_ms The "lag" of the LED (e.g. 250 for 250 mS)
//! \return 1: a read operation has happened in the last x mS (x == time defined by "lag")
//! \return 0: no read operation has happened in the last x mS (x == time defined by "lag")
/////////////////////////////////////////////////////////////////////////////
s32 MSD_RdLEDGet(u16 lag_ms)
{
  return (msd_memory_rd_led_ctr < lag_ms) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns status of "Write LED"
//! \param[in] lag_ms The "lag" of the LED (e.g. 250 for 250 mS)
//! \return 1: a write operation has happened in the last x mS (x == time defined by "lag")
//! \return 0: no write operation has happened in the last x mS (x == time defined by "lag")
/////////////////////////////////////////////////////////////////////////////
s32 MSD_WrLEDGet(u16 lag_ms)
{
  return (msd_memory_wr_led_ctr < lag_ms) ? 1 : 0;
}



/*************************************************************************
	HandleClassRequest
	==================
		Handle mass storage class request
	
**************************************************************************/
static BOOL HandleClassRequest(TSetupPacket *pSetup, int *piLen, u8 **ppbData)
{
	if (pSetup->wIndex != 0) {
	  //DBG("Invalid idx %X\n", pSetup->wIndex);
		return FALSE;
	}
	if (pSetup->wValue != 0) {
	  //DBG("Invalid val %X\n", pSetup->wValue);
		return FALSE;
	}

	switch (pSetup->bRequest) {

	// get max LUN
	case 0xFE:
		*ppbData[0] = 0;		// No LUNs
		*piLen = 1;
		break;

	// MSC reset
	case 0xFF:
		if (pSetup->wLength > 0) {
			return FALSE;
		}
		MSCBotReset();
		break;
		
	default:
	  // DBG("Unhandled class\n");
		return FALSE;
	}
	return TRUE;
}


// only used to return dynamically generated strings
// TK: maybe it's possible to use a free buffer from somewhere else?
#define BUFFER_SIZE 75
static BOOL HandleCustomRequest(TSetupPacket *pSetup, int *piLen, U8 **ppbData)
{
  const u8 vendor_str[] = MIOS32_USB_VENDOR_STR;
  const u8 product_str[] = "MIOS32 Mass Storage Device";
  static u8 buffer[BUFFER_SIZE]; // TODO: maybe buffer provided by USB Driver?

  if( pSetup->bRequest != REQ_GET_DESCRIPTOR )
    return FALSE;

  u8 bType = GET_DESC_TYPE(pSetup->wValue);
  u8 bIndex = GET_DESC_INDEX(pSetup->wValue);

  if( bType != DESC_STRING )
    return FALSE;

  u16 len;
  int i;

  switch( bIndex ) {
    case 0: // Language
      // buffer[0] and [1] initialized below
      buffer[2] = 0x09;        // CharSet
      buffer[3] = 0x04;        // U.S.
      len = 4;
      break;

    case 1: // Vendor
      // buffer[0] and [1] initialized below
      for(i=0, len=2; vendor_str[i] != '\0' && len<BUFFER_SIZE; ++i) {
	buffer[len++] = vendor_str[i];
	buffer[len++] = 0;
      }
      break;

    case 2: // Product
      // buffer[0] and [1] initialized below
      for(i=0, len=2; product_str[i] != '\0' && len<BUFFER_SIZE; ++i) {
	buffer[len++] = product_str[i];
	buffer[len++] = 0;
      }
      break;

    case 3: { // Serial Number
        u8 serial_number_str[40];
	if( MIOS32_SYS_SerialNumberGet((char *)serial_number_str) >= 0 ) {
	  for(i=0, len=2; serial_number_str[i] != '\0' && len<BUFFER_SIZE; ++i) {
	    buffer[len++] = serial_number_str[i];
	    buffer[len++] = 0;
	  }
	} else
	  return FALSE;
      }
      break;

    default: // string ID not supported
      return FALSE;
  }

  buffer[0] = len; // Descriptor Length
  buffer[1] = 3; //DSCR_STRING; // Descriptor Type
  *ppbData = buffer;
  return TRUE;
}
