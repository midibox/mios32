// $Id$
/*
 * MIDI USB 2x2 Interface Driver
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

#include <midimon.h>

#include "app.h"
#include "terminal.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////
#define LED_PWM_PERIOD   50 // *100 uS -> 5 mS

#define NUM_LED_TRIGGERS 5  // Board LED + dedicated LED for each port


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static u8 led_trigger[NUM_LED_TRIGGERS];
static u8 led_pwm_counter[NUM_LED_TRIGGERS];


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void APP_Periodic_100uS(void);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  int i;

  // init terminal
  TERMINAL_Init(0);

  // init MIDImon
  MIDIMON_Init(0);

  // initialize status LED
  MIOS32_BOARD_LED_Init(0xffffffff);
  MIOS32_BOARD_LED_Set(1, 0);
  led_pwm_counter[0] = LED_PWM_PERIOD;
  led_trigger[0] = LED_PWM_PERIOD; // trigger LED on startup for complete PWM cycle

  // initialize additional LEDs connected to J5A
  for(i=0; i<4; ++i) {
    led_pwm_counter[1+i] = LED_PWM_PERIOD;
    led_trigger[1+i] = LED_PWM_PERIOD; // trigger LED on startup for complete PWM cycle
    MIOS32_BOARD_J5_PinInit(i, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
    MIOS32_BOARD_J5_PinSet(i, 0);
  }

  // initialize J5B/J5C pins as inputs with pull-up enabled
  // these pins control diagnostic options of the MIDI monitor
  for(i=4; i<12; ++i)
    MIOS32_BOARD_J5_PinInit(i, MIOS32_BOARD_PIN_MODE_INPUT_PU);

  // install timer function which is called each 100 uS
  MIOS32_TIMER_Init(0, 100, APP_Periodic_100uS, MIOS32_IRQ_PRIO_MID);
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
    // do nothing
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // forward packages USBx->UARTx and UARTx->USBx
  switch( port ) {
    case USB0:
      MIOS32_MIDI_SendPackage(UART0, midi_package);
      led_trigger[0] = LED_PWM_PERIOD; // Board LED
      led_trigger[1] = LED_PWM_PERIOD; // J5A.0
      break;

    case USB1:
      MIOS32_MIDI_SendPackage(UART1, midi_package);
      led_trigger[0] = LED_PWM_PERIOD; // Board LED
      led_trigger[2] = LED_PWM_PERIOD; // J5A.1
      break;

    case UART0:
      MIOS32_MIDI_SendPackage(USB0, midi_package);
      led_trigger[0] = LED_PWM_PERIOD; // Board LED
      led_trigger[3] = LED_PWM_PERIOD; // J5A.2
      break;

    case UART1:
      MIOS32_MIDI_SendPackage(USB1, midi_package);
      led_trigger[0] = LED_PWM_PERIOD; // Board LED
      led_trigger[4] = LED_PWM_PERIOD; // J5A.3
      break;
  }

  // forward to MIDI Monitor
  // SysEx messages have to be filtered for USB0 and UART0 to avoid data corruption
  // (the SysEx stream would interfere with monitor messages)
  u8 filter_sysex_message = (port == USB0) || (port == UART0);
  MIDIMON_Receive(port, midi_package, filter_sysex_message);
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
// This timer function is periodically called each 100 uS
/////////////////////////////////////////////////////////////////////////////
static void APP_Periodic_100uS(void)
{
  // this is a very simple way to generate a nice PWM based flashing effect
  // for multiple LEDs from a single timer

  u8 *led_trigger_ptr = (u8 *)&led_trigger[0];
  u8 *led_pwm_counter_ptr = (u8 *)&led_pwm_counter[0];
  int i;
  for(i=0; i<NUM_LED_TRIGGERS; ++i, ++led_trigger_ptr, ++led_pwm_counter_ptr) {
    if( *led_trigger_ptr ) {
      if( --*led_pwm_counter_ptr ) {
	if( *led_pwm_counter_ptr == *led_trigger_ptr ) {
	  if( i == 0 )
	    MIOS32_BOARD_LED_Set(1, 1);
	  else
	    MIOS32_BOARD_J5_PinSet(i-1, 1);
	}
      } else {
	*led_pwm_counter_ptr = LED_PWM_PERIOD;
	--*led_trigger_ptr;

	if( i == 0 )
	  MIOS32_BOARD_LED_Set(1, 0);
	else
	  MIOS32_BOARD_J5_PinSet(i-1, 0);
      }
    }
  }
}
