// $Id$
/*
 * Example for a "fastscan button matrix"
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

#define DEBUG_VERBOSE_LEVEL 2
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_MATRIX_SCAN	( tskIDLE_PRIORITY + 2 )

// scan 16 rows
#define MATRIX_NUM_ROWS 16

// maximum number of supported keys (rowsxcolumns = 16*16)
#define KEYBOARD_NUM_PINS (16*16)

// sink drivers used? (no for Fatar keyboard)
#define MATRIX_DOUT_HAS_SINK_DRIVERS 0


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_MatrixScan(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

u16 din_value[MATRIX_NUM_ROWS];

u32 last_timestamp[KEYBOARD_NUM_PINS];


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize DIN arrays
  int row;
  for(row=0; row<MATRIX_NUM_ROWS; ++row) {
    din_value[row] = 0xffff; // default state: buttons depressed
  }

  // initialize timestamps
  int i;
  for(i=0; i<KEYBOARD_NUM_PINS; ++i) {
    last_timestamp[i] = 0;
  }

  // start matrix scan task
  xTaskCreate(TASK_MatrixScan, (signed portCHAR *)"MatrixScan", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_MATRIX_SCAN, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
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
// This task is called each mS to scan the button matrix
/////////////////////////////////////////////////////////////////////////////

// will be called on BLM pin changes (see TASK_BLM_Check)
void BUTTON_NotifyToggle(u8 row, u8 column, u8 pin_value, u32 timestamp)
{
  // determine pin number based on row/column
  // based on pin map for fadar keyboard provided by Robin
  // (pin number counted from 1, not 0, to match with the table)
  // tested with utils/test_pinmap.pl

  int pin = -1;

  // pin number (counted from 0) consists of:
  //   bit #0 if row-1 -> pin bit #0
  int bit0 = (row-1) & 1;
  //   bit #3..1 of row-1 -> pin bit #6..4
  int bit6to4 = ((row-1) & 0xe) >> 1;
  //   bit #2..0 of column -> pin bit #2..1
  int bit3to1 = column & 0x7;

  // combine to pin value
  if( column < 8 ) {
    // left half
    if( row >= 1 && row <= 0xa ) {
      pin = bit0 | (bit6to4 << 4) | (bit3to1 << 1);
    }
  } else {
    // right half
    if( row >= 1 && row <= 0xc ) {
      pin = 80 + (bit0 | (bit6to4 << 4) | (bit3to1 << 1));
    }
  }

  // calculate delay between last event
  int delay = -1;
  if( pin < KEYBOARD_NUM_PINS ) { // ensure that we never access the array outside the allocated range
    if( !last_timestamp[pin] )
      delay = 0; // very first key event - no timestamp has been stored yet
    else
      delay = timestamp - last_timestamp[pin];
  }

  // pin value = 0 -> we are starting to press the key
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[BUTTON_NotifyToggle] row=0x%02x, column=0x%02x, pin_value=%d -> pin=%d, timestamp=%d, delay=%d\n",
	    row, column, pin_value,
	    pin + 1, // +1 to match with Robin's table
	    timestamp,
	    delay);
#endif

  // store timestamp
  if( pin < KEYBOARD_NUM_PINS )
    last_timestamp[pin] = timestamp;
}


static void TASK_MatrixScan(void *pvParameters)
{
  while( 1 ) {
    // wait for next timesplice (1 mS)
    vTaskDelay(1 / portTICK_RATE_MS);

    // determine timestamp (we need it for delay measurements)
    mios32_sys_time_t t = MIOS32_SYS_TimeGet();
    u32 timestamp = 1000*t.seconds + t.fraction_ms;

    // select first row
    u16 select_row_pattern = ~(1 << 0);
#if MATRIX_DOUT_HAS_SINK_DRIVERS
    select_row_pattern ^= 0xffff; // invert selection pattern if sink drivers are connected to DOUT pins
#endif

    MIOS32_SPI_TransferByte(MIOS32_SRIO_SPI, (select_row_pattern >> 8) & 0xff);
    MIOS32_SPI_TransferByte(MIOS32_SRIO_SPI, (select_row_pattern >> 0) & 0xff);

    // now read the two DIN registers
    // while doing this, write the selection pattern for the next row to DOUT registers
    int row;
    for(row=0; row<MATRIX_NUM_ROWS; ++row) {
      // latch DIN values
      MIOS32_SPI_RC_PinSet(MIOS32_SRIO_SPI, MIOS32_SRIO_SPI_RC_PIN, 0); // spi, rc_pin, pin_value
      MIOS32_DELAY_Wait_uS(1);
      MIOS32_SPI_RC_PinSet(MIOS32_SRIO_SPI, MIOS32_SRIO_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

      // determine selection mask for next row (written into DOUT registers while reading DIN registers)
      u16 select_row_pattern = ~(1 << (row+1));
#if MATRIX_DOUT_HAS_SINK_DRIVERS
      select_row_pattern ^= 0xffff; // invert selection pattern if sink drivers are connected to DOUT pins
#endif

      // read DIN, write DOUT
      u8 din0 = MIOS32_SPI_TransferByte(MIOS32_SRIO_SPI, (select_row_pattern >> 8) & 0xff);
      u8 din1 = MIOS32_SPI_TransferByte(MIOS32_SRIO_SPI, (select_row_pattern >> 0) & 0xff);

      // combine to 16bit value
      u16 din_pattern = (din1 << 8) | din0;

      // check if values have been changed via XOR combination with previously scanned value
      u16 changed = din_pattern ^ din_value[row];
      if( changed ) {
	// store changed value
	din_value[row] = din_pattern;

	// notify changed value
	int column;
	for(column=0; column<16; ++column) {
	  u16 mask = 1 << column;
	  if( changed & mask )
	    BUTTON_NotifyToggle(row, column, (din_pattern & mask) ? 1 : 0, timestamp);
	}
      }
    }
  }
}
