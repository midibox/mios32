// $Id$
/*
 * SPI J16 Master
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
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
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>


// define priority level for SPI Handler task:
// use same priority as MIOS32 specific tasks (3)
#define PRIORITY_TASK_SPI_HANDLER	( tskIDLE_PRIORITY + 3 )

// local prototype of the task function
static void TASK_SPI_Handler(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Rx/Tx buffer
/////////////////////////////////////////////////////////////////////////////
#define MASTER_SPI     0 // @J16
#define MASTER_CS_PIN  0 // =RC1
#define TRANSFER_BUFFER_SIZE 16
u8 rx_buffer[TRANSFER_BUFFER_SIZE];
u8 tx_buffer[TRANSFER_BUFFER_SIZE];


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);


  // initialize SPI interface
  // ensure that fast pin drivers are activated
  MIOS32_SPI_IO_Init(MASTER_SPI, MIOS32_SPI_PIN_DRIVER_STRONG);

  // init SPI port
  MIOS32_SPI_TransferModeInit(MASTER_SPI, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_128);

  // ensure that chip select line deactivated
  MIOS32_SPI_RC_PinSet(MASTER_SPI, MASTER_CS_PIN, 1); // spi, rc_pin, pin_value

  // start task
  xTaskCreate(TASK_SPI_Handler, "SPI_Handler", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_SPI_HANDLER, NULL);
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
// This task periodically sends a SPI datastream
/////////////////////////////////////////////////////////////////////////////
static void SPI_Callback(void)
{
  // called after SPI transfer:

  // deactivate CS pin
  MIOS32_SPI_RC_PinSet(MASTER_SPI, MASTER_CS_PIN, 1); // spi, rc_pin, pin_value

  // change TX values
  int i;
  for(i=0; i<TRANSFER_BUFFER_SIZE; ++i)
    tx_buffer[i] += 0x10;
}

static void TASK_SPI_Handler(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  // fill Tx Buffer with housenumbers
  {
    int i;
    for(i=0; i<TRANSFER_BUFFER_SIZE; ++i)
      tx_buffer[i] = i;
  }

  while( 1 ) {
    // wait for 100 mS
    vTaskDelayUntil(&xLastExecutionTime, 100 / portTICK_RATE_MS);

    // toggle Status LED to as a sign of live
    MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

    // activate CS pin
    MIOS32_SPI_RC_PinSet(MASTER_SPI, MASTER_CS_PIN, 0); // spi, rc_pin, pin_value

    // send data non-blocking
    MIOS32_SPI_TransferBlock(MASTER_SPI, tx_buffer, rx_buffer, TRANSFER_BUFFER_SIZE, SPI_Callback);
  }
}
