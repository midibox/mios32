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

#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////
#define LED_PWM_PERIOD   50 // *100 uS -> 5 mS

#define NUM_LED_TRIGGERS 5  // Board LED + dedicated LED for each port

#define NUM_SYSEX_PORTS  (3*16) // prepared for whole USB, UART, IIC range


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static u8 port_sysex_ctr[NUM_SYSEX_PORTS];
static mios32_midi_package_t port_sysex_package[NUM_SYSEX_PORTS];

static u8 led_trigger[NUM_LED_TRIGGERS];
static u8 led_pwm_counter[NUM_LED_TRIGGERS];

static u32 ms_counter;


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

  // clear mS counter
  ms_counter = 0;

  // init MIDImon
  MIDIMON_Init(0);

  // init port SysEx status
  for(i=0; i<NUM_SYSEX_PORTS; ++i) {
    port_sysex_ctr[i] = 0;
    port_sysex_package[i].ALL = 0;
  }

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
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(mios32_midi_port_t port, mios32_midi_package_t midi_package)
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
  // SysEx messages have to be filtered for UART0 to avoid data corruption
  // (the SysEx stream would interfere with monitor messages)
  u8 filter_sysex_message = port == UART0;
  MIDIMON_Receive(port, midi_package, ms_counter, filter_sysex_message);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedSysEx(mios32_midi_port_t port, u8 sysex_byte)
{
  // System Exclusive Events are serialized by MIOS32, and will be de-serialized
  // here. This approach has the advantage, that MIOS32 can monitor SysEx streams
  // to issue a time out if required (e.g. MIDI cable deconnected)
  // It has the disadvantage, that some additional programming effort is required
  // to parse the stream and sort it into (USB compliant) packages

  // Somebody could think, that it would be better if MIOS32 would call
  // APP_NotifyReceivedEvent() directly on each incoming SysEx package, but
  // this could quickly lead to unexpected effects at the application side,
  // e.g. if the OS uses USB package type 0xf (single byte events) to send
  // the beginning part of a SysEx stream, as observed with PortMIDI under MacOS!

  // Accordingly, this cumbersome data flow should lead to the most robust
  // results - in the hope, that this application will never find its way into
  // the MIDI Interface Blacklist! -> http://www.midibox.org/dokuwiki/doku.php?id=midi_interface_blacklist

  // propagate realtime message immediately
  if( sysex_byte >= 0xf8 ) {
    mios32_midi_package_t p;
    p.ALL = 0;
    p.type = 0x5; // Single-byte system common message
    p.evnt0 = sysex_byte;
    APP_NotifyReceivedEvent(port, p);
  } else {
    int sysex_port = port - (int)USB0; // starting with USB0
    if( sysex_port >= 0x00 && sysex_port < NUM_SYSEX_PORTS ) {

      if( sysex_byte == 0xf0 ) {
	// propagate old package if it contains data of previous SysEx stream
	if( port_sysex_ctr[sysex_port] ) {
	  port_sysex_package[sysex_port].type = 0x4 + port_sysex_ctr[sysex_port]; // 5, 6 or 7
	  APP_NotifyReceivedEvent(port, port_sysex_package[sysex_port]);
	}

	// start new package
	port_sysex_package[sysex_port].ALL = 0;
	port_sysex_package[sysex_port].type = 0x4; // SysEx starts or continues
	port_sysex_package[sysex_port].evnt0 = sysex_byte;
	port_sysex_ctr[sysex_port] = 1;
      } else if( sysex_byte == 0xf7 ) {
	// send final package
	switch( port_sysex_ctr[sysex_port] ) {
	  case 0:
	    port_sysex_package[sysex_port].evnt0 = 0xf7;
	    port_sysex_package[sysex_port].type = 0x5; // Single-byte System Common Message or SysEx ends with following single bytes
	    break;

	  case 1:
	    port_sysex_package[sysex_port].evnt1 = 0xf7;
	    port_sysex_package[sysex_port].type = 0x6; // SysEx ends with following two bytes
	    break;

	  default:
	    port_sysex_package[sysex_port].evnt2 = 0xf7;
	    port_sysex_package[sysex_port].type = 0x7; // SysEx ends with following three bytes
	    break;
	}
	APP_NotifyReceivedEvent(port, port_sysex_package[sysex_port]);

	port_sysex_ctr[sysex_port] = 0; // SysEx stream finished
      } else {
	switch( ++port_sysex_ctr[sysex_port] ) {
	  case 1:
	    port_sysex_package[sysex_port].ALL = 0;
	    port_sysex_package[sysex_port].type = 0x4; // SysEx starts or continues
	    port_sysex_package[sysex_port].evnt0 = sysex_byte;
	    break;

	  case 2:
	    port_sysex_package[sysex_port].evnt1 = sysex_byte;
	    break;

	  default:
	    port_sysex_package[sysex_port].evnt2 = sysex_byte;

	    // send package
	    APP_NotifyReceivedEvent(port, port_sysex_package[sysex_port]);
	    // waiting for next SysEx bytes
	    port_sysex_ctr[sysex_port] = 0;
	}
      }
    }
  }
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
// This timer function is periodically called each 100 uS
/////////////////////////////////////////////////////////////////////////////
static void APP_Periodic_100uS(void)
{
  static u8 pre_ctr = 0;

  // increment the microsecond counter each 10th tick
  if( ++pre_ctr >= 10 ) {
    pre_ctr = 0;
    ++ms_counter;
  }

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
