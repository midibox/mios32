// $Id$
/*
 * MIOS32 Tutorial #022: Graphical LCD with Touchpanel
 * see README.txt for details
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

#include <glcd_font.h>


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// X/Y/Pressed state
static volatile u8 touchpanel_x, touchpanel_y, touchpanel_pressed;

// if 0: scan y direction, if 1: scan x direction
static u8 scan_x;

// used to insert setup times when alternating the direction
static u8 setup_time_ctr;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 AIN_ServicePrepare(void);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize scan direction
  touchpanel_pressed = 0;
  scan_x = 0;

  // install callback function which is called before each AIN channel scan
  MIOS32_AIN_ServicePrepareCallback_Init(AIN_ServicePrepare);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // print static screen
  MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);

  // clear LCD
  MIOS32_LCD_Clear();

  u8 last_touchpanel_x = 0;
  u8 last_touchpanel_y = 0;

  // endless loop - LED will flicker on each iteration
  while( 1 ) {
    // check for X/Y coordinate changes
    if( touchpanel_x != last_touchpanel_x || touchpanel_y != last_touchpanel_y ) {
      // clear marker at last position
      MIOS32_LCD_GCursorSet(last_touchpanel_x, last_touchpanel_y / 2);
      MIOS32_LCD_PrintChar(' ');

      // clear coordinate at the left/right side if required
      if( (last_touchpanel_x < 64 && touchpanel_x >= 64) || (last_touchpanel_x >= 64 && touchpanel_x < 64) ) {
	MIOS32_LCD_GCursorSet((last_touchpanel_x < 64) ? 128-5*6 : 0, 0*8);
	MIOS32_LCD_PrintString("     ");
	MIOS32_LCD_GCursorSet((last_touchpanel_x < 64) ? 128-5*6 : 0, 1*8);
	MIOS32_LCD_PrintString("     ");
      }

      // set marker at new position
      MIOS32_LCD_GCursorSet(touchpanel_x, touchpanel_y / 2);
      MIOS32_LCD_PrintChar('x');

      // print new coordinates
      MIOS32_LCD_GCursorSet((touchpanel_x < 64) ? 128-5*6 : 0, 0*8);
      MIOS32_LCD_PrintFormattedString("X:%3d", touchpanel_x);
      MIOS32_LCD_GCursorSet((touchpanel_x < 64) ? 128-5*6 : 0, 1*8);
      MIOS32_LCD_PrintFormattedString("Y:%3d", touchpanel_y);

      // store new position
      last_touchpanel_x = touchpanel_x;
      last_touchpanel_y = touchpanel_y;
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the main task which also handles DIN, ENC
// and AIN events. You could add more jobs here, but they shouldn't consume
// more than 300 uS to ensure the responsiveness of buttons, encoders, pots.
// Alternatively you could create a dedicated task for application specific
// jobs as explained in $MIOS32_PATH/apps/tutorials/006_rtos_tasks
/////////////////////////////////////////////////////////////////////////////
void APP_Tick(void)
{
  // PWM modulate the status LED (this is a sign of life)
  u32 timestamp = MIOS32_TIMESTAMP_Get();
  MIOS32_BOARD_LED_Set(1, (timestamp % 20) <= ((timestamp / 100) % 10));
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the MIDI task which checks for incoming
// MIDI events. You could add more MIDI related jobs here, but they shouldn't
// consume more than 300 uS to ensure the responsiveness of incoming MIDI.
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_Tick(void)
{
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
// This hook is called before an AIN channel scan
// It has been installed in APP_Init()
/////////////////////////////////////////////////////////////////////////////
s32 AIN_ServicePrepare(void)
{
  // increment counter, skip on the first 9 invocations (= 9 mS setup time)
  if( ++setup_time_ctr < 10 )
    return 1; // skip scan (see documentation of MIOS32_AIN_ServicePrepareCallback_Init())

  // scan with the 10th invocation
  if( setup_time_ctr++ == 10 )
    return 0; // allow scan, value changes will be forwarded to APP_AIN_NotifyChange()

  // reset counter
  setup_time_ctr = 0;

  // toggle scan direction
  scan_x ^= 1;

  // re-initialize AIN pins at J5A (Pin C0..C3)
  // Touchpanel connections:
  // J5.A0: Right
  // J5.A1: Top
  // J5.A2: Left
  // J5.A3: Bottom

  if( scan_x ) {
    // X Scan

    // A2 (Left) = 0V
    MIOS32_BOARD_J5_PinInit(2, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
    MIOS32_BOARD_J5_PinSet(2, 0);

    // A0 (Right) = 3.3V
    MIOS32_BOARD_J5_PinInit(0, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
    MIOS32_BOARD_J5_PinSet(0, 1);

    // A1 (Top) used as ADC Input
    MIOS32_BOARD_J5_PinInit(1, MIOS32_BOARD_PIN_MODE_ANALOG);

    // switch A3 (Bottom) to high impedance state
    MIOS32_BOARD_J5_PinInit(3, MIOS32_BOARD_PIN_MODE_ANALOG);
  } else {
    // Y Scan

    // A1 (Top) = 0V
    MIOS32_BOARD_J5_PinInit(1, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
    MIOS32_BOARD_J5_PinSet(1, 0);

    // A3 (Bottom) = 3.3V
    MIOS32_BOARD_J5_PinInit(3, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
    MIOS32_BOARD_J5_PinSet(3, 1);

    // A0 (Right) used as ADC Input
    MIOS32_BOARD_J5_PinInit(0, MIOS32_BOARD_PIN_MODE_ANALOG);

    // switch A2 (Left) to high impedance state
    MIOS32_BOARD_J5_PinInit(2, MIOS32_BOARD_PIN_MODE_ANALOG);
  }

  return 1; // skip conversion
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  // used to determine changes in X/Y position
  static u8 last_touchpanel_x = 0;
  static u8 last_touchpanel_y = 0;

  // convert 12bit -> 7bit
  u8 value_7bit = pin_value >> 5;

  // Take pin 0 if scan in Y direction
  if( scan_x == 0 && pin == 0 ) {
    if( value_7bit > 16 ) { // Y is usually >= 20 when touchpanel pressed
      touchpanel_pressed = 1;
      touchpanel_y = value_7bit;
    } else {
      // touchpanel not pressed - ignore conversion values
      touchpanel_pressed = 0;
    }
  }

  // Take pin 1 if scan in X direction
  // Take it only if touchpanel pressed
  if( touchpanel_pressed && scan_x == 1 && pin == 1 )
    touchpanel_x = value_7bit;

  // send debug message to terminal if position has changed
  if( last_touchpanel_x != touchpanel_x || last_touchpanel_y != touchpanel_y ) {
      last_touchpanel_x = touchpanel_x;
      last_touchpanel_y = touchpanel_y;
      MIOS32_MIDI_SendDebugMessage("X/Y = %3d/%3d\n", touchpanel_x, touchpanel_y);
  }
}
