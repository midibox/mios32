// $Id$
/*
 * SD Card Info
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
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


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  MIOS32_SDCARD_Init(0);

  // print welcome message on MIOS terminal
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("\n");
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  static u8 sdcard_available = 0;

  // init LCD
  MIOS32_LCD_Clear();
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("see README.txt   ");
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("for details     ");

  // endless loop
  while( 1 ) {
    int status = 0;

    // check if SD card is available
    // High-speed access if SD card was previously available
    u8 prev_sdcard_available = sdcard_available;
    sdcard_available = MIOS32_SDCARD_CheckAvailable(prev_sdcard_available);

    if( sdcard_available && !prev_sdcard_available ) {
      MIOS32_BOARD_LED_Set(0x1, 0x1); // turn on LED
      MIOS32_MIDI_SendDebugMessage("SD Card has been connected!\n");

      MIOS32_LCD_Clear();
      MIOS32_LCD_CursorSet(0, 0);
      MIOS32_LCD_PrintString("SD Card         ");
      MIOS32_LCD_CursorSet(0, 1);
      MIOS32_LCD_PrintString("connected       ");

      // read CID data
      mios32_sdcard_cid_t cid;
      if( (status=MIOS32_SDCARD_CIDRead(&cid)) < 0 ) {
	MIOS32_MIDI_SendDebugMessage("Reading CID failed with status %d!\n", status);
      } else {
       	MIOS32_MIDI_SendDebugMessage("--------------------\n");
	MIOS32_MIDI_SendDebugMessage("CID:\n");
	MIOS32_MIDI_SendDebugMessage("- ManufacturerID:\n", cid.ManufacturerID);
	MIOS32_MIDI_SendDebugMessage("- OEM AppliID:\n", cid.OEM_AppliID);
	MIOS32_MIDI_SendDebugMessage("- ProdName: %s\n", cid.ProdName);
	MIOS32_MIDI_SendDebugMessage("- ProdRev: %u\n", cid.ProdRev);
	MIOS32_MIDI_SendDebugMessage("- ProdSN: 0x%08x\n", cid.ProdSN);
	MIOS32_MIDI_SendDebugMessage("- Reserved1: %u\n", cid.Reserved1);
	MIOS32_MIDI_SendDebugMessage("- ManufactDate: %u\n", cid.ManufactDate);
	MIOS32_MIDI_SendDebugMessage("- msd_CRC: 0x%02x\n", cid.msd_CRC);
	MIOS32_MIDI_SendDebugMessage("- Reserved2: %u\n", cid.Reserved2);
	MIOS32_MIDI_SendDebugMessage("--------------------\n");
      }


      // read CSD data
      mios32_sdcard_csd_t csd;
      if( (status=MIOS32_SDCARD_CSDRead(&csd)) < 0 ) {
	MIOS32_MIDI_SendDebugMessage("Reading CSD failed with status %d!\n", status);
      } else {
	MIOS32_MIDI_SendDebugMessage("--------------------\n");
	MIOS32_MIDI_SendDebugMessage("CSDStruct: %u\n", csd.CSDStruct);
	MIOS32_MIDI_SendDebugMessage("SysSpecVersion: %u\n", csd.SysSpecVersion);
	MIOS32_MIDI_SendDebugMessage("Reserved1: %u\n", csd.Reserved1);
	MIOS32_MIDI_SendDebugMessage("TAAC: %u\n", csd.TAAC);
	MIOS32_MIDI_SendDebugMessage("NSAC: %u\n", csd.NSAC);
	MIOS32_MIDI_SendDebugMessage("MaxBusClkFrec: %u\n", csd.MaxBusClkFrec);
	MIOS32_MIDI_SendDebugMessage("CardComdClasses: %u\n", csd.CardComdClasses);
	MIOS32_MIDI_SendDebugMessage("RdBlockLen: %u\n", csd.RdBlockLen);
	MIOS32_MIDI_SendDebugMessage("PartBlockRead: %u\n", csd.PartBlockRead);
	MIOS32_MIDI_SendDebugMessage("WrBlockMisalign: %u\n", csd.WrBlockMisalign);
	MIOS32_MIDI_SendDebugMessage("RdBlockMisalign: %u\n", csd.RdBlockMisalign);
	MIOS32_MIDI_SendDebugMessage("DSRImpl: %u\n", csd.DSRImpl);
	MIOS32_MIDI_SendDebugMessage("Reserved2: %u\n", csd.Reserved2);
	MIOS32_MIDI_SendDebugMessage("DeviceSize: %u\n", csd.DeviceSize);
	MIOS32_MIDI_SendDebugMessage("MaxRdCurrentVDDMin: %u\n", csd.MaxRdCurrentVDDMin);
	MIOS32_MIDI_SendDebugMessage("MaxRdCurrentVDDMax: %u\n", csd.MaxRdCurrentVDDMax);
	MIOS32_MIDI_SendDebugMessage("MaxWrCurrentVDDMin: %u\n", csd.MaxWrCurrentVDDMin);
	MIOS32_MIDI_SendDebugMessage("MaxWrCurrentVDDMax: %u\n", csd.MaxWrCurrentVDDMax);
	MIOS32_MIDI_SendDebugMessage("DeviceSizeMul: %u\n", csd.DeviceSizeMul);
	MIOS32_MIDI_SendDebugMessage("EraseGrSize: %u\n", csd.EraseGrSize);
	MIOS32_MIDI_SendDebugMessage("EraseGrMul: %u\n", csd.EraseGrMul);
	MIOS32_MIDI_SendDebugMessage("WrProtectGrSize: %u\n", csd.WrProtectGrSize);
	MIOS32_MIDI_SendDebugMessage("WrProtectGrEnable: %u\n", csd.WrProtectGrEnable);
	MIOS32_MIDI_SendDebugMessage("ManDeflECC: %u\n", csd.ManDeflECC);
	MIOS32_MIDI_SendDebugMessage("WrSpeedFact: %u\n", csd.WrSpeedFact);
	MIOS32_MIDI_SendDebugMessage("MaxWrBlockLen: %u\n", csd.MaxWrBlockLen);
	MIOS32_MIDI_SendDebugMessage("WriteBlockPaPartial: %u\n", csd.WriteBlockPaPartial);
	MIOS32_MIDI_SendDebugMessage("Reserved3: %u\n", csd.Reserved3);
	MIOS32_MIDI_SendDebugMessage("ContentProtectAppli: %u\n", csd.ContentProtectAppli);
	MIOS32_MIDI_SendDebugMessage("FileFormatGrouop: %u\n", csd.FileFormatGrouop);
	MIOS32_MIDI_SendDebugMessage("CopyFlag: %u\n", csd.CopyFlag);
	MIOS32_MIDI_SendDebugMessage("PermWrProtect: %u\n", csd.PermWrProtect);
	MIOS32_MIDI_SendDebugMessage("TempWrProtect: %u\n", csd.TempWrProtect);
	MIOS32_MIDI_SendDebugMessage("FileFormat: %u\n", csd.FileFormat);
	MIOS32_MIDI_SendDebugMessage("ECC: %u\n", csd.ECC);
	MIOS32_MIDI_SendDebugMessage("msd_CRC: 0x%02x\n", csd.msd_CRC);
	MIOS32_MIDI_SendDebugMessage("Reserved4: %u\n", csd.Reserved4);
	MIOS32_MIDI_SendDebugMessage("--------------------\n");
      }



    } else if( !sdcard_available && prev_sdcard_available ) {
      MIOS32_BOARD_LED_Set(0x1, 0x0); // turn off LED
      MIOS32_MIDI_SendDebugMessage("SD Card disconnected!\n");

      MIOS32_LCD_Clear();
      MIOS32_LCD_CursorSet(0, 0);
      MIOS32_LCD_PrintString("SD Card         ");
      MIOS32_LCD_CursorSet(0, 1);
      MIOS32_LCD_PrintString("disconnected    ");
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedSysEx(mios32_midi_port_t port, u8 sysex_byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a byte has been received via COM interface
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedCOM(mios32_com_port_t port, u8 byte)
{
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
