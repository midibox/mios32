// $Id$
/*
 * IIC MIDI Test
 * see REAMDE.txt for details
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

#include <FreeRTOS.h>
#include <task.h>

#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// checking for connected IIC MIDI module each second
// should run at same (or higher) priority like the task which polls the MIDI
// receive status, otherwise following scenario can happen:
//   - TASK_PERIOD1S entered, it takes IIC
//   - interrupted by the "Hooks" task of the MIOS32 programming model
//   -> MIOS32_MIDI_Receive_Handler() will endless try to get the IIC bus, 
//      since the low priority TASK_PERIOD1S will never be executed anymore.
// In future, this should be properly solved by using a MUTEX for access to 
// IIC resources! (a MUTEX would temporary increase the priority of TASK_PERIOD1S)
#define PRIORITY_TASK_PERIOD1S		( tskIDLE_PRIORITY + 3 )


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static void TASK_Period1S(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // start periodic task
  xTaskCreate(TASK_Period1S,  (signed portCHAR *)"Period1S",  configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD1S, NULL);

  // initialize IIC_MIDI
  MIOS32_IIC_MIDI_Init(0);

  // send debug message
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("Connected IIC_MIDI Modules:\n");
  int i;
  for(i=0; i<MIOS32_IIC_MIDI_NUM; ++i) {
    MIOS32_MIDI_SendDebugMessage("IIC%d: %s\n", i, MIOS32_MIDI_CheckAvailable(IIC0+i) ? "yes" : "no");
  }
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // clear LCD
  MIOS32_LCD_Clear();

  // print static screen
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("Sl1 Sl2 Sl3 Sl4");

  // run endless
  while( 1 ) {
    // check for available interfaces
    static u8 last_available_iic[MIOS32_IIC_MIDI_NUM] = { 0, 0, 0, 0 };
    u8 available_iic[MIOS32_IIC_MIDI_NUM];

    int i;
    for(i=0; i<4; ++i) {
      available_iic[i] = MIOS32_MIDI_CheckAvailable(IIC0 + i);

      // print status on screen
      MIOS32_LCD_CursorSet(i*4+1, 1);
      MIOS32_LCD_PrintChar(available_iic[i] ? '*' : 'o');

      // send message if status has changed
      if( available_iic[i] != last_available_iic[i] ) {
	last_available_iic[i] = available_iic[i];
	MIOS32_MIDI_SendDebugMessage("IIC%d: %s\n", i, available_iic[i] ? "connected" : "disconnected");
      }
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


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each second
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period1S(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1000 / portTICK_RATE_MS);

    // check for availability of IIC_MIDI modules
    // retry in 1 mS until we will get the IIC bus (blocking)
    while( MIOS32_IIC_MIDI_ScanInterfaces() == -2 )
      vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);
  }
}

