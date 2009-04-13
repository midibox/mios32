// $Id: app.c 108 2008-10-26 13:08:31Z tk $
/*
 * Checks the handling of a button/LED matrix driven by the blm_x module
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Matthias Mächler (maechler@mm-computing.ch / thismaechler@gmx.ch)
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

#include <blm_x.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_BLM_CHECK		( tskIDLE_PRIORITY + 2 )


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_BLM_Check(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

u8 last_btn = 0;
u8 last_btn_value = 1;
u32 last_led_color = 0;

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void){
	s32 i;
	// initialize all LEDs
	MIOS32_BOARD_LED_Init(0xffffffff);
	// initialize BLM driver
	BLM_X_Init();
	// start BLM check task
	xTaskCreate(TASK_BLM_Check, (signed portCHAR *)"BLM_Check", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_BLM_CHECK, NULL);
	}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void){
	// init LCD
	MIOS32_LCD_Clear();
	// endless loop: print status information on LCD
	while( 1 ) {
		// toggle the state of all LEDs (allows to measure the execution speed with a scope)
		MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());
		// print text on LCD screen
		MIOS32_LCD_CursorSet(0, 0);
		MIOS32_LCD_PrintFormattedString("Button #%3d %c", last_btn, last_btn_value ? 'o' : '*');
		}
	}


/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(mios32_midi_port_t port, mios32_midi_package_t midi_package){
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
void APP_SRIO_ServicePrepare(void){
	// prepare DOUT registers to drive the row
	BLM_X_PrepareRow();
	}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void){
	// call the BLM_GetRow function after scan is finished to capture the read DIN values
	BLM_X_GetRow();
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
// This task is called each mS to check the BLM button states
/////////////////////////////////////////////////////////////////////////////

// will be called on BLM pin changes (see TASK_BLM_Check)
void BLM_Button_NotifyToggle(u32 btn, u32 value){
	last_led_color++;
	if(btn != last_btn || (last_led_color >> BLM_X_LED_NUM_COLORS) )
		last_led_color = 0;
	// output LED color
	BLM_X_LEDColorSet(btn,last_led_color);
	// store btn / value
	last_btn = btn;
	last_btn_value = value;
	}

static void TASK_BLM_Check(void *pvParameters){
	portTickType xLastExecutionTime;
  	// Initialise the xLastExecutionTime variable on task entry
  	xLastExecutionTime = xTaskGetTickCount();
  	while( 1 ) {
   	vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);
    	// check for BLM pin changes, call BLM_Button_NotifyToggle on each toggled pin
    	BLM_X_BtnHandler(BLM_Button_NotifyToggle);
  		}
	}
