// $Id$
/*
 * Simple MIOS32_IIC_BS check
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
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
    // toggle Status LED
    MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

    if( midi_package.type == NoteOn && midi_package.velocity > 0 ) {
      switch( midi_package.note % 12 ) {

      ////////////////////////////////////////////////////////////////////////////////////
      case 0: { // C
	s32 status;
	if( (status=MIOS32_IIC_BS_ScanBankSticks()) < 0 )
	  MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_BS_ScanBankSticks() failed with %d\n", status);

	int i;
	for(i=0; i<MIOS32_IIC_BS_NUM; ++i)
	  MIOS32_MIDI_SendDebugMessage("BS%d %savailable\n", i, MIOS32_IIC_BS_CheckAvailable(i) ? "" : "NOT ");
      } break;

      ////////////////////////////////////////////////////////////////////////////////////
      case 2: { // D
	s32 status;
	int i;
	u8 bs = 0;
// buffer size for 24LC64 limited to 32 bytes!
#define BUFFER_SIZE 32
	u8 wr_buffer[BUFFER_SIZE];
	u8 rd_buffer[BUFFER_SIZE];
	u32 address = 0x0000;

	if( (status=MIOS32_IIC_BS_Read(bs, address, rd_buffer, BUFFER_SIZE)) < 0 ) {
	  MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_BS_Read() failed with %d\n", status);
	  return;
	}

	MIOS32_MIDI_SendDebugMessage("Successfully read the first %d bytes:\n", BUFFER_SIZE);
	MIOS32_MIDI_SendDebugHexDump(rd_buffer, BUFFER_SIZE);

	for(i=0; i<BUFFER_SIZE; ++i) {
	  wr_buffer[i] = 0x80 + i;
	  rd_buffer[i] = 0xee;
	}

	if( (status=MIOS32_IIC_BS_Write(bs, address, wr_buffer, BUFFER_SIZE)) < 0 ) {
	  MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_BS_Write() failed with %d\n", status);
	  return;
	}

	while( (status=MIOS32_IIC_BS_CheckWriteFinished(bs)) == 1 );

	if( status != 0 ) {
	  MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_BS_CheckWriteFinished() failed with %d\n", status);
	  return;
	}

	MIOS32_MIDI_SendDebugMessage("Successfully written.\n");

	if( (status=MIOS32_IIC_BS_Read(bs, address, rd_buffer, BUFFER_SIZE)) < 0 ) {
	  MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_BS_Read() failed with %d\n", status);
	  return;
	}

	u32 mismatch_found = 0;			     
	for(i=0; i<BUFFER_SIZE; ++i)
	  if( wr_buffer[i] != rd_buffer[i] )
	    ++mismatch_found;

	if( !mismatch_found ) {
	  MIOS32_MIDI_SendDebugMessage("No mismatches found after readback! :-)\n");
	} else {
	  MIOS32_MIDI_SendDebugMessage("%d mismatches found after readback! :-(\n", mismatch_found);
	  MIOS32_MIDI_SendDebugHexDump(rd_buffer, BUFFER_SIZE);
	}

      } break;


      ////////////////////////////////////////////////////////////////////////////////////
      case 4: { // E
	s32 status;
	int i;
	u8 bs = 0;
// buffer size for 24LC64 limited to 32 bytes!
#define BUFFER_SIZE 32
	u8 wr_buffer[BUFFER_SIZE];
	u8 rd_buffer[BUFFER_SIZE];
	u32 address = 0x0000;

	if( (status=MIOS32_IIC_BS_Read(bs, address, rd_buffer, BUFFER_SIZE)) < 0 ) {
	  MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_BS_Read() failed with %d\n", status);
	  return;
	}

	MIOS32_MIDI_SendDebugMessage("Successfully read the first %d bytes:\n", BUFFER_SIZE);
	MIOS32_MIDI_SendDebugHexDump(rd_buffer, BUFFER_SIZE);

	for(i=0; i<BUFFER_SIZE; ++i) {
	  wr_buffer[i] = 0x40 + i;
	  rd_buffer[i] = 0xee;
	}

	for(i=0; i<BUFFER_SIZE; ++i) {
	  if( (status=MIOS32_IIC_BS_Write(bs, address+i, (u8 *)&wr_buffer[i], 1)) < 0 ) {
	    MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_BS_Write() failed with %d\n", status);
	    return;
	  }

	  while( (status=MIOS32_IIC_BS_CheckWriteFinished(bs)) == 1 );

	  if( status != 0 ) {
	    MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_BS_CheckWriteFinished() failed with %d\n", status);
	    return;
	  }
	}

	MIOS32_MIDI_SendDebugMessage("Successfully written bytewise.\n");

	for(i=0; i<BUFFER_SIZE; ++i) {
	  if( (status=MIOS32_IIC_BS_Read(bs, address+i, (u8 *)&rd_buffer[i], 1)) < 0 ) {
	    MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_BS_Read() failed with %d\n", status);
	    return;
	  }
	}

	u32 mismatch_found = 0;			     
	for(i=0; i<BUFFER_SIZE; ++i)
	  if( wr_buffer[i] != rd_buffer[i] )
	    ++mismatch_found;

	if( !mismatch_found ) {
	  MIOS32_MIDI_SendDebugMessage("No mismatches found after readback bytewise! :-)\n");
	} else {
	  MIOS32_MIDI_SendDebugMessage("%d mismatches found after readback bytewise! :-(\n", mismatch_found);
	  MIOS32_MIDI_SendDebugHexDump(rd_buffer, BUFFER_SIZE);
	}

      } break;


      ////////////////////////////////////////////////////////////////////////////////////
      case 5: { // F
	s32 status;
	int i;
	u8 bs = 0;
// buffer size for 24LC64 limited to 32 bytes!
#define BUFFER_SIZE 32
	u16 wr_buffer_16[BUFFER_SIZE/2];
	u16 rd_buffer_16[BUFFER_SIZE/2];
	u32 address = 0x0000;
	u32 size = MIOS32_IIC_BS_CheckAvailable(bs);

	for(address=0; address<size; address+=BUFFER_SIZE) {
	  MIOS32_MIDI_SendDebugMessage("Programming block 0x%04x\n", address);
	  for(i=0; i<BUFFER_SIZE/2; ++i)
	    wr_buffer_16[i] = address + 2*i;

	  if( (status=MIOS32_IIC_BS_Write(bs, address, (u8 *)wr_buffer_16, BUFFER_SIZE)) < 0 ) {
	    MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_BS_Write() failed at address 0x%04x with %d\n", address, status);
	    return;
	  }

	  while( (status=MIOS32_IIC_BS_CheckWriteFinished(bs)) == 1 );

	  if( status != 0 ) {
	    MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_BS_CheckWriteFinished() failed at address 0x%04x with %d\n", address, status);
	    return;
	  }
	}

	MIOS32_MIDI_SendDebugMessage("Successfully written %d bytes.\n", size);

	u32 any_mismatches_found = 0;
	for(address=0; address<size; address+=BUFFER_SIZE) {
	  MIOS32_MIDI_SendDebugMessage("Checking block 0x%04x\n", address);
	  for(i=0; i<BUFFER_SIZE/2; ++i)
	    rd_buffer_16[i] = 0xeeee;

	  if( (status=MIOS32_IIC_BS_Read(bs, address, (u8 *)rd_buffer_16, BUFFER_SIZE)) < 0 ) {
	    MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_BS_Read() failed at address 0x%04x with %d\n", address, status);
	    return;
	  }

	  u32 mismatch_found = 0;			     
	  for(i=0; i<BUFFER_SIZE/2; ++i)
	    if( rd_buffer_16[i] != (address+2*i) )
	      ++mismatch_found;

	  if( !mismatch_found ) {
	  } else {
	    any_mismatches_found = 1;
	    MIOS32_MIDI_SendDebugMessage("%d mismatches found after readback! :-(\n", mismatch_found);
	    MIOS32_MIDI_SendDebugHexDump((u8 *)rd_buffer_16, BUFFER_SIZE);
	  }
	}

	if( any_mismatches_found ) {
	  MIOS32_MIDI_SendDebugMessage("Mismatches found! Check Log!\n");
	} else {
	  MIOS32_MIDI_SendDebugMessage("No mismatches found! :-)\n");
	}

      } break;

#if 0
      ////////////////////////////////////////////////////////////////////////////////////
      case 11: { // B
	MIOS32_SYS_Reset();
      } break;
#endif

      default:
	MIOS32_MIDI_SendDebugMessage("not assigned\n");
      }
    }
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
