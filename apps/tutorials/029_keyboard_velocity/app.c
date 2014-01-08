// $Id$
/*
 * MIOS32 Tutorial #029: Fast Scan Matrix for velocity sensitive Keyboard
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

// for velocity
static u16 timestamp;
static u16 din_activated_timestamp[KEYBOARD_NUM_PINS];

// for debouncing each pin has a flag which notifies if the Note On/Off value has already been sent
#if (KEYBOARD_NUM_PINS % 8)
# error "KEYBOARD_NUM_PINS must be dividable by 8!"
#endif
static u8 din_note_on_sent[KEYBOARD_NUM_PINS / 8];
static u8 din_note_off_sent[KEYBOARD_NUM_PINS / 8];

// soft-configuration (could be changed during runtime)
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

  for(i=0; i<KEYBOARD_NUM_PINS/8; ++i) {
    din_note_on_sent[i] = 0x00;
    din_note_off_sent[i] = 0x00;
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
    // do nothing
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
  // the DIN scan was done with previous row selection, not the current one:
  int prev_row = selected_row ? (selected_row - 1) : (MATRIX_NUM_ROWS - 1);

  u8 sr = MATRIX_DIN_SR;
  MIOS32_DIN_SRChangedGetAndClear(sr-1, 0xff); // ensure that change won't be propagated to normal DIN handler
  u8 sr_value = MIOS32_DIN_SRGet(sr-1);

  // determine pin changes
  u8 changed = sr_value ^ din_value[prev_row];

  if( changed ) {
    // add them to existing notifications
    din_value_changed[prev_row] |= changed;

    // store new value
    din_value[prev_row] = sr_value;

    // store timestamp for changed pin on 1->0 transition
    u8 sr_pin;
    u8 mask = 0x01;
    for(sr_pin=0; sr_pin<8; ++sr_pin, mask <<= 1) {
      if( (changed & mask) && !(sr_value & mask) ) {
	din_activated_timestamp[prev_row*8 + sr_pin] = timestamp;
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
void BUTTON_NotifyToggle(u8 row, u8 column, u8 depressed)
{
  // determine pin number based on row/column

  // each key has two contacts, I call them "break contact" and "make contact"
  // the assignments can be determined by setting DEBUG_VERBOSE_LEVEL to 2

  // default: linear addressing (e.g. Fatar Keyboards?)
  // the make contacts are at row 0, 2, 4, 6, 8, 10, 12, 14
  // the break contacts are at row 1, 3, 5, 7, 9, 11, 13, 15

  // determine key number:
  int key = 8*(row / 2) + column;

  // check if key is assigned to an "break contact"
  u8 break_contact = (row & 1); // odd numbers

  // determine note number (here we could insert an octave shift)
  int note_number = key + 36;

  // reference to break and make pin
  int pin_make = (row)*8 + column;
  int pin_break = (row+1)*8 + column;

  // ensure valid note range
  if( note_number > 127 )
    note_number = 127;
  else if( note_number < 0 )
    note_number = 0;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("row=%d, column=%d, depressed=%d  -->  key=%d, break_contact:%d, note_number=%d\n",
	    row, column, depressed, key, break_contact, note_number);
#endif

  // determine key mask and pointers for access to combined arrays
  u8 key_mask = (1 << (key % 8));
  u8 *note_on_sent = (u8 *)&din_note_on_sent[key / 8];
  u8 *note_off_sent = (u8 *)&din_note_off_sent[key / 8];


  // break contacts don't send MIDI notes, but they are used for delay measurements,
  // and they release the Note On/Off debouncing mechanism
  if( break_contact ) {
    if( depressed ) {
      *note_on_sent &= ~key_mask;
      *note_off_sent &= ~key_mask;
    }
    return;
  }

  // branch depending on pressed or released key
  if( depressed ) {
    if( !(*note_off_sent & key_mask) ) {
      *note_off_sent |= key_mask;

#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("DEPRESSED key=%d\n", key);
#endif

      MIOS32_MIDI_SendNoteOn(KEYBOARD_MIDI_PORT, KEYBOARD_MIDI_CHN, note_number, 0x00); // velocity 0
    }

  } else {

    if( !(*note_on_sent & key_mask) ) {
      *note_on_sent |= key_mask;

      // determine timestamps between break and make contact
      u16 timestamp_break = din_activated_timestamp[pin_break];
      u16 timestamp_make = din_activated_timestamp[pin_make];
      // and the delta delay (IMPORTANT: delay variable needs same resolution like timestamps to handle overrun correctly!)
      s16 delay = timestamp_make - timestamp_break;

      int velocity = 127;
      if( delay > keyboard_delay_fastest ) {
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
      DEBUG_MSG("PRESSED key=%d, delay=%d, velocity=%d\n", key, delay, velocity);
#endif

      MIOS32_MIDI_SendNoteOn(KEYBOARD_MIDI_PORT, KEYBOARD_MIDI_CHN, note_number, velocity);
    }
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
