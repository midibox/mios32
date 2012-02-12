// $Id$
/*
 * MIOS32 Tutorial #029: Keyboard Velocity
 * see README.txt for details
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose(tk@midibox.org)
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

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages
/////////////////////////////////////////////////////////////////////////////

// level >= 1: print warnings (recommended default value)
// level >= 2: print debug messages to output velocity values
#define DEBUG_VERBOSE_LEVEL 2
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_MATRIX_SCAN	( tskIDLE_PRIORITY + 2 )

// scan 16 columns for up to 8*8 keys (each key connected to two rows)
#define MATRIX_NUM_ROWS 16

// DOUT/DIN SRs (counted from 1, 0 disables SR)
#define MATRIX_DOUT_SR1 1
#define MATRIX_DOUT_SR2 2
#define MATRIX_DIN_SR   1

// maximum number of supported keys
#define KEYBOARD_NUM_PINS (8*16)

// used MIDI port and channel (DEFAULT, USB0, UART0 or UART1)
#define KEYBOARD_MIDI_PORT DEFAULT
#define KEYBOARD_MIDI_CHN  Chn1

// initial minimum/maximum delay to calculate velocity
// (will be copied into variables, so that values could be changed during runtime)
#define INITIAL_KEYBOARD_DELAY_FASTEST 4
#define INITIAL_KEYBOARD_DELAY_SLOWEST 200


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_MatrixScan(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static u8 selected_row;
static u8 din_value[MATRIX_NUM_ROWS];
static u8 din_value_changed[MATRIX_NUM_ROWS];

static u32 timestamp;
static u32 din_activated_timestamp[KEYBOARD_NUM_PINS];

static u32 keyboard_delay_fastest;
static u32 keyboard_delay_slowest;

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // start with first column
  selected_row = 0;

  // initialize DIN arrays
  int row;
  for(row=0; row<MATRIX_NUM_ROWS; ++row) {
    din_value[row] = 0xff; // default state: buttons depressed
    din_value_changed[row] = 0x00;
  }

  // initialize timestamps
  int i;
  timestamp = 0;
  for(i=0; i<KEYBOARD_NUM_PINS; ++i) {
    din_activated_timestamp[i] = 0;
  }

  // initialize keyboard delay values
  keyboard_delay_fastest = INITIAL_KEYBOARD_DELAY_FASTEST;
  keyboard_delay_slowest = INITIAL_KEYBOARD_DELAY_SLOWEST;

  // start matrix scan task
  xTaskCreate(TASK_MatrixScan, (signed portCHAR *)"MatrixScan", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_MATRIX_SCAN, NULL);

  // limit the number of DIN/DOUT SRs which will be scanned for faster scan rate
  MIOS32_SRIO_ScanNumSet(2);

  // speed up SPI transfer rate (was MIOS32_SPI_PRESCALER_128, initialized by MIOS32_SRIO_Init())
  MIOS32_SPI_TransferModeInit(MIOS32_SRIO_SPI, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_64);
  // prescaler 64 results into a transfer rate of 0.64 uS per bit
  // when 2 SRs are transfered, we are able to scan the whole 16x8 matrix in 300 uS

  // standard SRIO scan has been disabled in programming_models/traditional/main.c via MIOS32_DONT_SERVICE_SRIO_SCAN in mios32_config.h
  // start the scan here - and retrigger it whenever it's finished
  APP_SRIO_ServicePrepare();
  MIOS32_SRIO_ScanStart(APP_SRIO_ServiceFinish);
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
    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

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
  // select next column, wrap at 16
  if( ++selected_row >= MATRIX_NUM_ROWS ) {
    selected_row = 0;

    // increment timestamp for velocity delay measurements
    ++timestamp;
  }

  // selection pattern (active selection is 0, all other outputs 1)
  u16 selection_mask = ~(1 << (u16)selected_row);

  // transfer to DOUTs
#if MATRIX_DOUT_SR1
  MIOS32_DOUT_SRSet(MATRIX_DOUT_SR1-1, (selection_mask >> 0) & 0xff);
#endif
#if MATRIX_DOUT_SR2
  MIOS32_DOUT_SRSet(MATRIX_DOUT_SR2-1, (selection_mask >> 8) & 0xff);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
  // check DINs
#if MATRIX_DIN_SR
  u8 sr = MATRIX_DIN_SR;
  MIOS32_DIN_SRChangedGetAndClear(sr-1, 0xff); // ensure that change won't be propagated to normal DIN handler
  u8 sr_value = MIOS32_DIN_SRGet(sr-1);

  // determine pin changes
  u8 changed = sr_value ^ din_value[selected_row];

  if( changed ) {
    // add them to existing notifications
    din_value_changed[selected_row] |= changed;

    // store new value
    din_value[selected_row] = sr_value;

    // store timestamp for changed pin on 1->0 transition
    u8 sr_pin;
    u8 mask = 0x01;
    for(sr_pin=0; sr_pin<8; ++sr_pin, mask <<= 1) {
      if( (changed & mask) && !(sr_value & mask) ) {
	din_activated_timestamp[selected_row*8 + sr_pin] = timestamp;
      }
    }
  }

#endif

  // standard SRIO scan has been disabled in programming_models/traditional/main.c via MIOS32_DONT_SERVICE_SRIO_SCAN in mios32_config.h
  // start the scan here - and retrigger it whenever it's finished
  APP_SRIO_ServicePrepare();
  MIOS32_SRIO_ScanStart(APP_SRIO_ServiceFinish);
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
// This task is called each mS to scan the button matrix
/////////////////////////////////////////////////////////////////////////////

// will be called on button pin changes (see TASK_BLM_Check)
void BUTTON_NotifyToggle(u8 row, u8 column, u8 pin_value)
{
  // determine pin number based on row/column

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("row=%d, column=%d, pin_value=%d\n", row, column, pin_value);
#endif

  // each key has two contacts, I call them "early contact" and "final contact"
  // the assignments can be determined by setting DEBUG_VERBOSE_LEVEL to 2
  // the early contacts are at row 1, 3, 5, 7, 9
  // the final contacts are at row 2, 4, 6, 8, 10

  u8 early_contact = (row & 1); // odd numbers
  // we ignore button changes on the early contacts:
  if( early_contact )
    return;

  // determine pin number:
  int pin = 8*(row / 2) + column;

  // determine note number (here we could insert an octave shift)
  // substracted -11 because this is the first pin which can be played
  int note_number = (pin-11) + 36;

  // branch depending on pressed or released key
  if( pin_value == 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("DEPRESSED pin=%d\n", pin);
#endif

    MIOS32_MIDI_SendNoteOn(KEYBOARD_MIDI_PORT, KEYBOARD_MIDI_CHN, note_number, 0x00); // velocity 0

  } else {
    // determine timestamps between early and final contact
    u32 timestamp_early = din_activated_timestamp[(row-1)*8 + column];
    u32 timestamp_final = din_activated_timestamp[(row)*8 + column];
    // and the delta delay
    int delay = timestamp_final - timestamp_early;

    int velocity = 127;
    if( delay > delay-keyboard_delay_fastest ) {
      // determine velocity depending on delay
      // lineary scaling - here we could also apply a curve table
      velocity = 127 - (((delay-keyboard_delay_fastest) * 127) / (keyboard_delay_slowest-keyboard_delay_fastest));
      // saturate to ensure that range 1..127 won't be exceeded
      if( velocity < 1 )
	velocity = 1;
      if( velocity > 127 )
	velocity = 127;
    }

#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("PRESSED pin=%d, delay=%d, velocity=%d\n", pin, delay, velocity);
#endif

    MIOS32_MIDI_SendNoteOn(KEYBOARD_MIDI_PORT, KEYBOARD_MIDI_CHN, note_number, velocity);
  }
}


static void TASK_MatrixScan(void *pvParameters)
{
  while( 1 ) {
    // wait for next timesplice (1 mS)
    vTaskDelay(1 / portTICK_RATE_MS);

    // check for DIN pin changes
    int row;
    for(row=0; row<MATRIX_NUM_ROWS; ++row) {
        // check if there are pin changes - must be atomic!
        MIOS32_IRQ_Disable();
        u8 changed = din_value_changed[row];
        din_value_changed[row] = 0;
        MIOS32_IRQ_Enable();

        // any pin change at this SR?
        if( !changed )
          continue;

        // check all 8 pins of the SR
        int sr_pin;
        u8 mask = 0x01;
        for(sr_pin=0; sr_pin<8; ++sr_pin, mask <<= 1)
          if( changed & mask )
            BUTTON_NotifyToggle(row, sr_pin, (din_value[row] & mask) ? 1 : 0);
    }
  }
}
