// $Id$
/*
 * Check routines for MBHP_CORE_STM32 board
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
#define DEBUG_VERBOSE_LEVEL 2
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// Settle Delay in uS
// Especially required when pull devices are switched
/////////////////////////////////////////////////////////////////////////////
#define IO_SETTLE_DELAY 10


/////////////////////////////////////////////////////////////////////////////
// Should JTAG pins checked as well?
/////////////////////////////////////////////////////////////////////////////
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
  u16           expected_fails;
} io_pin_t;

static const io_pin_t io_pin_table[] = {
  { "PC13", GPIOC, GPIO_Pin_13,  (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PC14", GPIOC, GPIO_Pin_14,  (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PC15", GPIOC, GPIO_Pin_15,  0x00 },
  { "PC0",  GPIOC, GPIO_Pin_0,   0x00 },
  { "PC1",  GPIOC, GPIO_Pin_1,   0x00 },
  { "PC2",  GPIOC, GPIO_Pin_2,   0x00 },
  { "PC3",  GPIOC, GPIO_Pin_3,   0x00 },
  { "PA0",  GPIOA, GPIO_Pin_0,   0x00 },
  { "PA1",  GPIOA, GPIO_Pin_1,   0x00 },
  { "PA2",  GPIOA, GPIO_Pin_2,   0x00 },
  { "PA3",  GPIOA, GPIO_Pin_3,   0x00 },
  { "PA4",  GPIOA, GPIO_Pin_4,   (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PA5",  GPIOA, GPIO_Pin_5,   (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PA6",  GPIOA, GPIO_Pin_6,   0x00 },
  { "PA7",  GPIOA, GPIO_Pin_7,   (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PC4",  GPIOC, GPIO_Pin_4,   0x00 },
  { "PC5",  GPIOC, GPIO_Pin_5,   0x00 },
  { "PB0",  GPIOB, GPIO_Pin_0,   0x00 },
  { "PB1",  GPIOB, GPIO_Pin_1,   0x00 },
  { "PB2",  GPIOB, GPIO_Pin_2,   (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PB10", GPIOB, GPIO_Pin_10,  (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PB11", GPIOB, GPIO_Pin_11,  (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PB12", GPIOB, GPIO_Pin_12,  (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PB13", GPIOB, GPIO_Pin_13,  (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PB14", GPIOB, GPIO_Pin_14,  0x00 },
  { "PB15", GPIOB, GPIO_Pin_15,  (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PC6",  GPIOC, GPIO_Pin_6,   (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PC7",  GPIOC, GPIO_Pin_7,   (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PC8",  GPIOC, GPIO_Pin_8,   (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PC9",  GPIOC, GPIO_Pin_9,   0x00 },
  { "PA8",  GPIOA, GPIO_Pin_8,   0x00 },
  { "PA9",  GPIOA, GPIO_Pin_9,   0x00 },
  { "PA10", GPIOA, GPIO_Pin_10,  (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
#if 0 // USB
  { "PA11", GPIOA, GPIO_Pin_11,  0x00 },
  { "PA12", GPIOA, GPIO_Pin_12,  0x00 },
#endif
#if CHECK_JTAG_PINS
  { "PA13", GPIOA, GPIO_Pin_13,  0x00 },
  { "PA14", GPIOA, GPIO_Pin_14,  0x00 },
  { "PA15", GPIOA, GPIO_Pin_15,  0x00 },
#endif
  { "PC10", GPIOC, GPIO_Pin_10,  0x00 },
  { "PC11", GPIOC, GPIO_Pin_11,  (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PD2",  GPIOD, GPIO_Pin_2,   0x00 },
#if CHECK_JTAG_PINS
  { "PB3",  GPIOB, GPIO_Pin_3,   0x00 },
  { "PB4",  GPIOB, GPIO_Pin_4,   0x00 },
#endif
  { "PB5",  GPIOB, GPIO_Pin_5,   (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PB6",  GPIOB, GPIO_Pin_6,   (1 << EXPECTED_FAIL_IPL) }, // On-Board Pull-Up connected
  { "PB7",  GPIOB, GPIO_Pin_7,   0x00 },
  { "PB8",  GPIOB, GPIO_Pin_8,   (1 << EXPECTED_FAIL_IPL) }, // On-Board Diode and Pull-Up connected
  { "PB9",  GPIOB, GPIO_Pin_9,   (1 << EXPECTED_FAIL_IPL) }, // On-Board Diode connected
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 CHECK_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// IO test
/////////////////////////////////////////////////////////////////////////////
s32 CHECK_Pins(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);

  s32 num_errors = 0;
  s32 subtest_errors = 0;

  int pin;
  int num_pins = sizeof(io_pin_table)/sizeof(io_pin_t);

#if CHECK_JTAG_PINS
  // ensure that JTAG pins are disabled
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE); // "ENABLE" enables the disable...
#endif

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("\n");
  DEBUG_MSG("====================================\n");
  DEBUG_MSG("Testing GPIO Pins for shorts/bridges\n");
  DEBUG_MSG("====================================\n");
#endif

  /////////////////////////////////////////////////////////////////////////////
  // Set all pins to input with pull-up enabled
  /////////////////////////////////////////////////////////////////////////////
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("Set all pins to input with pull-up enabled\n");
#endif

  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  for(subtest_errors=0, pin=0; pin<num_pins; ++pin) {
    io_pin_t *io_pin = (io_pin_t *)&io_pin_table[pin];
    GPIO_InitStructure.GPIO_Pin = io_pin->GPIO_Pin;
    GPIO_Init(io_pin->GPIOx, &GPIO_InitStructure);
  }


  /////////////////////////////////////////////////////////////////////////////
  // Push-Pull Toggle Test
  /////////////////////////////////////////////////////////////////////////////
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("Starting Push-Pull Toggle Test\n");
#endif

  for(subtest_errors=0, pin=0; pin<num_pins; ++pin) {
    io_pin_t *io_pin = (io_pin_t *)&io_pin_table[pin];
    s32 error_found = 0;

    GPIO_ResetBits(io_pin->GPIOx, io_pin->GPIO_Pin);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = io_pin->GPIO_Pin;
    GPIO_Init(io_pin->GPIOx, &GPIO_InitStructure);

    // toggle 0->1
    GPIO_SetBits(io_pin->GPIOx, io_pin->GPIO_Pin);
    MIOS32_DELAY_Wait_uS(IO_SETTLE_DELAY);
    error_found = GPIO_ReadInputDataBit(io_pin->GPIOx, io_pin->GPIO_Pin) != Bit_SET;
#if DEBUG_VERBOSE_LEVEL >= 1
    if( error_found || DEBUG_VERBOSE_LEVEL >= 2 )
      DEBUG_MSG("Toggle %s 0->1 %s\n", io_pin->name, error_found ? "FAILED" : "OK");
#endif
    subtest_errors += error_found;


    // toggle 1->0
    GPIO_ResetBits(io_pin->GPIOx, io_pin->GPIO_Pin);
    MIOS32_DELAY_Wait_uS(IO_SETTLE_DELAY);
    error_found = GPIO_ReadInputDataBit(io_pin->GPIOx, io_pin->GPIO_Pin) != Bit_RESET;
#if DEBUG_VERBOSE_LEVEL >= 1
    if( error_found || DEBUG_VERBOSE_LEVEL >= 2 )
      DEBUG_MSG("Toggle %s 1->0 %s\n", io_pin->name, error_found ? "FAILED" : "OK");
#endif
    subtest_errors += error_found;

    // release pin by activating input mode
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = io_pin->GPIO_Pin;
    GPIO_Init(io_pin->GPIOx, &GPIO_InitStructure);
  }

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("Push-Pull Toggle Test found %d errors\n", subtest_errors);
#endif
  num_errors += subtest_errors;


  /////////////////////////////////////////////////////////////////////////////
  // Pull-Down/Up Toggle Test
  /////////////////////////////////////////////////////////////////////////////
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("Starting Pull-Down/Up Toggle Test\n");
#endif

  for(subtest_errors=0, pin=0; pin<num_pins; ++pin) {
    io_pin_t *io_pin = (io_pin_t *)&io_pin_table[pin];
    s32 error_found = 0;

    // toggle H->L
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_InitStructure.GPIO_Pin = io_pin->GPIO_Pin;
    GPIO_Init(io_pin->GPIOx, &GPIO_InitStructure);
    MIOS32_DELAY_Wait_uS(IO_SETTLE_DELAY);
    error_found = GPIO_ReadInputDataBit(io_pin->GPIOx, io_pin->GPIO_Pin) != Bit_RESET;
    if( error_found && (io_pin->expected_fails & (1 << EXPECTED_FAIL_IPL)) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("Toggle %s H->L not working due to external Pull-Up Resistor\n", io_pin->name);
#endif
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      if( error_found || DEBUG_VERBOSE_LEVEL >= 2 )
	DEBUG_MSG("Toggle %s H->L %s\n", io_pin->name, error_found ? "FAILED" : "OK");
#endif
      subtest_errors += error_found;
    }


    // toggle L->H
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = io_pin->GPIO_Pin;
    GPIO_Init(io_pin->GPIOx, &GPIO_InitStructure);
    MIOS32_DELAY_Wait_uS(IO_SETTLE_DELAY);
    error_found = GPIO_ReadInputDataBit(io_pin->GPIOx, io_pin->GPIO_Pin) != Bit_SET;
    if( error_found && (io_pin->expected_fails & (1 << EXPECTED_FAIL_IPU)) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("Toggle %s L->H not working due to external Pull-Down Resistor\n", io_pin->name);
#endif
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      if( error_found || DEBUG_VERBOSE_LEVEL >= 2 )
	DEBUG_MSG("Toggle %s L->H %s\n", io_pin->name, error_found ? "FAILED" : "OK");
#endif
      subtest_errors += error_found;
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("Pull-Down/Up Toggle Test found %d errors\n", subtest_errors);
#endif
  num_errors += subtest_errors;


  /////////////////////////////////////////////////////////////////////////////
  // Set all pins to output 0
  /////////////////////////////////////////////////////////////////////////////
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("Set all pins to output 0\n");
#endif

  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  for(subtest_errors=0, pin=0; pin<num_pins; ++pin) {
    io_pin_t *io_pin = (io_pin_t *)&io_pin_table[pin];
    GPIO_InitStructure.GPIO_Pin = io_pin->GPIO_Pin;
    GPIO_Init(io_pin->GPIOx, &GPIO_InitStructure);
    GPIO_ResetBits(io_pin->GPIOx, io_pin->GPIO_Pin);
  }


  /////////////////////////////////////////////////////////////////////////////
  // Running-1 test
  /////////////////////////////////////////////////////////////////////////////
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("Starting Running-1 Test\n");
#endif

  for(subtest_errors=0, pin=0; pin<num_pins; ++pin) {
    io_pin_t *io_pin = (io_pin_t *)&io_pin_table[pin];
    s32 error_found = 0;

    // toggle 0->1
    GPIO_SetBits(io_pin->GPIOx, io_pin->GPIO_Pin);
    MIOS32_DELAY_Wait_uS(IO_SETTLE_DELAY);
    error_found = GPIO_ReadInputDataBit(io_pin->GPIOx, io_pin->GPIO_Pin) != Bit_SET;
    if( error_found && (io_pin->expected_fails & (1 << EXPECTED_FAIL_P1)) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("Toggle %s 0->1 not working due to external Diode\n", io_pin->name);
#endif
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      if( error_found || DEBUG_VERBOSE_LEVEL >= 2 )
	DEBUG_MSG("Toggle %s 0->1 %s\n", io_pin->name, error_found ? "FAILED" : "OK");
#endif
      subtest_errors += error_found;
    }


    // toggle 1->0
    GPIO_ResetBits(io_pin->GPIOx, io_pin->GPIO_Pin);
    MIOS32_DELAY_Wait_uS(IO_SETTLE_DELAY);
    error_found = GPIO_ReadInputDataBit(io_pin->GPIOx, io_pin->GPIO_Pin) != Bit_RESET;
    if( error_found && (io_pin->expected_fails & (1 << EXPECTED_FAIL_P0)) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("Toggle %s 1->0 not working due to external Diode\n", io_pin->name);
#endif
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      if( error_found || DEBUG_VERBOSE_LEVEL >= 2 )
	DEBUG_MSG("Toggle %s 1->0 %s\n", io_pin->name, error_found ? "FAILED" : "OK");
#endif
      subtest_errors += error_found;
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("Running-1 Test found %d errors\n", subtest_errors);
#endif
  num_errors += subtest_errors;


  /////////////////////////////////////////////////////////////////////////////
  // Set all pins to output 1
  /////////////////////////////////////////////////////////////////////////////
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("Set all pins to output 1\n");
#endif

  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  for(subtest_errors=0, pin=0; pin<num_pins; ++pin) {
    io_pin_t *io_pin = (io_pin_t *)&io_pin_table[pin];
    GPIO_InitStructure.GPIO_Pin = io_pin->GPIO_Pin;
    GPIO_Init(io_pin->GPIOx, &GPIO_InitStructure);
    GPIO_SetBits(io_pin->GPIOx, io_pin->GPIO_Pin);
  }


  /////////////////////////////////////////////////////////////////////////////
  // Running-0 test
  /////////////////////////////////////////////////////////////////////////////
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("Starting Running-0 Test\n");
#endif

  for(subtest_errors=0, pin=0; pin<num_pins; ++pin) {
    io_pin_t *io_pin = (io_pin_t *)&io_pin_table[pin];
    s32 error_found = 0;

    // toggle 1->0
    GPIO_ResetBits(io_pin->GPIOx, io_pin->GPIO_Pin);
    MIOS32_DELAY_Wait_uS(IO_SETTLE_DELAY);
    error_found = GPIO_ReadInputDataBit(io_pin->GPIOx, io_pin->GPIO_Pin) != Bit_RESET;
    if( error_found && (io_pin->expected_fails & (1 << EXPECTED_FAIL_P0)) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("Toggle %s 1->0 not working due to external Diode\n", io_pin->name);
#endif
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      if( error_found || DEBUG_VERBOSE_LEVEL >= 2 )
	DEBUG_MSG("Toggle %s 1->0 %s\n", io_pin->name, error_found ? "FAILED" : "OK");
#endif
      subtest_errors += error_found;
    }


    // toggle 0->1
    GPIO_SetBits(io_pin->GPIOx, io_pin->GPIO_Pin);
    MIOS32_DELAY_Wait_uS(IO_SETTLE_DELAY);
    error_found = GPIO_ReadInputDataBit(io_pin->GPIOx, io_pin->GPIO_Pin) != Bit_SET;
    if( error_found && (io_pin->expected_fails & (1 << EXPECTED_FAIL_P1)) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("Toggle %s 0->1 not working due to external Diode\n", io_pin->name);
#endif
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      if( error_found || DEBUG_VERBOSE_LEVEL >= 2 )
	DEBUG_MSG("Toggle %s 0->1 %s\n", io_pin->name, error_found ? "FAILED" : "OK");
#endif
      subtest_errors += error_found;
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("Running-0 Test found %d errors\n", subtest_errors);
#endif
  num_errors += subtest_errors;


  /////////////////////////////////////////////////////////////////////////////
  // Set all pins to input with pull-up enabled
  /////////////////////////////////////////////////////////////////////////////
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("Set all pins to input with pull-up enabled\n");
#endif

  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  for(subtest_errors=0, pin=0; pin<num_pins; ++pin) {
    io_pin_t *io_pin = (io_pin_t *)&io_pin_table[pin];
    GPIO_InitStructure.GPIO_Pin = io_pin->GPIO_Pin;
    GPIO_Init(io_pin->GPIOx, &GPIO_InitStructure);
  }


  /////////////////////////////////////////////////////////////////////////////
  // Final Result
  /////////////////////////////////////////////////////////////////////////////
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("\n");
  DEBUG_MSG("====================================\n");
  DEBUG_MSG("Test found %d errors\n", num_errors);
#endif

  return num_errors ? -1 : 0;
}
