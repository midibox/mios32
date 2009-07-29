// $Id$
/*
 * USB Mass Storage Device
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
#include <msd.h>

#include "app.h"

#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIDI
// should be enabled for this application!
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_SDCARD	( tskIDLE_PRIORITY + 2 )


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////
static u8 sdcard_available;


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_SDCard(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
s32 MASS_PROP_Init(u32 mode);
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // init SD Card driver
  MIOS32_SDCARD_Init(0);

  // start task
  xTaskCreate(TASK_SDCard, (signed portCHAR *)"SDCard", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_SDCARD, NULL);

#if DEBUG_VERBOSE_LEVEL >= 1
  // print welcome message on MIOS terminal
  DEBUG_MSG("\n");
  DEBUG_MSG("====================\n");
  DEBUG_MSG("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  DEBUG_MSG("====================\n");
  DEBUG_MSG("\n");
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS
/////////////////////////////////////////////////////////////////////////////
static void TASK_SDCard(void *pvParameters)
{
  u16 second_delay_ctr = 0;
  portTickType xLastExecutionTime;

  // turn off LED (will be turned on once SD card connected/detected)
  MIOS32_BOARD_LED_Set(0x1, 0x0);

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // each second:
    // check if SD card is available
    // High-speed access if SD card was previously available

    if( ++second_delay_ctr >= 1000 ) {
      second_delay_ctr = 0;

      u8 prev_sdcard_available = sdcard_available;
      sdcard_available = MIOS32_SDCARD_CheckAvailable(prev_sdcard_available);

      if( sdcard_available && !prev_sdcard_available ) {
	MIOS32_BOARD_LED_Set(0x1, 0x1); // turn on LED

#if DEBUG_VERBOSE_LEVEL >= 0
	// always print...
	DEBUG_MSG("SD Card has been connected!\n");
#endif

	// switch to mass storage device
	MSD_Init(0);

    } else if( !sdcard_available && prev_sdcard_available ) {
	// re-init USB for MIDI
	MIOS32_USB_Init(1);

	MIOS32_BOARD_LED_Set(0x1, 0x0); // turn off LED
#if DEBUG_VERBOSE_LEVEL >= 0
	// always print...
	DEBUG_MSG("SD Card disconnected!\n");
#endif
      }
    }

    // each millisecond:
    // handle USB access if device is available
    if( sdcard_available )
      MSD_Periodic_mS();
  }
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // init LCD
  MIOS32_LCD_Clear();
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("see README.txt   ");
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("for details     ");

  // endless loop
  while( 1 ) {
    if( sdcard_available ) {
      MIOS32_LCD_CursorSet(0, 0);
      MIOS32_LCD_PrintString("SD Card     ");
      MIOS32_LCD_PrintChar(MSD_CheckAvailable() ? 'U' : ' ');
      MIOS32_LCD_PrintChar(MSD_LUN_AvailableGet(0) ? 'M' : ' ');
      MIOS32_LCD_PrintChar(MSD_RdLEDGet(250) ? 'R' : ' ');
      MIOS32_LCD_PrintChar(MSD_WrLEDGet(250) ? 'W' : ' ');
      MIOS32_LCD_CursorSet(0, 1);
      MIOS32_LCD_PrintString("connected       ");
    } else {
      MIOS32_LCD_CursorSet(0, 0);
      MIOS32_LCD_PrintString("SD Card         ");
      MIOS32_LCD_CursorSet(0, 1);
      MIOS32_LCD_PrintString("disconnected    ");
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
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
