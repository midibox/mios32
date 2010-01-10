// $Id: app.c 674 2009-07-29 19:54:48Z tk $
/*
 * 
 * See README.txt for details
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
#include <string.h>

#include "app.h"
#include <uip.h>
#include <uip_arp.h>
#include <timer.h>
#include "uip_task.h"
#include "dhcpc.h"


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

volatile u8 print_msg;
volatile u8 sdcard_available;

xSemaphoreHandle xSDCardSemaphore;

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// Mutex for controlling access to SDCARD/ENC28J60
xSemaphoreHandle xSPI0Semaphore;

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  s32 i;

  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // MUST be initialized before the SPI functions
  xSPI0Semaphore = xSemaphoreCreateMutex();
  xSDCardSemaphore = xSemaphoreCreateMutex();
  
  // Init filesystem and start SD Card monitoring thread
  FS_Init(0);
  xTaskCreate(TASK_Period1S, (signed portCHAR *)"Period1S", configMINIMAL_STACK_SIZE, NULL, ( tskIDLE_PRIORITY + 4 ), NULL);
  
  // start uIP task
  UIP_TASK_Init(0); 

  // print first message
  print_msg = PRINT_MSG_INIT;
	
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
  int i;

  // clear LCD screen
  MIOS32_LCD_Clear();

  // endless loop: print status information on LCD
  while( 1 ) {
    // new message requested?
    // TODO: add FreeRTOS specific queue handling!
    u8 new_msg = PRINT_MSG_NONE;
    portENTER_CRITICAL(); // port specific FreeRTOS function to disable tasks (nested)
    if( print_msg ) {
      new_msg = print_msg;
      print_msg = PRINT_MSG_NONE; // clear request
    }
    portEXIT_CRITICAL(); // port specific FreeRTOS function to enable tasks (nested)

    switch( new_msg ) {
      case PRINT_MSG_INIT:
        MIOS32_LCD_CursorSet(0, 0);
        MIOS32_LCD_PrintString("see README.txt   ");
        MIOS32_LCD_CursorSet(0, 1);
        MIOS32_LCD_PrintString("for details     ");
	break;

      case PRINT_MSG_STATUS:
      {
        MIOS32_LCD_CursorSet(0, 0);

	// request status screen again (will stop once a new screen is requested by another task)
	print_msg = PRINT_MSG_STATUS;
      }
      break;
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

static void TASK_Period1S(void *pvParameters)
{
  portTickType xLastExecutionTime;
  sdcard_available = 0;
  MIOS32_MIDI_SendDebugMessage("About to initialize SDCard Reader\n");

  // initialize SD Card
  MIOS32_SDCARD_Init(0);
  //MIOS32_SDCARD_PowerOn(); not needed.
  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
     vTaskDelayUntil(&xLastExecutionTime, 1000 / portTICK_RATE_MS);
	 // Must have this mutex before accessing the sdcard.
     MUTEX_SDCARD_TAKE
     // check if SD card is available
     // High-speed access if SD card was previously available
     u8 prev_sdcard_available = sdcard_available;
     sdcard_available = MIOS32_SDCARD_CheckAvailable(prev_sdcard_available);

    if( sdcard_available && !prev_sdcard_available ) {
      MIOS32_MIDI_SendDebugMessage("SD Card has been connected!\n");
      s32 status;
      if( (status=FS_mount_fs()) < 0 ) {
        MIOS32_MIDI_SendDebugMessage("File system cannot be mounted, status: %d\n", status);
      } //else {
		// This is where we do any post mounting tasks.
      // }
    } else if( !sdcard_available && prev_sdcard_available ) {
        MIOS32_MIDI_SendDebugMessage("SD Card has been disconnected!\n");
    }
	MUTEX_SDCARD_GIVE
  }

}


void APP_MutexSPI0Take(void)
{
	// This must block as we aren't telling the calling process that the semaphore couldn't be obtained!
	while( xSemaphoreTake(xSPI0Semaphore, (portTickType)1) != pdTRUE ); 
	return;	
}


void APP_MutexSPI0Give(void)
{
	xSemaphoreGive(xSPI0Semaphore);
	return;	
}

