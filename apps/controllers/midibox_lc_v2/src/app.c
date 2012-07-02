// $Id$
/*
 * MIDIbox LC V2
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
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include "app.h"
#include "lc_hwcfg.h"
#include "lc_lcd.h"
#include "lc_sysex.h"
#include "lc_midi.h"
#include "lc_mf.h"
#include "lc_vpot.h"
#include "lc_dio.h"
#include "lc_gpc.h"
#include "lc_leddigits.h"
#include "lc_meters.h"


/////////////////////////////////////////////////////////////////////////////
// RTOS tasks
/////////////////////////////////////////////////////////////////////////////

// define priority level for periodic handler
// use same priority as MIOS32 specific tasks
#define PRIORITY_TASK_PERIOD_1mS ( tskIDLE_PRIORITY + 3 )

// low-prio thread for LCD output
#define PRIORITY_TASK_PERIOD_1mS_LP ( tskIDLE_PRIORITY + 2 )

// local prototype of the task functions
static void TASK_Period_1mS(void *pvParameters);
static void TASK_Period_1mS_LP(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// status of LC emulation
lc_flags_t lc_flags;


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize application specific variables
  LC_HWCFG_Init(0);
  LC_SYSEX_Init(0);
  LC_VPOT_Init(0);
  LC_MF_Init(0);
  LC_GPC_Init(0);
  LC_LEDDIGITS_Init(0);
  LC_DIO_SFBLEDUpdate();

  // initialize the shift registers
  //  MIOS32_SRIO_TS_SensitivitySet(TOUCHSENSOR_SENSITIVITY); // TODO
  MIOS32_SRIO_DebounceSet(SRIO_DEBOUNCE_CTR);

  // start tasks
  xTaskCreate(TASK_Period_1mS, (signed portCHAR *)"1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS, NULL);
  xTaskCreate(TASK_Period_1mS_LP, (signed portCHAR *)"1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS_LP, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  if( port == MIOS32_MIDI_DefaultPortGet() ) {
    // forward MIDI event to MIDI handler
    LC_MIDI_Received(midi_package);

    // forward MIDI event to GPC handler
    LC_GPC_Received(midi_package);

    // pitchbend events are also forwarded to MBHP_MF_NG module
    if( midi_package.type == PitchBend ) {
      MIOS32_MIDI_SendPackage(UART2, midi_package); // STM32 and LPC17
      MIOS32_MIDI_SendPackage(UART3, midi_package); // LPC17
    }
  }

  // forward packages USB1<->UART0, USB2<->UART1 and USB3<->UART2
  switch( port ) {
  case USB1:  MIOS32_MIDI_SendPackage(UART0, midi_package); break;
  case USB2:  MIOS32_MIDI_SendPackage(UART1, midi_package); break;

  // extra for MBHP_MF_NG: messages received via USB3 are forwarded to UART2 and UART3
  case USB3: {
    MIOS32_MIDI_SendPackage(UART2, midi_package);
    MIOS32_MIDI_SendPackage(UART3, midi_package);
  } break;

  case UART0: MIOS32_MIDI_SendPackage(USB1, midi_package); break;
  case UART1: MIOS32_MIDI_SendPackage(USB2, midi_package); break;

  // extra for MBHP_MF_NG: PitchBend and Note Events are also forwarded to default port
  case UART2:
  case UART3: {
    MIOS32_MIDI_SendPackage(USB3, midi_package);

    if( midi_package.type == NoteOff || midi_package.type == NoteOn || midi_package.type == PitchBend )
      MIOS32_MIDI_SendPackage(USB0, midi_package);
  } break;
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
  // sets the LEDring (and Meter) patterns
  LC_VPOT_LEDRing_SRHandler();

  // updates the MTC display
  LC_LEDDIGITS_SRHandler();
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
  // branch to button handler
  LC_DIO_ButtonHandler(pin, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
#if MBSEQ_HARDWARE_OPTION

  // encoder number 0: Jogwheel
  // encoder number 1, 3, 5, 7, 9, 11, 13, 15: VPOT
  // encoder number 2, 4, 6, 8, 10, 12, 14, 16: Fader replacement
  if( encoder == 0 )
    LC_VPOT_SendJogWheelEvent(incrementer);
  else if( encoder < 16 ) {
    if( !(encoder & 1) )
      LC_VPOT_SendFADEREvent((encoder >> 1)-1, incrementer);
    else
      LC_VPOT_SendENCEvent((encoder >> 1)-1, incrementer);
  }
  // encoder number 16: datawheel
  else {
    // here you could handle additional encoders
  }

#else

  // encoder number 0: Jogwheel
  if( encoder == 0 )
    LC_VPOT_SendJogWheelEvent(incrementer);
  // encoder number 1..8: VPOT
  else if( encoder < 9 )
    LC_VPOT_SendENCEvent(encoder-1, incrementer);
  else {
    // here you could handle additional encoders
  }

#endif
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
#if MBSEQ_HARDWARE_OPTION == 0
  LC_MF_FaderEvent(pin, pin_value << 4); // convert 12bit to 16bit
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS to handle LC functions
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period_1mS(void *pvParameters)
{
  u8 counter_20ms = 0;

  while( 1 ) {
    vTaskDelay(1 / portTICK_RATE_MS);

    // toggle Status LED to as a sign of live
    MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

    // call the meter timer each 20 mS
    if( ++counter_20ms >= 20 ) {
      counter_20ms = 0;
      LC_METERS_Timer();
    }

    // handles the update requests for VPOT LEDrings
    LC_VPOT_LEDRing_CheckUpdates();

    // handles the update requests for meters
    LC_METERS_CheckUpdates();

    // handles the update requests for LEDs
    LC_DIO_LED_CheckUpdate();
  }
}


/////////////////////////////////////////////////////////////////////////////
// This low-prio task handles the LCD
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period_1mS_LP(void *pvParameters)
{
  // init LCD
  LC_LCD_Init(0);

  while( 1 ) {
    vTaskDelay(1 / portTICK_RATE_MS);

    // call LCD screen handler
    LC_LCD_Update(0);
  }
}
