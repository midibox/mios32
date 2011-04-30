// $Id$
/*
 * Simple MIOS32_IIC_MIDI check
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "app.h"


// from mios32_iic
extern volatile u32 MIOS32_IIC_unexpected_event;


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("Play MIDI notes via USB0 to start different test sequences\n");
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  if( port == USB0 ) {
    if( midi_package.type == NoteOn && midi_package.velocity > 0 ) {
      switch( midi_package.note % 12 ) {
      case 0: { // C
	s32 status;
	if( (status=MIOS32_IIC_MIDI_ScanInterfaces()) < 0 )
	  MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_MIDI_ScanInterfaces() failed with %d\n", status);

	int i;
	for(i=0; i<MIOS32_IIC_MIDI_NUM; ++i)
	  MIOS32_MIDI_SendDebugMessage("IIC%d %savailable\n", i, MIOS32_IIC_MIDI_CheckAvailable(i) ? "" : "NOT ");
      } break;

      case 2: { // D
	s32 status;
	u8 iic_port = 0;

	if( (status=MIOS32_IIC_TransferBegin(MIOS32_IIC_MIDI_PORT, IIC_Non_Blocking)) < 0 )
	  MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_TransferBegin failed with %d!\n", status);
	else {
	  if( (status=MIOS32_IIC_Transfer(MIOS32_IIC_MIDI_PORT, IIC_Write, MIOS32_IIC_MIDI_ADDR_BASE + 2*iic_port, NULL, 0)) < 0 )
	    MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_Transfer failed with %d!\n", status);
	  else {
	    if( (status=MIOS32_IIC_TransferWait(MIOS32_IIC_MIDI_PORT)) < 0 )
	      MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_TransferWait failed with %d (event: %02x)!\n", status, MIOS32_IIC_unexpected_event);
	    else {
	      MIOS32_IIC_TransferFinished(iic_port);
	      MIOS32_MIDI_SendDebugMessage("MBHP_IIC_MIDI %d replied! :-)\n", iic_port);
	    }
	  }
	}
      } break;

#if 0
      case 11: { // B
	MIOS32_SYS_Reset();
      } break;
#endif

      default:
	MIOS32_MIDI_SendDebugMessage("not assigned\n");
      }
    } else {
      if( midi_package.type == CC ) {
	// CCs: forward to all IICx
	int i;
	for(i=0; i<MIOS32_IIC_MIDI_NUM; ++i)
	  MIOS32_MIDI_SendPackage(IIC0 + i, midi_package);
      }
    }
  }

  if( (port & 0xf0) == IIC0 ) {
    // toggle Status LED
    MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

    MIOS32_MIDI_SendDebugMessage("IIC%d received %02x %02x %02x\n",
				 port & 0xf,
				 midi_package.evnt0, midi_package.evnt1, midi_package.evnt2);
  }

}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
