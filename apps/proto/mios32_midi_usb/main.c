// $Id$
/*
 * Demo application for MIDI and USB driver
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

#include <stm32f10x_lib.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

/////////////////////////////////////////////////////////////////////////////
// Task Priorities
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_LED_TOGGLE	( tskIDLE_PRIORITY + 1 )
#define PRIORITY_TASK_DIN_CHECK		( tskIDLE_PRIORITY + 2 )
#define PRIORITY_TASK_MIDI_RECEIVE	( tskIDLE_PRIORITY + 2 )


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_LED_Toggle(void *pvParameters);
static void TASK_DIN_Check(void *pvParameters);
static void TASK_MIDI_Receive(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Main function
/////////////////////////////////////////////////////////////////////////////
int main(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  // initialize hardware and MIOS32 modules
  MIOS32_SYS_Init(0);
  MIOS32_SRIO_Init(0);
  MIOS32_DIN_Init(0);
  MIOS32_DOUT_Init(0);
  MIOS32_MIDI_Init(0);

#if defined(_STM32_PRIMER_)
  /* Configure LED pins as output push-pull. */
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 ;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init( GPIOB, &GPIO_InitStructure );
#else
  XXX unsupported derivative XXX
#endif

  // start the tasks
  xTaskCreate(TASK_LED_Toggle, (signed portCHAR *)"LED_Toggle", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_LED_TOGGLE, NULL);
  xTaskCreate(TASK_DIN_Check,  (signed portCHAR *)"DIN_Check", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_DIN_CHECK, NULL);
  xTaskCreate(TASK_MIDI_Receive, (signed portCHAR *)"MIDI_Receive", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_MIDI_RECEIVE, NULL);

  // start the scheduler
  vTaskStartScheduler();

  // Will only get here if there was not enough heap space to create the idle task
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Application Tick Hook (called by FreeRTOS each mS)
/////////////////////////////////////////////////////////////////////////////
void vApplicationTickHook( void )
{
  // start next SRIO scan - no IRQ based notification required
  MIOS32_SRIO_ScanStart(NULL);
}


/////////////////////////////////////////////////////////////////////////////
// DIN Handler
/////////////////////////////////////////////////////////////////////////////

// will be called on pin changes (see TASK_DIN_Check)
static void DIN_NotifyToggle(u32 pin, u32 value)
{
  u8 sysex[256];
  u8 i;
  u8 res;

  // map pin and value:
  // - DOUT pins of a SR are mirrored
  // - invert DIN value (so that LED lit when button pressed)
  MIOS32_DOUT_PinSet(pin ^ 7 , value ? 0 : 1);

  // fire MIDI message depending on pin
  do {
    switch( pin ) {    
      case 0: // SysEx with single byte only
        sysex[0] = 0xf7;
        MIOS32_MIDI_SendSysEx(MIOS32_MIDI_PORT_USB0, sysex, 1);
        res = 0; // (blocking function, no retry required)
        break;
  
      case 1: // SysEx with two bytes
        sysex[0] = 0xf0;
        sysex[1] = 0xf7;
        MIOS32_MIDI_SendSysEx(MIOS32_MIDI_PORT_USB0, sysex, 2);
        res = 0; // (blocking function, no retry required)
        break;
  
      case 2: // SysEx with three bytes
        sysex[0] = 0xf0;
        sysex[1] = 0x11;
        sysex[2] = 0xf7;
        MIOS32_MIDI_SendSysEx(MIOS32_MIDI_PORT_USB0, sysex, 3);
        res = 0; // (blocking function, no retry required)
        break;
  
      case 3: // SysEx with four bytes
        sysex[0] = 0xf0;
        for(i=1; i<3; ++i) sysex[i] = i*0x11;
        sysex[3] = 0xf7;
        MIOS32_MIDI_SendSysEx(MIOS32_MIDI_PORT_USB0, sysex, 4);
        res = 0; // (blocking function, no retry required)
        break;
  
      case 4: // SysEx with five bytes
        sysex[0] = 0xf0;
        for(i=1; i<4; ++i) sysex[i] = i*0x11;
        sysex[4] = 0xf7;
        MIOS32_MIDI_SendSysEx(MIOS32_MIDI_PORT_USB0, sysex, 5);
        res = 0; // (blocking function, no retry required)
        break;
  
      case 5: // SysEx with six bytes
        sysex[0] = 0xf0;
        for(i=1; i<5; ++i) sysex[i] = i*0x11;
        sysex[5] = 0xf7;
        MIOS32_MIDI_SendSysEx(MIOS32_MIDI_PORT_USB0, sysex, 6);
        res = 0; // (blocking function, no retry required)
        break;
  
      case 6: // SysEx with seven bytes
        sysex[0] = 0xf0;
        for(i=1; i<6; ++i) sysex[i] = i*0x11;
        sysex[6] = 0xf7;
        MIOS32_MIDI_SendSysEx(MIOS32_MIDI_PORT_USB0, sysex, 7);
        res = 0; // (blocking function, no retry required)
        break;
  
      case 7: // SysEx with 128+2 bytes
        sysex[0] = 0xf0;
        for(i=1; i<129; ++i) sysex[i] = i-1;
        sysex[129] = 0xf7;
        MIOS32_MIDI_SendSysEx(MIOS32_MIDI_PORT_USB0, sysex, 128+2);
        res = 0; // (blocking function, no retry required)
        break;
  
      case 8: // special event
        res = MIOS32_MIDI_SendMTC(MIOS32_MIDI_PORT_USB0, 0x11);
        break;
  
      case 9: // special event
        res = MIOS32_MIDI_SendSongPosition(MIOS32_MIDI_PORT_USB0, 0x1234);
        break;
  
      case 10: // special event
        res = MIOS32_MIDI_SendSongSelect(MIOS32_MIDI_PORT_USB0, 0x11);
        break;
  
      case 11: // special event
        res = MIOS32_MIDI_SendClock(MIOS32_MIDI_PORT_USB0);
        break;
  
      case 12: // special event
        res = MIOS32_MIDI_SendTick(MIOS32_MIDI_PORT_USB0);
        break;
  
      case 13: // special event
        res = MIOS32_MIDI_SendStart(MIOS32_MIDI_PORT_USB0);
        break;
  
      case 14: // special event
        res = MIOS32_MIDI_SendStop(MIOS32_MIDI_PORT_USB0);
        break;
  
      case 15: // special event
        res = MIOS32_MIDI_SendContinue(MIOS32_MIDI_PORT_USB0);
        break;
  
      case 16: // special event
        res = MIOS32_MIDI_SendActiveSense(MIOS32_MIDI_PORT_USB0);
        break;
  
      case 17: // special event
        res = MIOS32_MIDI_SendReset(MIOS32_MIDI_PORT_USB0);
        break;
  
      case 18: // note off
        res = MIOS32_MIDI_SendNoteOff(MIOS32_MIDI_PORT_USB0, 0, 0x11, value ? 0x00 : 0x7f);
        break;
  
      case 19: // note on
        res = MIOS32_MIDI_SendNoteOn(MIOS32_MIDI_PORT_USB0, 0, 0x11, value ? 0x00 : 0x7f);
        break;
  
      case 20: // Poly Pressure
        res = MIOS32_MIDI_SendPolyPressure(MIOS32_MIDI_PORT_USB0, 0, 0x11, value ? 0x00 : 0x7f);
        break;
  
      case 21: // CC
        res = MIOS32_MIDI_SendCC(MIOS32_MIDI_PORT_USB0, 0, 0x11, value ? 0x00 : 0x7f);
        break;
  
      case 22: // Program Change
        res = MIOS32_MIDI_SendProgramChange(MIOS32_MIDI_PORT_USB0, 0, value ? 0x00 : 0x7f);
        break;
  
      case 23: // Aftertouch
        res = MIOS32_MIDI_SendAftertouch(MIOS32_MIDI_PORT_USB0, 0, value ? 0x00 : 0x7f);
        break;
  
      case 24: // Pitch Bend
        res = MIOS32_MIDI_SendPitchBend(MIOS32_MIDI_PORT_USB0, 0, value ? 0x0000 : 0x1234);
        break;
  
      default:   // MIDI event
        res = MIOS32_MIDI_SendNoteOn(MIOS32_MIDI_PORT_USB0, 0x00, pin, value ? 0x00 : 0x7f);
    }

    // retry required? Suspend task for 1 mS
    if( res == -2 )
      vTaskDelay(1 / portTICK_RATE_MS);
  } while( res == -2);

}

// checks for toggled DIN pins
static void TASK_DIN_Check(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // check for pin changes, call DIN_NotifyToggle on each toggled pin
    MIOS32_DIN_Handler(DIN_NotifyToggle);
  }
}


/////////////////////////////////////////////////////////////////////////////
// MIDI Handlers
/////////////////////////////////////////////////////////////////////////////

// this hook is called on received MIDI events
void MIDI_NotifyReceivedEvent(u8 port, mios32_midi_package_t midi_package)
{
  MIOS32_MIDI_SendPackage(port, midi_package);
}

// this hook is called if SysEx data is received
void MIDI_NotifyReceivedSysEx(u8 port, u8 sysex_byte)
{
  MIOS32_MIDI_SendCC(port, 0, (sysex_byte & 0x80) ? 1 : 0, sysex_byte & 0x7f);
}

// checks for incoming MIDI messages
static void TASK_MIDI_Receive(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // handle USB messages
    MIOS32_USB_Handler();
    
    // check for incoming MIDI messages and call hooks
    MIOS32_MIDI_Receive_Handler(MIDI_NotifyReceivedEvent, MIDI_NotifyReceivedSysEx);
  }
}


/////////////////////////////////////////////////////////////////////////////
// LED Toggle Task (sends a sign of life)
/////////////////////////////////////////////////////////////////////////////
static void TASK_LED_Toggle(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    // toggle the LEDs periodically
    vTaskDelayUntil(&xLastExecutionTime, (portTickType)1000 / portTICK_RATE_MS);

#if defined(_STM32_PRIMER_)
    // toggle LEDs at pin B8/B9
    if( GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_8) ) {
      GPIO_ResetBits(GPIOB, GPIO_Pin_8);
      GPIO_SetBits(GPIOB, GPIO_Pin_9);
    } else {
      GPIO_SetBits(GPIOB, GPIO_Pin_8);
      GPIO_ResetBits(GPIOB, GPIO_Pin_9);
    }
#else
  XXX unsupported derivative XXX
#endif

  }
}
