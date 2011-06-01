// $Id$
/*
 * Code is used from some MIOS32 tutorials
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
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

// include everything FreeRTOS related we don't understand yet ;)
#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_ENCODERS 40
#define NUM_MATRICES 2

// define priority level for MATRIX_SCAN task:
// use same priority as MIOS32 specific tasks (3)
#define PRIORITY_TASK_MATRIX_SCAN	( tskIDLE_PRIORITY + 3 )

// local prototype of the task function
static void TASK_MATRIX_Scan(void *pvParameters);

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 enc_virtual_pos[NUM_ENCODERS];

static u16 matrix16x16_button_row_values[NUM_MATRICES][16];
static u16 matrix16x16_button_row_changed[NUM_MATRICES][16];

static u8 matrix16x16_ctr;

// "ampelcode"
const u8 ledring_pattern[8] = {
  0x01,
  0x03,
  0x07,
  0x0f,
  0x1f,
  0x3f,
  0x7f,
  0xff,
};

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_MATRIX_Scan(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize rotary encoders of the same type (DETENTED2)
  int enc;
  for(enc=0; enc<NUM_ENCODERS; ++enc) {
#if FIRST_ENC_DIN_SR
    u8 pin_sr = (FIRST_ENC_DIN_SR-1) + (enc >> 2); // each DIN SR has 4 encoders connected
    u8 pin_pos = (enc & 0x3) << 1; // Pin position of first ENC channel: either 0, 2, 4 or 6

    mios32_enc_config_t enc_config = MIOS32_ENC_ConfigGet(enc);
    enc_config.cfg.type = DETENTED2; // see mios32_enc.h for available types
    enc_config.cfg.sr = pin_sr;
    enc_config.cfg.pos = pin_pos;
#if 1
    // normal speed, incrementer either 1 or -1
    enc_config.cfg.speed = NORMAL;
    enc_config.cfg.speed_par = 0;
#else
    // higher incrementer values on fast movements
    enc_config.cfg.speed = FAST;
    enc_config.cfg.speed_par = 2;
#endif
    MIOS32_ENC_ConfigSet(enc, enc_config);
#endif

    // reset virtual positions
    enc_virtual_pos[enc] = 0;
  }

  matrix16x16_ctr = 0;

  u8 mod;
  for(mod=0; mod<NUM_MATRICES; ++mod) {
    u8 row;
    for(row=0; row<16; ++row) {
      matrix16x16_button_row_values[mod][row] = 0xffff;
      matrix16x16_button_row_changed[mod][row] = 0x0000;
    }
  }

  // start task
  xTaskCreate(TASK_MATRIX_Scan, (signed portCHAR *)"MATRIX_Scan", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_MATRIX_SCAN, NULL);
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

  // endless loop - LED will flicker on each iteration
  while( 1 ) {
    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    // X/Y "position" of displays (see also comments in $MIOS32_PATH/modules/app_lcd/pcd8544/README.txt)
    const u8 lcd_x[8] = {0, 1, 2, 0, 1, 2, 0, 1}; // CS#0..7
    const u8 lcd_y[8] = {0, 0, 0, 1, 1, 1, 2, 2};

    u8 i;
    for(i=0; i<8; ++i) {
      u8 x_offset = 84*lcd_x[i];
      u8 y_offset = 6*8*lcd_y[i];

      // print text
      MIOS32_LCD_GCursorSet(x_offset + 0, y_offset + 0*8);
      MIOS32_LCD_PrintFormattedString("  PCD8544 #%d", i+1);

      MIOS32_LCD_GCursorSet(x_offset + 0, y_offset + 2*8);
      MIOS32_LCD_PrintString("  powered by  ");

      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_BIG);
      MIOS32_LCD_GCursorSet(x_offset + 0, y_offset + 3*8);
      MIOS32_LCD_PrintString("MIOS");

      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
      MIOS32_LCD_GCursorSet(x_offset + 64, y_offset + 4*8);
      MIOS32_LCD_PrintString("32");
    }
  }
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
  // 2 * 16x16 Matrix DOUTs
  if( ++matrix16x16_ctr >= 16 )
    matrix16x16_ctr = 0;

  u16 matrix_select = ~(1 << matrix16x16_ctr); // if cathodes are connected to DOUTs

#if DOUT_16x16_L
  MIOS32_DOUT_SRSet(DOUT_16x16_L-1, (matrix_select >> 0) & 0xff);
#endif
#if DOUT_16x16_R
  MIOS32_DOUT_SRSet(DOUT_16x16_R-1, (matrix_select >> 8) & 0xff);
#endif

  // 32x8 LED rings DOUTs
  u8 enc_offset = matrix16x16_ctr % 8;
#if DOUT_LEDRINGS_CATHODES
  u8 ledring_cathodes = ~(1 << enc_offset);
  MIOS32_DOUT_SRSet(DOUT_LEDRINGS_CATHODES-1, ledring_cathodes);
#endif

#if DOUT_LEDRINGS_1_8
  {
    u8 pattern = ledring_pattern[enc_virtual_pos[0 + enc_offset] >> 4];
    MIOS32_DOUT_SRSet(DOUT_LEDRINGS_1_8-1, pattern);
  }
#endif
#if DOUT_LEDRINGS_9_16
  {
    u8 pattern = ledring_pattern[enc_virtual_pos[8 + enc_offset] >> 4];
    MIOS32_DOUT_SRSet(DOUT_LEDRINGS_9_16-1, pattern);
  }
#endif
#if DOUT_LEDRINGS_17_24
  {
    u8 pattern = ledring_pattern[enc_virtual_pos[16 + enc_offset] >> 4];
    MIOS32_DOUT_SRSet(DOUT_LEDRINGS_17_24-1, pattern);
  }
#endif
#if DOUT_LEDRINGS_25_32
  {
    u8 pattern = ledring_pattern[enc_virtual_pos[24 + enc_offset] >> 4];
    MIOS32_DOUT_SRSet(DOUT_LEDRINGS_25_32-1, pattern);
  }
#endif

}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
  const u8 din_map[2*2] = {
    DIN_16x16_L0,
    DIN_16x16_R0,
    DIN_16x16_L1,
    DIN_16x16_R1
  };

  // check for DIN changes
  // Note: matrix16x16_ctr was incremented before in APP_SRIO_ServicePrepare()
  int selected_row = (matrix16x16_ctr-1) & 0xf;

  // check DINs
  int mod;
  for(mod=0; mod<NUM_MATRICES; ++mod) {
    int sr0 = din_map[2*mod+0];
    int sr1 = din_map[2*mod+1];

    u16 sr_value = 0;
    if( sr0 ) {
      MIOS32_DIN_SRChangedGetAndClear(sr0-1, 0xff); // ensure that change won't be propagated to normal DIN handler
      sr_value |= (MIOS32_DIN_SRGet(sr0-1) << 0);
    }

    if( sr1 ) {
      MIOS32_DIN_SRChangedGetAndClear(sr1-1, 0xff); // ensure that change won't be propagated to normal DIN handler
      sr_value |= (MIOS32_DIN_SRGet(sr1-1) << 8);
    }

    // determine pin changes
    u16 changed = sr_value ^ matrix16x16_button_row_values[mod][selected_row];
    
    if( changed ) {
      // add them to existing notifications
      matrix16x16_button_row_changed[mod][selected_row] |= changed;
      
      // store new value
      matrix16x16_button_row_values[mod][selected_row] = sr_value;
    }
  }
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
  // increment to virtual position and ensure that the value is in range 0..127
  int value = enc_virtual_pos[encoder] + incrementer;
  if( value < 0 )
    value = 0;
  else if( value > 127 )
    value = 127;

  // only send if value has changed
  if( enc_virtual_pos[encoder] != value ) {
    // store new value
    enc_virtual_pos[encoder] = value;

    // send event
    MIOS32_MIDI_SendCC(DEFAULT, Chn1, 0x20 + encoder, value);
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  // we have 128 pots
  // 8 pots assigned per channel

  // channel
  u8 chn = pin / 8;

  // CC number
  u8 cc_number = 0x10 + (pin % 8);

  // convert 12bit value to 7bit value
  u8 value_7bit = pin_value >> 5;

  // send MIDI event
  MIOS32_MIDI_SendCC(DEFAULT, chn, cc_number, value_7bit);
}


/////////////////////////////////////////////////////////////////////////////
// This task scans MATRIX pins periodically
/////////////////////////////////////////////////////////////////////////////
static void TASK_MATRIX_Scan(void *pvParameters)
{
  u8 old_state[12]; // to store the state of 12 pins
  u8 debounce_ctr[12]; // to store debounce delay counters
  portTickType xLastExecutionTime;

  // initialize pin state and debounce counters to inactive value
  int pin;
  for(pin=0; pin<12; ++pin) {
    old_state[pin] = 1;
    debounce_ctr[pin] = 0;
  }

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // check all shift registers for DIN pin changes
    int mod;
    for(mod=0; mod<NUM_MATRICES; ++mod) {
      int row;
      for(row=0; row<16; ++row) {
	// check if there are pin changes - must be atomic!
	MIOS32_IRQ_Disable();
	u8 changed = matrix16x16_button_row_changed[mod][row];
	matrix16x16_button_row_changed[mod][row] = 0;
	MIOS32_IRQ_Enable();

	// any pin change at this SR?
	if( !changed )
	  continue;

	// check all 16 pins of the SR
	int sr_pin;
	for(sr_pin=0; sr_pin<16; ++sr_pin)
	  if( changed & (1 << sr_pin) ) {
	    u32 pin = 16*row + sr_pin;
	    u8 value = (matrix16x16_button_row_values[mod][row] & (1 << sr_pin)) ? 1 : 0;

	    // decoding: 2*256 pins mapped to channel 1..16
	    u8 chn = pin / 16;

	    // each channel has 16 buttons
	    u8 note = mod*16 + 0x3c; // 0x3c is C-3

	    // velocity: 0x7f if button pressed, 0x00 if button depressed
	    u8 velocity = value ? 0x00 : 0x7f;

	    // send MIDI event
	    MIOS32_MIDI_SendNoteOn(DEFAULT, chn, note, velocity);
	  }
      }
    }
  }
}
