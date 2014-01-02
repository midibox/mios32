// $Id$
/*
 * USB OSC MIDI Proxy
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

#include <eeprom.h>

#include "app.h"
#include "presets.h"
#include "terminal.h"
#include "midimon.h"
#include "tasks.h"
#include "uip_task.h"
#include "osc_client.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////
#define LED_PWM_PERIOD   50 // *100 uS -> 5 mS

#define NUM_LED_TRIGGERS 5  // Board LED + dedicated LED for each port


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
// Mutex for MIDI IN/OUT handler
xSemaphoreHandle xMIDIINSemaphore;
xSemaphoreHandle xMIDIOUTSemaphore;



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static u8 led_trigger[NUM_LED_TRIGGERS];
static u8 led_pwm_counter[NUM_LED_TRIGGERS];

// SysEx buffer for each input
#define NUM_SYSEX_BUFFERS     6
#define SYSEX_BUFFER_IN_USB0  0
#define SYSEX_BUFFER_IN_USB1  1
#define SYSEX_BUFFER_IN_UART0 2
#define SYSEX_BUFFER_IN_UART1 3
#define SYSEX_BUFFER_IN_OSC0  4
#define SYSEX_BUFFER_IN_OSC1  5

#define SYSEX_BUFFER_SIZE 1024
static u8 sysex_buffer[NUM_SYSEX_BUFFERS][SYSEX_BUFFER_SIZE];
static u32 sysex_buffer_len[NUM_SYSEX_BUFFERS];


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

  // create semaphores
  xMIDIINSemaphore = xSemaphoreCreateRecursiveMutex();
  xMIDIOUTSemaphore = xSemaphoreCreateRecursiveMutex();

  // clear SysEx buffers
  for(i=0; i<NUM_SYSEX_BUFFERS; ++i)
    sysex_buffer_len[i] = 0;

  // install SysEx callback
  MIOS32_MIDI_SysExCallback_Init(APP_SYSEX_Parser);

  // read EEPROM content
  PRESETS_Init(0);

  // init terminal
  TERMINAL_Init(0);

  // init MIDImon
  MIDIMON_Init(0);

  // start uIP task
  UIP_TASK_Init(0);

  // initialize status LED
  MIOS32_BOARD_LED_Init(0xffffffff);
  MIOS32_BOARD_LED_Set(1, 0);
  led_pwm_counter[0] = LED_PWM_PERIOD;
  led_trigger[0] = LED_PWM_PERIOD; // trigger LED on startup for complete PWM cycle

  // initialize additional LEDs connected to J5A
  for(i=1; i<NUM_LED_TRIGGERS; ++i) {
    led_pwm_counter[i] = LED_PWM_PERIOD;
    led_trigger[i] = LED_PWM_PERIOD; // trigger LED on startup for complete PWM cycle
    MIOS32_BOARD_J5_PinInit(i-1, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
    MIOS32_BOARD_J5_PinSet(i-1, 0);
  }

  // initialize J5B/J5C pins as inputs with pull-up enabled
  // these pins control diagnostic options of the MIDI monitor
  for(i=4; i<12; ++i)
    MIOS32_BOARD_J5_PinInit(i, MIOS32_BOARD_PIN_MODE_INPUT_PU);

  // install timer function which is called each 100 uS
  MIOS32_TIMER_Init(0, 100, APP_Periodic_100uS, MIOS32_IRQ_PRIO_MID);

  // print welcome message on MIOS terminal
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("=====================\n");
  MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  MIOS32_MIDI_SendDebugMessage("=====================\n");
  MIOS32_MIDI_SendDebugMessage("\n");
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
  // SysEx handled by APP_SYSEX_Parser()
  if( midi_package.type >= 4 && midi_package.type <= 7 )
    return;

  switch( port ) {
  case USB0:
    MIOS32_MIDI_SendPackage(UART0, midi_package);
    OSC_CLIENT_SendMIDIEvent(0, midi_package);
    led_trigger[0] = LED_PWM_PERIOD; // Board LED
    led_trigger[1] = LED_PWM_PERIOD; // J5A.0
    break;

  case USB1:
    MIOS32_MIDI_SendPackage(UART1, midi_package);
    OSC_CLIENT_SendMIDIEvent(1, midi_package);
    led_trigger[0] = LED_PWM_PERIOD; // Board LED
    led_trigger[2] = LED_PWM_PERIOD; // J5A.1
    break;

  case UART0:
    MIOS32_MIDI_SendPackage(USB0, midi_package);
    OSC_CLIENT_SendMIDIEvent(2, midi_package);
    led_trigger[0] = LED_PWM_PERIOD; // Board LED
    led_trigger[3] = LED_PWM_PERIOD; // J5A.2
    break;

  case UART1:
    MIOS32_MIDI_SendPackage(USB1, midi_package);
    OSC_CLIENT_SendMIDIEvent(3, midi_package);
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
// This function parses an incoming sysex stream for MIOS32 commands
/////////////////////////////////////////////////////////////////////////////
s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // determine SysEx buffer
  int sysex_in = 0;

  switch( port ) {
  case USB0: sysex_in = SYSEX_BUFFER_IN_USB0; break;
  case USB1: sysex_in = SYSEX_BUFFER_IN_USB1; break;
  case UART0: sysex_in = SYSEX_BUFFER_IN_UART0; break;
  case UART1: sysex_in = SYSEX_BUFFER_IN_UART1; break;
  case OSC0: sysex_in = SYSEX_BUFFER_IN_OSC0; break;
  case OSC1: sysex_in = SYSEX_BUFFER_IN_OSC1; break;
  default:
    return -1; // not assigned
  }

  // store value into buffer, send when:
  //   o 0xf7 (end of stream) has been received
  //   o 0xf0 (start of stream) has been received although buffer isn't empty
  //   o buffer size has been exceeded
  // we check for (SYSEX_BUFFER_SIZE-1), so that we always have a free byte for F7
  u32 buffer_len = sysex_buffer_len[sysex_in];
  if( midi_in == 0xf7 || (midi_in == 0xf0 && buffer_len != 0) || buffer_len >= (SYSEX_BUFFER_SIZE-1) ) {

    if( midi_in == 0xf7 && buffer_len < SYSEX_BUFFER_SIZE ) // note: we always have a free byte for F7
      sysex_buffer[sysex_in][sysex_buffer_len[sysex_in]++] = midi_in;

    switch( port ) {
    case USB0:
      MIOS32_MIDI_SendSysEx(UART0, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
      OSC_CLIENT_SendSysEx(0, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
      break;
    case USB1:
      MIOS32_MIDI_SendSysEx(UART1, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
      OSC_CLIENT_SendSysEx(1, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
      break;

    case UART0:
      MIOS32_MIDI_SendSysEx(USB0, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
      OSC_CLIENT_SendSysEx(2, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
      break;
    case UART1:
      MIOS32_MIDI_SendSysEx(USB1, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
      OSC_CLIENT_SendSysEx(3, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
      break;

    case OSC0:
      MIOS32_MIDI_SendSysEx(USB0, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
      MIOS32_MIDI_SendSysEx(UART0, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
      break;
    case OSC1:
      MIOS32_MIDI_SendSysEx(USB1, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
      MIOS32_MIDI_SendSysEx(UART1, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
      break;
    }

    // empty buffer
    sysex_buffer_len[sysex_in] = 0;

    // fill with next byte if buffer size hasn't been exceeded
    if( midi_in != 0xf7 )
      sysex_buffer[sysex_in][sysex_buffer_len[sysex_in]++] = midi_in;

  } else {
    // add to buffer
    sysex_buffer[sysex_in][sysex_buffer_len[sysex_in]++] = midi_in;
  }

  return 0; // no error
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
