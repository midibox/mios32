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

u32 btn_change_count = 0;
u32 last_btn_change_count = 1;

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void){
	s32 i;
	// initialize all LEDs
	MIOS32_BOARD_LED_Init(0xffffffff);
	// initialize BLM driver
	BLM_X_Init();
	BLM_X_DebounceDelaySet(1);
	// start BLM check task
	xTaskCreate(TASK_BLM_Check, (signed portCHAR *)"BLM_Check", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_BLM_CHECK, NULL);
	//send init message
	MIOS32_MIDI_SendDebugMessage("BLM_X Test-Application Started");
	//display all possible LED color combinations (if matrix is big enough!)
	for(i=0;i < BLM_X_NUM_ROWS*BLM_X_LED_NUM_COLS; i++){
		if((++last_led_color) >> BLM_X_LED_NUM_COLORS)
			last_led_color = 1;
		BLM_X_LEDColorSet(i,last_led_color);
		}
	last_led_color = 0;
	}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void){
	// init LCD
	MIOS32_LCD_Clear();
	// endless loop: print status information on LCD
	while( 1 ) {
		MIOS32_LCD_CursorSet(0, 0);
		// print text on LCD screen, send debug message
		if(btn_change_count != last_btn_change_count){
			MIOS32_LCD_CursorSet(0, 0);
			MIOS32_LCD_PrintFormattedString("Button #%3d %c", last_btn, last_btn_value ? 'o' : '*');
			MIOS32_MIDI_SendDebugMessage("Button #%3d %c - Changes: %5d", last_btn, last_btn_value ? 'o' : '*',btn_change_count);
			MIOS32_LCD_CursorSet(0, 1);
			MIOS32_LCD_PrintFormattedString("Btn changes: %5d", btn_change_count);
			last_btn_change_count = btn_change_count;
			}
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
	//sets board led's to 0 (enables to measure the time consumed by the srio-service & blm stuff
	MIOS32_BOARD_LED_Set(0xffffffff, 0);
	//prepare DOUT registers to drive the row
	BLM_X_PrepareRow();
	}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void){
	// call the BLM_GetRow function after scan is finished to capture the read DIN values
	BLM_X_GetRow();
	//sets board led's back to 1 (enables to measure the time consumed by the srio-service & blm stuff
	MIOS32_BOARD_LED_Set(0xffffffff, 1);
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
	if(!value){
		// increment LED color, swap to 0 when all available color bits are set
		last_led_color++;
		if(btn != last_btn || (last_led_color >> BLM_X_LED_NUM_COLORS) )
			last_led_color = 0;
		// output LED color
		BLM_X_LEDColorSet(btn,last_led_color);
		MIOS32_MIDI_SendDebugMessage("LED #%3d color set to 0x%08x", btn, last_led_color);
		}
	// store btn & value, increment button change counter
	last_btn = btn;
	last_btn_value = value;
	btn_change_count++;
	}

static void TASK_BLM_Check(void *pvParameters){
	portTickType xLastExecutionTime;
  	// Initialize the xLastExecutionTime variable on task entry
  	xLastExecutionTime = xTaskGetTickCount();
  	while( 1 ) {
   	vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);
    	// check for BLM pin changes, call BLM_Button_NotifyToggle on each toggled pin
    	BLM_X_BtnHandler(BLM_Button_NotifyToggle);
  		}
	}
