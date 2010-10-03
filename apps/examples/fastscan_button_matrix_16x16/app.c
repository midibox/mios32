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
void BUTTON_NotifyToggle(u8 row, u8 column, u8 pin_value)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[BUTTON_NotifyToggle] row=0x%02x, column=0x%02x, pin_value=%d\n", row, column, pin_value);
#endif
}

static void TASK_MatrixScan(void *pvParameters)
{
  while( 1 ) {
    // wait for next timesplice (1 mS)
    vTaskDelay(1 / portTICK_RATE_MS);

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
      s32 status = 0;
      u8 din0 = MIOS32_SPI_TransferByte(MIOS32_SRIO_SPI, (select_row_pattern >> 8) & 0xff);
      u8 din1 = MIOS32_SPI_TransferByte(MIOS32_SRIO_SPI, (select_row_pattern >> 0) & 0xff);

      // combine to 16bit value
      u16 din_pattern = (din1 << 8) | din0;

      // check if values has been changed via XOR combination with previously scanned value
      u16 changed = din_pattern ^ din_value[row];
      if( changed ) {
	// store changed value
	din_value[row] = din_pattern;

	// notify changed value
	int column;
	for(column=0; column<16; ++column) {
	  u16 mask = 1 << column;
	  if( changed & mask )
	    BUTTON_NotifyToggle(row, column, (din_pattern & mask) ? 1 : 0);
	}
      }
    }
  }
}
