// $Id$
//! \defgroup MIOS32_BOARD
//!
//! Development Board specific functions for MIOS32
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_BOARD)


/////////////////////////////////////////////////////////////////////////////
// J5 pin mapping
/////////////////////////////////////////////////////////////////////////////

#if defined(MIOS32_BOARD_MBHP_CORE_STM32)

// note: adaptions also have to be done in MIOS32_BOARD_J5_(Set/Get),
// since these functions access the ports directly
typedef struct {
  GPIO_TypeDef *port;
  u16 pin_mask;
} j5_pin_t;

#define J5_NUM_PINS 12
static const j5_pin_t j5_pin[J5_NUM_PINS] = {
  // J5A
  { GPIOC, 1 << 0 },
  { GPIOC, 1 << 1 },
  { GPIOC, 1 << 2 },
  { GPIOC, 1 << 3 },

  // J5B
  { GPIOA, 1 << 0 },
  { GPIOA, 1 << 1 },
  { GPIOA, 1 << 2 },
  { GPIOA, 1 << 3 },

  // J5C
  { GPIOC, 1 << 4 },
  { GPIOC, 1 << 5 },
  { GPIOB, 1 << 0 },
  { GPIOB, 1 << 1 },
};

#else
#define J5_NUM_PINS 0
#endif


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u16 j5_enable_mask;



/////////////////////////////////////////////////////////////////////////////
//! Initializes MIOS32_BOARD driver
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  j5_enable_mask = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes LEDs of the board
//! \param[in] leds mask contains a flag for each LED which should be initialized<BR>
//! <UL>
//!   <LI>MBHP_CORE_STM32: 1 LED (flag 0: green)
//!   <LI>STM32_PRIMER: 2 LEDs (flag 0: green, flag 1: red)
//! </UL>
//! \return 0 if initialisation passed
//! \return -1 if no LEDs specified for board
//! \return -2 if one or more LEDs not available on board
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_LED_Init(u32 leds)
{
#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  // only one LED, connected to PD2
  if( leds & 1 ) {
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
  }

  if( leds & 0xfffffffe)
    return -2; // LED doesn't exist

  return 0; // no error
#elif defined(MIOS32_BOARD_STM32_PRIMER)
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  // two LEDs, connected to PB8 (green) and PB9 (red)
  if( leds & 1 ) {
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
  }
  if( leds & 2 ) {
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
  }

  if( leds & 0xfffffffc)
    return -2; // LED doesn't exist

  return 0; // no error
#else
  return -1; // no LED specified for board
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Sets one or more LEDs to the given value(s)
//! \param[in] leds mask contains a flag for each LED which should be changed
//! \param[in] value contains the value which should be set
//! \return 0 if initialisation passed
//! \return -1 if no LEDs specified for board
//! \return -2 if one or more LEDs not available on board
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_LED_Set(u32 leds, u32 value)
{
#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
  // only one LED, connected to PD2
  if( leds & 1 ) {
    GPIOD->BSRR = (value&1) ? GPIO_Pin_2 : (GPIO_Pin_2 << 16);
  }

  if( leds & 0xfffffffe)
    return -2; // LED doesn't exist

  return 0; // no error
#elif defined(MIOS32_BOARD_STM32_PRIMER)
  // two LEDs, connected to PB8 (green) and PB9 (red)
  if( leds & 1 ) {
    GPIOB->BSRR = (value&1) ? GPIO_Pin_8 : (GPIO_Pin_8 << 16);
  }
  if( leds & 2 ) {
    GPIOB->BSRR = (value&2) ? GPIO_Pin_9 : (GPIO_Pin_9 << 16);
  }

  if( leds & 0xfffffffc)
    return -2; // LED doesn't exist

  return 0; // no error
#else
  return -1; // no LED specified for board
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the status of all LEDs
//! \return status of all LEDs
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_BOARD_LED_Get(void)
{
  u32 values = 0;

#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
  // only one LED, connected to PD2
  if( GPIOD->ODR & GPIO_Pin_2 )
    values |= (1 << 0);
#elif defined(MIOS32_BOARD_STM32_PRIMER)
  // two LEDs, connected to PB8 (green) and PB9 (red)
  if( GPIOB->ODR & GPIO_Pin_8 )
    values |= (1 << 0);
  if( GPIOB->ODR & GPIO_Pin_9 )
    values |= (1 << 1);
#endif

  return values;
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes a J5 pin
//! \param[in] pin the pin number (0..11)
//! \param[in] mode the pin mode
//!   <UL>
//!     <LI>MIOS32_BOARD_PIN_MODE_IGNORE: configuration shouldn't be touched (e.g. if used as analog input)
//!     <LI>MIOS32_BOARD_PIN_MODE_INPUT: pin is used as input w/o pull device (floating)
//!     <LI>MIOS32_BOARD_PIN_MODE_INPUT_PD: pin is used as input, internal pull down enabled
//!     <LI>MIOS32_BOARD_PIN_MODE_INPUT_PU: pin is used as input, internal pull up enabled
//!     <LI>MIOS32_BOARD_PIN_MODE_OUTPUT_PP: pin is used as output in push-pull mode
//!     <LI>MIOS32_BOARD_PIN_MODE_OUTPUT_OD: pin is used as output in open drain mode
//!   </UL>
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J5_PinInit(u8 pin, mios32_board_pin_mode_t mode)
{
  if( pin >= J5_NUM_PINS )
    return -1; // pin not supported

  if( mode == MIOS32_BOARD_PIN_MODE_IGNORE ) {
    // don't touch
    j5_enable_mask &= ~(1 << pin);
  } else {
    // enable pin
    j5_enable_mask |= (1 << pin);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = j5_pin[pin].pin_mask;

    switch( mode ) {
      case MIOS32_BOARD_PIN_MODE_INPUT:
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	break;
      case MIOS32_BOARD_PIN_MODE_INPUT_PD:
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	break;
      case MIOS32_BOARD_PIN_MODE_INPUT_PU:
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	break;
      case MIOS32_BOARD_PIN_MODE_OUTPUT_PP:
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	break;
      case MIOS32_BOARD_PIN_MODE_OUTPUT_OD:
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	break;
      default:
	return -2; // invalid pin mode
    }

    // init IO mode
    GPIO_Init(j5_pin[pin].port, &GPIO_InitStructure);

    // set pin value to 0
    j5_pin[pin].port->BRR = j5_pin[pin].pin_mask;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets all pins of J5 at once
//! \param[in] value 12 bits which are forwarded to J5A/B/C
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J5_Set(u16 value)
{
#if J5_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J5 not supported
#else
# if defined(MIOS32_BOARD_MBHP_CORE_STM32)
  // J5A[3:0] -> GPIOC[3:0]
  // J5B[3:0] -> GPIOA[3:0]
  // J5C[1:0] -> GPIOC[5:4]
  // J5C[3:2] -> GPIOB[1:0]

  // BSRR[15:0] sets output register to 1, BSRR[31:16] to 0

  GPIOC->BSRR =
    (( value & j5_enable_mask & 0x000f) <<  0) |  // set flags
    ((~value & j5_enable_mask & 0x000f) << 16) |  // clear flags
    (( value & j5_enable_mask & 0x0300) >>  4) |  // set flags
    ((~value & j5_enable_mask & 0x0300) << 12);   // clear flags

  GPIOA->BSRR =
    (( value & j5_enable_mask & 0x00f0) >>  4) |  // set flags
    ((~value & j5_enable_mask & 0x00f0) << 12);   // clear flags

  GPIOB->BSRR =
    (( value & j5_enable_mask & 0x0c00) >> 10) |  // set flags
    ((~value & j5_enable_mask & 0x0c00) <<  6);   // clear flags

  return 0; // no error
# else
  return -2; // board not supported
# endif
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets a single pin of J5
//! \param[in] pin the pin number (0..11)
//! \param[in] value the pin value (0 or 1)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J5_PinSet(u8 pin, u8 value)
{
#if J5_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J5 not supported
#else
  if( pin >= J5_NUM_PINS )
    return -1; // pin not supported

  if( !(j5_enable_mask & (1 << pin)) )
    return -2; // pin disabled

  if( value )
    j5_pin[pin].port->BSRR = j5_pin[pin].pin_mask;
  else
    j5_pin[pin].port->BRR = j5_pin[pin].pin_mask;

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the state of all pins of J5
//! \return 12 bits which are forwarded from J5A/B/C
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J5_Get(void)
{
#if J5_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J5 not supported
#else
# if defined(MIOS32_BOARD_MBHP_CORE_STM32)
  // J5A[3:0] -> GPIOC[3:0]
  // J5B[3:0] -> GPIOA[3:0]
  // J5C[1:0] -> GPIOC[5:4]
  // J5C[3:2] -> GPIOB[1:0]

  return
    (((GPIOC->IDR & 0x000f) <<  0) |
     ((GPIOA->IDR & 0x00f0) <<  4) |
     ((GPIOC->IDR & 0x0030) <<  4) |
     ((GPIOB->IDR & 0x0003) << 10)) & j5_enable_mask;
# else
  return -2; // board not supported
# endif
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the state of a single pin of J5
//! \param[in] pin the pin number (0..11)
//! \return < 0 if pin not available
//! \return >= 0: input state of pin
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J5_PinGet(u8 pin)
{
#if J5_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J5 not supported
#else
  if( pin >= J5_NUM_PINS )
    return -1; // pin not supported

  if( !(j5_enable_mask & (1 << pin)) )
    return -2; // pin disabled

  return (j5_pin[pin].port->IDR & j5_pin[pin].pin_mask) ? 1 : 0;
#endif
}


//! \}

#endif /* MIOS32_DONT_USE_BOARD */
