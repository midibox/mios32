// $Id$
/*
 * MIDIbox SEQ V4
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
#include <seq_demo.h>

#include "app.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_PERIOD1MS		( tskIDLE_PRIORITY + 2 )


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_PERIOD1MS(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Encoder Configuration
/////////////////////////////////////////////////////////////////////////////
#define NUM_ENCODERS 17
const mios32_enc_config_t encoders[NUM_ENCODERS] = {
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 6, .cfg.pos=2 }, // Data Wheel

  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 1, .cfg.pos=6 }, // V-Pot 1
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 1, .cfg.pos=4 }, // V-Pot 2
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 1, .cfg.pos=2 }, // V-Pot 3
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 1, .cfg.pos=0 }, // V-Pot 4
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 3, .cfg.pos=6 }, // V-Pot 5
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 3, .cfg.pos=4 }, // V-Pot 6
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 3, .cfg.pos=2 }, // V-Pot 7
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 3, .cfg.pos=0 }, // V-Pot 8
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 4, .cfg.pos=6 }, // V-Pot 9
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 4, .cfg.pos=4 }, // V-Pot 10
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 4, .cfg.pos=2 }, // V-Pot 11
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 4, .cfg.pos=0 }, // V-Pot 12
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 5, .cfg.pos=6 }, // V-Pot 13
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 5, .cfg.pos=4 }, // V-Pot 14
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 5, .cfg.pos=2 }, // V-Pot 15
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 5, .cfg.pos=0 }, // V-Pot 16
};


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  s32 i;

  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

#if 1
  // initialize encoders
  for(i=0; i<NUM_ENCODERS; ++i)
    MIOS32_ENC_ConfigSet(i, encoders[i]);
#endif

  // initialize second CLCD
  MIOS32_LCD_DeviceSet(1);
  MIOS32_LCD_Init(0);
  MIOS32_LCD_DeviceSet(0);

  // init MBSEQ Core
  Init(0);

  // start periodical 1mS task
  xTaskCreate(TASK_PERIOD1MS, (signed portCHAR *)"Period1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD1MS, NULL);

#ifdef COM_DEBUG
  // for debugging
  printf("Application initialized\n\r");
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
//void APP_Background(void)
//{
//}
// located in seq_demo


/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
//void APP_NotifyReceivedEvent(u8 port, mios32_midi_package_t midi_package)
//{
//}
// located in seq_demo


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
//void APP_NotifyReceivedSysEx(u8 port, u8 sysex_byte)
//{
//}
// located in seq_demo


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a byte has been received via COM interface
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedCOM(mios32_com_port_t port, u8 byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
//void APP_SRIO_ServicePrepare(void)
//{
//}
// located in seq_demo


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
//void APP_SRIO_ServiceFinish(void)
//{
//}
// located in seq_demo


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
//void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
//{
//}
// located in seq_demo


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
//void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
//{
//}
// located in seq_demo


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS
/////////////////////////////////////////////////////////////////////////////
static void TASK_PERIOD1MS(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // continue in application hook
    SEQ_TASK_Period1mS();
  }
}
