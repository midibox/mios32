// $Id: app.c 108 2008-10-26 13:08:31Z tk $
/*
 * Checks a connected SD-Card
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Matthias Mächler (thismaechler@gmx.ch / maechler@mm-computing.ch)
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
#include <FreeRTOS.h>
#include <task.h>



/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#define max_first_sector 0xFF
#define max_subseq_check_errors  0x10
#define check_step 0xF2
#define initial_sector_inc 0x1000

u8 phase = 0;//0x00 init;0x01 determine first rw sector;0x02 determine last rw sector;0x03 deep check;0xff check done;
u32 sector;
u32 sector_inc;//current sector increment

u32 bad_sector_count;
u32 first_sector_rw;
u32 last_sector_rw;
u8 last_error_code;
u8 last_error_op;
u16 subseq_check_errors;

u8 was_available;

///////////////////////////////////////////////////////////////////////////////
// Application functions
///////////////////////////////////////////////////////////////////////////////

void init_sdcard_buffer(void);
u8 check_sdcard_buffer(void);
void clear_sdcard_buffer(void);
u8 check_sector_rw(void);
void print_long_hex(u32);

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  s32 i;

  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);
  MIOS32_SDCARD_Init(0);
  was_available = 0;
 
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
/* Block for 500ms. */
 const portTickType xDelay = 1000 / portTICK_RATE_MS;
  // init LCD
  MIOS32_LCD_Clear();
 // endless loop: print status information on LCD
  while( 1 ) {
  MIOS32_LCD_Clear();
  MIOS32_LCD_CursorSet(0,0);
 if(was_available = MIOS32_SDCARD_CheckAvailable(was_available))
	MIOS32_LCD_PrintString("SDCard available!");
  else
  	MIOS32_LCD_PrintString("SDCard not available");
	vTaskDelay( xDelay );

   
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
