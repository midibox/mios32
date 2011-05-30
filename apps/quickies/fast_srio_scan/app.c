// $Id$
/*
 * SRIO Fast Scan (10 uS)
 *
 * ==========================================================================
 *
 *  Copyright (C) <year> <your name> (<your email address>)
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

// include everything FreeRTOS related we don't understand yet ;)
#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>


// define priority level for DIN task:
// use lower priority as MIOS32 specific tasks (3)
// so that the task can run endless
#define PRIORITY_TASK_DIN_HANDLER   ( tskIDLE_PRIORITY + 1 )

// local prototype of the task function
static void TASK_DIN_Handler(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static void SRIO_Handler(void);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static u8 muxctr;		//JM counter for mux pointer

static volatile u8 srio_dout[4][MIOS32_SRIO_NUM_SR];	//jm add 2d to dout array
static volatile u8 srio_din[4][MIOS32_SRIO_NUM_SR];
static volatile u8 srio_din_buffer[4][MIOS32_SRIO_NUM_SR];
static volatile u8 srio_din_changed[4][MIOS32_SRIO_NUM_SR];



/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize some pins of J5A/J5B/J5C as outputs in Push-Pull Mode - JM
  int pin;
  for(pin=0; pin<9; ++pin)
    MIOS32_BOARD_J5_PinInit(pin, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
  
  // Pin 11 as input with pull-up enabled
  MIOS32_BOARD_J5_PinInit(11, MIOS32_BOARD_PIN_MODE_INPUT_PU);  

  // clear chains
  // will be done again in MIOS32_DIN_Init and MIOS32_DOUT_Init
  // we don't reference to these functions here to allow the programmer to remove/replace these driver modules)
  // MIOS32_BOARD_J5_PinSet(3, 1);	//jm
  int i, j;
  for(j=0; j<4; ++j) {
    for(i=0; i<MIOS32_SRIO_NUM_SR; ++i) {
      srio_dout[j][i] = 0x00; 	// passive state (LEDs off) JM todo:add all dim levels?
      srio_din[j][i] = 0xff;        	// passive state (Buttons depressed) JM
      srio_din_buffer[j][i] = 0xff; 	// passive state (Buttons depressed) JM
      srio_din_changed[j][i] = 0;   	// no change JM
    }
  }

  // initial state of RCLK
  MIOS32_SPI_RC_PinSet(MIOS32_SRIO_SPI, MIOS32_SRIO_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

  // init GPIO structure
  // use Push-Pull with strong (!) drivers
  MIOS32_SPI_IO_Init(MIOS32_SRIO_SPI, MIOS32_SPI_PIN_DRIVER_STRONG);

  // init SPI port for baudrate of ca. 2 uS period @ 72 MHz
  MIOS32_SPI_TransferModeInit(MIOS32_SRIO_SPI, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_4); //org=128 JM

  // start first DMA transfer (takes 5us)
  MIOS32_SPI_TransferBlock(MIOS32_SRIO_SPI,
			   (u8 *)&srio_dout[muxctr][0], (u8 *)&srio_din_buffer[muxctr][0],  //jm todo ??
			   MIOS32_SRIO_NUM_SR,
			   SRIO_Handler);

  // start task
  xTaskCreate(TASK_DIN_Handler, (signed portCHAR *)"DIN_Handler", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_DIN_HANDLER, NULL);

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
// This DMA callback is called periodically
/////////////////////////////////////////////////////////////////////////////
static void SRIO_Handler(void)
{
  // TK: using direct port accesses instead of the comfortable MIOS32_BOARD_J5_PinSet
  // functions to speed up the timing critical handler

  // Ports are located at:
  // J5A[3:0] -> GPIOC[3:0]
  // J5B[3:0] -> GPIOA[3:0]
  // J5C[1:0] -> GPIOC[5:4]
  // J5C[3:2] -> GPIOB[1:0]

  // BSRR[15:0] sets output register to 1, BSRR[31:16] to 0
  // BRR[15:0] sets output register to 0 as well (no shifting required, but multiple accesses required if pins shout be set/cleared)

  //MIOS32_BOARD_J5_PinSet(muxctr , 0);		//jm organ muxpin off 
  //MIOS32_BOARD_J5_PinSet(8 , 0);		//jm Dout disabled
  GPIOC->BSRR = (1 << (muxctr+16)) | (1 << 4); // clear muxctr pin, set GPIOC[4] = J5C.A8

  //MIOS32_BOARD_J5_PinSet(muxctr + 4 , 1);	//jm keyboard mux pin on
  GPIOA->BSRR = 1 << muxctr;


  //latch DOUT registers by pulsing RCLK: 1->0->1
  MIOS32_SPI_RC_PinSet(MIOS32_SRIO_SPI, MIOS32_SRIO_SPI_RC_PIN, 0); // spi, rc_pin, pin_value
  //MIOS32_DELAY_Wait_uS(1); //(not needed JM)
  MIOS32_SPI_RC_PinSet(MIOS32_SRIO_SPI, MIOS32_SRIO_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

  //MIOS32_BOARD_J5_PinSet(muxctr + 4 , 0);	//jm keyboard mux pin off
  GPIOA->BRR = 1 << muxctr;

  //JM Mux counter 
  // TK: remember the previous one for DIN buffer copies
  u8 prev_muxctr = muxctr;
  if ( ++muxctr >= 4 ) {
   	muxctr = 0; 
  }  

  //mux pin on     
  //MIOS32_BOARD_J5_PinSet(muxctr , 1);		//jm organ mux pin on
  //MIOS32_BOARD_J5_PinSet(8 , 1);		//jm Dout enabled
  GPIOC->BSRR = 1 << muxctr | (1 << 4);


  // TK: not optimized as this seems to be a test option which will be removed later?
  // disable it to measure best-case timing
#if 0
  if (MIOS32_BOARD_J5_PinGet(11) == 0) {	//test for mode 1
      MIOS32_BOARD_J5_PinSet(muxctr + 4 , 1);	//jm set keyboard mux pin on mode 1
  }
#endif

  // start next SRIO Scan
  // we use J5.A3 for debugging
  MIOS32_SPI_TransferBlock(MIOS32_SRIO_SPI,
			   (u8 *)&srio_dout[muxctr][0], (u8 *)&srio_din_buffer[muxctr][0],  //jm todo ??
			   MIOS32_SRIO_NUM_SR,
			   SRIO_Handler);

  // copy/or buffered DIN values/changed flags of previous scan
  int i;
  for(i=0; i<MIOS32_SRIO_NUM_SR; ++i) {
    srio_din_changed[prev_muxctr][i] |= srio_din[prev_muxctr][i] ^ srio_din_buffer[prev_muxctr][i];
    srio_din[prev_muxctr][i] = srio_din_buffer[prev_muxctr][i];
  }
}


/////////////////////////////////////////////////////////////////////////////
// This low-prio task is running endless to check for DIN changes
/////////////////////////////////////////////////////////////////////////////
static void TASK_DIN_Handler(void *pvParameters)
{
  s32 mux;
  s32 sr;
  s32 sr_pin;
  u8 changed;

  // endless loop
  while( 1 ) {
    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    // check all shift registers for DIN pin changes
    for(mux=0; mux<4; ++mux) {
      for(sr=0; sr<MIOS32_SRIO_NUM_SR; ++sr) {
    
	// check if there are pin changes (mask all pins)
	// must be atomic!
	MIOS32_IRQ_Disable();
	changed = srio_din_changed[mux][sr];
	srio_din_changed[mux][sr] = 0;
	MIOS32_IRQ_Enable();

	// any pin change at this SR?
	if( !changed )
	  continue;

	// check all 8 pins of the SR
	for(sr_pin=0; sr_pin<8; ++sr_pin)
	  if( changed & (1 << sr_pin) ) {
	    // call the notification function
	    u32 pin = MIOS32_SRIO_NUM_SR*8*mux + 8*sr + sr_pin;
	    u32 pin_value = (srio_din[mux][sr] & (1 << sr_pin)) ? 1 : 0;
	    APP_DIN_NotifyToggle(pin, pin_value);
	  }
      }
    }
  }
}

