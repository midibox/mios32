// $Id$
/*
 * Bootloader Update
 * See README.txt for details
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


#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
# include "bsl_image_MBHP_CORE_STM32.inc"
#else
# error "BSL update not supported for the selected MIOS32_BOARD"
#endif


/////////////////////////////////////////////////////////////////////////////
// Local Macros
/////////////////////////////////////////////////////////////////////////////

// STM32: determine page size (mid density devices: 1k, high density devices: 2k)
// TODO: find a proper way, as there could be high density devices with less than 256k?)
#define FLASH_PAGE_SIZE   (MIOS32_SYS_FlashSizeGet() >= (256*1024) ? 0x800 : 0x400)

// STM32: flash memory range (16k BSL range excluded)
#define BSL_START_ADDR  0x08000000
#define BSL_END_ADDR    0x08003fff


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // turn off green LED as a clear indication that core shouldn't be powered-off/rebooted
  MIOS32_BOARD_LED_Set(0xffffffff, 0);
}


/////////////////////////////////////////////////////////////////////////////
// Help functions
/////////////////////////////////////////////////////////////////////////////
s32 Wait1Second(u8 dont_toggle_led)
{
  int i;

  for(i=0; i<50; ++i) {
    if( !dont_toggle_led ) {
      // toggle LED and wait for 20 mS (sign of life)
      MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());
    }
    MIOS32_DELAY_Wait_uS(20000);
  }

  return 0; // no error
}

s32 CompareBSL(void)
{
  // comparing BSL range with binary image
  u32 addr;
  u8 *bsl_addr_ptr = (u8 *)BSL_START_ADDR;
  int mismatches = 0;

  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("BootloaderUpdate"); // 16 chars
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("Checking        ");

  MIOS32_MIDI_SendDebugMessage("Checking Bootloader...\n");

  for(addr=0; addr<sizeof(bsl_image); ++addr) {
    // toggle LED (sign of life)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    if( (addr & 0xf) == 0 ) {
      MIOS32_LCD_CursorSet(9, 1);
      MIOS32_LCD_PrintFormattedString("0x%04x", addr);
    }

    if( bsl_addr_ptr[addr] != bsl_image[addr] ) {
      ++mismatches;
      if( mismatches < 10 ) {
	MIOS32_MIDI_SendDebugMessage("Mismatch at address 0x%04x\n", addr);
      } else if( mismatches == 10 ) {
	MIOS32_MIDI_SendDebugMessage("Too many mismatches, no additional messages will be print...\n");
      }
    }
  }

  return mismatches;
}

s32 UpdateBSL(void)
{
  // disable interrupts - this is really a critical section!
  MIOS32_IRQ_Disable();

  // Note: Device ID will be overwritten as well - this is intended, because it
  // can only be programmed once after flash has been erased!
  MIOS32_MIDI_DeviceIDSet(0x00);

  // FLASH_* routines are part of the STM32 code library
  FLASH_Unlock();

  FLASH_Status status;
  int i;
  u32 addr = BSL_START_ADDR;
  u32 len = BSL_END_ADDR - BSL_START_ADDR + 1;
  
  for(i=0; i<len; addr+=2, i+=2) {
    // toggle LED (sign of life)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    if( (addr % FLASH_PAGE_SIZE) == 0 ) {
      if( (status=FLASH_ErasePage(addr)) != FLASH_COMPLETE ) {
	FLASH_ClearFlag(FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); // clear error flags, otherwise next program attempts will fail
	MIOS32_IRQ_Enable();
	return -1; // erase failed
      }
    }

    if( (status=FLASH_ProgramHalfWord(addr, bsl_image[i+0] | ((u16)bsl_image[i+1] << 8))) != FLASH_COMPLETE ) {
      FLASH_ClearFlag(FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); // clear error flags, otherwise next program attempts will fail
      MIOS32_IRQ_Enable();
      return -2; // programming failed
    }
  }

  // ensure that flash write access is locked
  FLASH_Lock();

  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // init LCD
  MIOS32_LCD_Clear();

  // print welcome message on MIOS terminal
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("\n");

  // no mismatches? Fine! Wait for a new application upload (endless)
  if( CompareBSL() == 0 ) {
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintString("Bootloader is   "); // 16 chars
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintString("up-to-date! :-) ");

    // turn on green LED as a clear indication that core can be powered-off/rebooted
    MIOS32_BOARD_LED_Set(0xffffffff, 1);

    while( 1 ) {
      MIOS32_MIDI_SendDebugMessage("No mismatches found.\n");
      MIOS32_MIDI_SendDebugMessage("The bootloader is up-to-date!\n");
      MIOS32_MIDI_SendDebugMessage("You can upload another application now!\n");

      // wait for 1 second before printing the message again
      Wait1Second(1); // don't toggle LED!
    }
  }

  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("Bootloader Update"); // 16 chars

  MIOS32_MIDI_SendDebugMessage("Bootloader requires an update...\n");

  int i;
  for(i=10; i>=0; --i) {
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintFormattedString("in %2d seconds! ", i);
    MIOS32_MIDI_SendDebugMessage("Bootloader update in %d seconds!\n", i);
    Wait1Second(0);
  }

  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("Starting Update  ");
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("Don't power off!!");

  MIOS32_MIDI_SendDebugMessage("Starting Update - don't power off!!!\n");

  int num_retries = 5;
  int retry = num_retries;
  int mismatches = 0;
  do {
    // update the bootloader
    s32 status;
    if( (status=UpdateBSL()) < 0 ) {
      if( status == -1 ) {
	MIOS32_MIDI_SendDebugMessage("Oh-oh! Erase failed!\n");
      } else {
	MIOS32_MIDI_SendDebugMessage("Oh-oh! Programming failed!\n");
      }
    }

    // checking again
    mismatches = CompareBSL();

    if( !mismatches )
      retry = 0;
    else {
      --retry;
      if( retry )
	MIOS32_MIDI_SendDebugMessage("Update failed - retry #%d\n", num_retries-retry);
    }
  } while( mismatches != 0 && retry );



  // turn on green LED as a clear indication that core can be powered-off/rebooted
  MIOS32_BOARD_LED_Set(0xffffffff, 1);

  if( !mismatches ) {
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintString("Successful Update");
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintString("Have fun!        ");

    while( 1 ) {
      MIOS32_MIDI_SendDebugMessage("The bootloader has been successfully updated!\n");
      MIOS32_MIDI_SendDebugMessage("You can upload another application now!\n");

      // wait for 1 second before printing the message again
      Wait1Second(1);
    }
  } else {
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintString("Update failed !!! ");
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintString("Contact support!  ");

    while( 1 ) {
      MIOS32_MIDI_SendDebugMessage("Bootloader Update failed!\n");
      MIOS32_MIDI_SendDebugMessage("Thats really unexpected - probably it has to be uploaded via JTAG or UART BSL!\n");
      MIOS32_MIDI_SendDebugMessage("Please contact support if required!\n");

      // wait for 1 second before printing the message again
      Wait1Second(1);
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
