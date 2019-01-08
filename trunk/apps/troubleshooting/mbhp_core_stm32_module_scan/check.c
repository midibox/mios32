// $Id$
/*
 * Scan routines for MBHP_CORE_STM32 board
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


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIDI
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// DUT settings
/////////////////////////////////////////////////////////////////////////////
#define NUM_CYCLES      1000 // mS if clocked @72 MHz
#define DELAY_NEXT_PIN  10 // mS if clocked @72 MHz


/////////////////////////////////////////////////////////////////////////////
// Tester settings (should be at least NUM_CYCLES)
/////////////////////////////////////////////////////////////////////////////
#define TIMEOUT_CYCLES  5000 // mS


/////////////////////////////////////////////////////////////////////////////
// Should USB/JTAG pins checked as well?
/////////////////////////////////////////////////////////////////////////////
#define CHECK_USB_PINS 0
#define CHECK_JTAG_PINS 0

/////////////////////////////////////////////////////////////////////////////
// This table defines the neighboured pins of a STM32F103RE chip (64 pin package)
/////////////////////////////////////////////////////////////////////////////

// flags which identify expected fails (e.g. if a pull-up/down resistor is connected on board)
// such fails will be tolerated, but also won't lead to an error if the test passes, e.g.
// because the pull device isn't stuffed yet.
#define EXPECTED_FAIL_P0   0
#define EXPECTED_FAIL_P1   1
#define EXPECTED_FAIL_IPL  2
#define EXPECTED_FAIL_IPU  3

typedef struct {
  char          name[10];
  GPIO_TypeDef *GPIOx;
  u16           GPIO_Pin;
} io_pin_t;

static const io_pin_t io_pin_table[] = {
  // J5A
  { "PC0",  GPIOC, GPIO_Pin_0 }, // Important: this should be the first pin, it's used to sync the running-one sequence
  { "PC1",  GPIOC, GPIO_Pin_1 },
  { "PC2",  GPIOC, GPIO_Pin_2 },
  { "PC3",  GPIOC, GPIO_Pin_3 },

  // J5B
  { "PA0",  GPIOA, GPIO_Pin_0 },
  { "PA1",  GPIOA, GPIO_Pin_1 },
  { "PA2",  GPIOA, GPIO_Pin_2 },
  { "PA3",  GPIOA, GPIO_Pin_3 },

  // J16 (SRIO)
  { "PA4",  GPIOA, GPIO_Pin_4 },
  { "PA5",  GPIOA, GPIO_Pin_5 },
  { "PA6",  GPIOA, GPIO_Pin_6 },
  { "PA7",  GPIOA, GPIO_Pin_7 },
  { "PC15", GPIOC, GPIO_Pin_15 },

  // J5C  
  { "PC4",  GPIOC, GPIO_Pin_4 },
  { "PC5",  GPIOC, GPIO_Pin_5 },
  { "PB0",  GPIOB, GPIO_Pin_0 },
  { "PB1",  GPIOB, GPIO_Pin_1 },

  // J27 (Boot1 Pin)
  { "PB2",  GPIOB, GPIO_Pin_2 },

  // J4 (IIC)
  { "PB10", GPIOB, GPIO_Pin_10 },
  { "PB11", GPIOB, GPIO_Pin_11 },


  // J8/9 (SRIO)
  { "PB12", GPIOB, GPIO_Pin_12 },
  { "PB13", GPIOB, GPIO_Pin_13 },
  { "PB14", GPIOB, GPIO_Pin_14 },
  { "PB15", GPIOB, GPIO_Pin_15 },

  // LCD pins
  { "PC6",  GPIOC, GPIO_Pin_6 },
  { "PC7",  GPIOC, GPIO_Pin_7 },
  { "PC8",  GPIOC, GPIO_Pin_8 },
  { "PC9",  GPIOC, GPIO_Pin_9 },
  { "PA8",  GPIOA, GPIO_Pin_8 },
  { "PC12", GPIOC, GPIO_Pin_12 }, // Note: connected to 74HC595 Output - will cause failure if chip in socket

  // MIDI IO 1
  { "PA9",  GPIOA, GPIO_Pin_9 },
  { "PA10", GPIOA, GPIO_Pin_10 },

#if CHECK_USB_PINS
  // USB
  { "PA11", GPIOA, GPIO_Pin_11 },
  { "PA12", GPIOA, GPIO_Pin_12 },
#endif

#if CHECK_JTAG_PINS
  // JTAG
  { "PA13", GPIOA, GPIO_Pin_13 },
  { "PA14", GPIOA, GPIO_Pin_14 },
  { "PA15", GPIOA, GPIO_Pin_15 },
  { "PB3",  GPIOB, GPIO_Pin_3 },
  { "PB4",  GPIOB, GPIO_Pin_4 },
#endif

  // MIDI IO 2
  { "PC10", GPIOC, GPIO_Pin_10 },
  { "PC11", GPIOC, GPIO_Pin_11 },

  // J19
  { "PB5",  GPIOB, GPIO_Pin_5 },
  { "PB6",  GPIOB, GPIO_Pin_6 },
  { "PB7",  GPIOB, GPIO_Pin_7 },
  { "PC13", GPIOC, GPIO_Pin_13 },
  { "PC14", GPIOC, GPIO_Pin_14 },

#if 0
  // Status LED
  // never tested, as it shows the test status
  { "PD2",  GPIOD, GPIO_Pin_2,   0x00 },
#endif

  // J18 (CAN)
  { "PB8",  GPIOB, GPIO_Pin_8 },
  { "PB9",  GPIOB, GPIO_Pin_9 },
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 CHECK_Init(u32 mode)
{
  int pin;
  int num_pins = sizeof(io_pin_table)/sizeof(io_pin_t);

  if( mode >= 1 )
    return -1; // unsupported mode

  // clear status LED
  MIOS32_BOARD_LED_Set(1, 0);

#if CHECK_JTAG_PINS
  // ensure that JTAG pins are disabled
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE); // "ENABLE" enables the disable...
#endif

  // init drivers for all pins
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
#if BUILD_FOR_DUT
  // DUT Drives pins
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
#else
  // Tester reads pins
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
#endif
  for(pin=0; pin<num_pins; ++pin) {
    io_pin_t *io_pin = (io_pin_t *)&io_pin_table[pin];
    GPIO_InitStructure.GPIO_Pin = io_pin->GPIO_Pin;
    GPIO_Init(io_pin->GPIOx, &GPIO_InitStructure);
    GPIO_ResetBits(io_pin->GPIOx, io_pin->GPIO_Pin);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Drives outputs of the "Device Under Test"
// This function has to be called periodically (e.g. each mS)
/////////////////////////////////////////////////////////////////////////////
s32 CHECK_Tick(void)
{

/////////////////////////////////////////////////////////////////////////////
// DUT Code
#if BUILD_FOR_DUT
/////////////////////////////////////////////////////////////////////////////
  static u32 cycle_ctr = 0;
  int pin;
  int num_pins = sizeof(io_pin_table)/sizeof(io_pin_t);


  /////////////////////////////////////////////////////////////////////////////
  // Running 1000 cycles (-> 1 second)
  /////////////////////////////////////////////////////////////////////////////
  if( cycle_ctr++ >= NUM_CYCLES )
    cycle_ctr = 0;
  

  /////////////////////////////////////////////////////////////////////////////
  // Each 10 cycles (-> 10 mS), set another pin to 1, set all other pins to 0
  // ("running one test")
  /////////////////////////////////////////////////////////////////////////////
  if( (cycle_ctr % DELAY_NEXT_PIN) == 0 ) {
    int current_pin = cycle_ctr / DELAY_NEXT_PIN;

    // set/clear board LED (lits while pins are toggling)
    MIOS32_BOARD_LED_Set(1, (current_pin < num_pins) ? 1 : 0);

    // set/clear IO pins
    for(pin=0; pin<num_pins; ++pin) {
      io_pin_t *io_pin = (io_pin_t *)&io_pin_table[pin];

      if( pin == current_pin )
	GPIO_SetBits(io_pin->GPIOx, io_pin->GPIO_Pin);
      else
	GPIO_ResetBits(io_pin->GPIOx, io_pin->GPIO_Pin);
    }
  }

  return 0; // no error


/////////////////////////////////////////////////////////////////////////////
// Tester Code
#else /* BUILD_FOR_DUT == 0 */
/////////////////////////////////////////////////////////////////////////////

  static u32 timeout_ctr = 0;
  static s32 tested_pin = -1;
  static u8 samples = 0;
  static s32 num_errors = 0;

  int pin;
  int num_pins = sizeof(io_pin_table)/sizeof(io_pin_t);

  // check for timeout
  if( timeout_ctr++ >= TIMEOUT_CYCLES ) {
    // clear status LED
    MIOS32_BOARD_LED_Set(1, 0);

#if DEBUG_VERBOSE_LEVEL >= 1
    // output message
    if( tested_pin == -1 || tested_pin == 0 )
      DEBUG_MSG("[ERROR] Sync pin hasn't toggled after %d cycles\n", timeout_ctr);
    else
      DEBUG_MSG("[ERROR] Timeout during running one sequence\n", timeout_ctr);
#endif
    // wait for first pin again
    tested_pin = -1;
    num_errors = 1;
    timeout_ctr = 0;
  }


  /////////////////////////////////////////////////////////////////////////////
  // Wait until first pin toggles
  // This allows us to synch to the running-one sequence
  /////////////////////////////////////////////////////////////////////////////
  if( tested_pin < 0 ) {
    io_pin_t *io_pin = (io_pin_t *)&io_pin_table[0];
    BitAction state = GPIO_ReadInputDataBit(io_pin->GPIOx, io_pin->GPIO_Pin);

    // wait for level 0
    if( state == Bit_RESET )
      ++samples;
    else
      samples = 0;

    // sample 3 times
    if( samples >= 3 ) {
      tested_pin = 0; // Ok, let's start to wait for 1
      samples = 0;
      num_errors = 0; // reset error counter
      timeout_ctr = 0; // reset timeout counter
    }
  } else {
    // wait until current pin is 1 for 3 cycles
    io_pin_t *io_pin = (io_pin_t *)&io_pin_table[tested_pin];
    BitAction state = GPIO_ReadInputDataBit(io_pin->GPIOx, io_pin->GPIO_Pin);

    if( state == Bit_SET )
      ++samples;
    else
      samples = 0;

    if( samples >= 3 ) {
      samples = 0;

#if DEBUG_VERBOSE_LEVEL >= 1
      // output message
      if( tested_pin == 0 )
	DEBUG_MSG("[OK] Sync pin has toggled, starting checks.\n", timeout_ctr);
#endif

      // check state of all pins
      for(pin=0; pin<num_pins; ++pin) {
	io_pin_t *io_pin = (io_pin_t *)&io_pin_table[pin];
	BitAction state = GPIO_ReadInputDataBit(io_pin->GPIOx, io_pin->GPIO_Pin);

	if( pin == tested_pin ) {
	  if( state != Bit_SET ) {
	    MIOS32_BOARD_LED_Set(1, 0); // error found - turn off LED immediately
	    ++num_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[ERROR] Pin number #%d (%s) failed (expected 1, is 0)!\n", pin, io_pin->name);
#endif
	  }
	} else {
	  if( state != Bit_RESET ) {
	    MIOS32_BOARD_LED_Set(1, 0); // error found - turn off LED immediately
	    ++num_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[ERROR] Pin number #%d (%s) failed (expected 0, is 1)!\n", pin, io_pin->name);
#endif
	  }
	}
      }

      // last pin reached?
      if( ++tested_pin >= num_pins ) {
	// output status
	if( num_errors == 0 ) {
	  MIOS32_BOARD_LED_Set(1, 1); // no errors found - turn on LED
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[OK] All pins toggled - test passed!\n", pin);
#endif
	} else {
	  MIOS32_BOARD_LED_Set(1, 0); // errors found - turn off LED
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[ERROR] Running-One sequence failed!\n", pin);
#endif
	}

	// restart test
	tested_pin = -1; // wait for sync pin again
	num_errors = 0; // reset error counter
	timeout_ctr = 0; // reset timeout counter
      } else {
	tested_pin; // check next pin
	timeout_ctr = 0; // reset timeout counter
      }
    }
  }


  return num_errors ? -num_errors : 0;
#endif /* BUILD_FOR_DUT */
}
