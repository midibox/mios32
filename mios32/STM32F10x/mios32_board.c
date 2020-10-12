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
#warning "No J5 pins defined for this MIOS32_BOARD"
#endif


/////////////////////////////////////////////////////////////////////////////
// J10 pin mapping
/////////////////////////////////////////////////////////////////////////////

#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
#define J10_NUM_PINS 0
// not supported by this board
#else
#define J10_NUM_PINS 0
#warning "No J10 pins defined for this MIOS32_BOARD"
#endif


/////////////////////////////////////////////////////////////////////////////
// J28 pin mapping
/////////////////////////////////////////////////////////////////////////////

#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
#define J28_NUM_PINS 0
// not supported by this board
#else
#define J28_NUM_PINS 0
#warning "No J28 pins defined for this MIOS32_BOARD"
#endif


/////////////////////////////////////////////////////////////////////////////
// J15 (LCD) pin mapping
/////////////////////////////////////////////////////////////////////////////

#if defined(MIOS32_BOARD_MBHP_CORE_STM32)

#define J15_AVAILABLE 1

#define J15_SCLK_PORT      GPIOA
#define J15_SCLK_PIN       GPIO_Pin_8

#define J15_RCLK_PORT      GPIOC
#define J15_RCLK_PIN       GPIO_Pin_9

#define J15_SER_PORT       GPIOC        // also used as DC (data/command select) for serial interfaces
#define J15_SER_PIN        GPIO_Pin_8

#define J15_E1_PORT        GPIOC        // also used to control SCLK of serial interfaces
#define J15_E1_PIN         GPIO_Pin_7

#define J15_E2_PORT        GPIOC
#define J15_E2_PIN         GPIO_Pin_6

#ifdef MIOS32_BOARD_LCD_E3_PORT
#define LCD_E3_PORT        MIOS32_BOARD_LCD_E3_PORT
#define LCD_E3_PIN         MIOS32_BOARD_LCD_E3_PIN
#endif

#ifdef MIOS32_BOARD_LCD_E4_PORT
#define LCD_E4_PORT        MIOS32_BOARD_LCD_E4_PORT
#define LCD_E4_PIN         MIOS32_BOARD_LCD_E4_PIN
#endif

#define J15_RW_PORT        GPIOB        // also used to control data output of serial interfaces
#define J15_RW_PIN         GPIO_Pin_2

#define J15_D7_PORT        GPIOC
#define J15_D7_PIN         GPIO_Pin_12

// 0: RS connected to SER input of 74HC595 shift register
// 1: RS connected to D7' output of the 74HC595 register (only required if no open drain mode is used, and a 5V RS signal is needed)
#define J15_RS_AT_D7APOSTROPHE 0

// following macros simplify the access to J15 pins
#define J15_PIN_SER(b)  MIOS32_SYS_STM_PINSET(J15_SER_PORT, J15_SER_PIN, b)
#define J15_PIN_E1(b)   MIOS32_SYS_STM_PINSET(J15_E1_PORT, J15_E1_PIN, b)
#define J15_PIN_E2(b)   MIOS32_SYS_STM_PINSET(J15_E2_PORT, J15_E2_PIN, b)
#ifdef MIOS32_BOARD_LCD_E3_PORT
#define LCD_PIN_E3(b)   MIOS32_SYS_STM_PINSET(J15_E3_PORT, J15_E3_PIN, b)
#endif
#ifdef MIOS32_BOARD_LCD_E4_PORT
#define LCD_PIN_E4(b)   MIOS32_SYS_STM_PINSET(J15_E4_PORT, J15_E4_PIN, b)
#endif
#define J15_PIN_RW(b)   MIOS32_SYS_STM_PINSET(J15_RW_PORT, J15_RW_PIN, b)

#define J15_PIN_SERLCD_DATAOUT(b) MIOS32_SYS_STM_PINSET(J15_RW_PORT, J15_RW_PIN, b)
#define J15_PIN_SERLCD_SCLK_0     { MIOS32_SYS_STM_PINSET_0(J15_E1_PORT, J15_E1_PIN); MIOS32_SYS_STM_PINSET_0(J15_E2_PORT, J15_E2_PIN); }
#define J15_PIN_SERLCD_SCLK_1     { MIOS32_SYS_STM_PINSET_1(J15_E1_PORT, J15_E1_PIN); MIOS32_SYS_STM_PINSET_1(J15_E2_PORT, J15_E2_PIN); }

#define J15_PIN_RCLK_0  MIOS32_SYS_STM_PINSET_0(J15_RCLK_PORT, J15_RCLK_PIN)
#define J15_PIN_RCLK_1  MIOS32_SYS_STM_PINSET_1(J15_RCLK_PORT, J15_RCLK_PIN)

#define J15_PIN_SCLK_0  MIOS32_SYS_STM_PINSET_0(J15_SCLK_PORT, J15_SCLK_PIN)
#define J15_PIN_SCLK_1  MIOS32_SYS_STM_PINSET_1(J15_SCLK_PORT, J15_SCLK_PIN)

#define J15_PIN_D7_IN   MIOS32_SYS_STM_PINGET(J15_D7_PORT, J15_D7_PIN)

#else
#define J15_AVAILABLE 0
#warning "No J15 (LCD) port defined for this MIOS32_BOARD"
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
#elif defined(MIOS32_BOARD_BLUE_PILL)
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  // only one LED, connected to PC13
  if( leds & 1 ) {
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
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
    MIOS32_SYS_STM_PINSET(GPIOD, GPIO_Pin_2, value & 1);
  }

  if( leds & 0xfffffffe)
    return -2; // LED doesn't exist

  return 0; // no error
#elif defined(MIOS32_BOARD_BLUE_PILL)
  // only one LED, connected to PC13
  if( leds & 1 ) {
    MIOS32_SYS_STM_PINSET(GPIOC, GPIO_Pin_13, value & 1);
  }

  if( leds & 0xfffffffe)
    return -2; // LED doesn't exist

  return 0; // no error
#elif defined(MIOS32_BOARD_STM32_PRIMER)
  // two LEDs, connected to PB8 (green) and PB9 (red)
  if( leds & 1 ) {
    MIOS32_SYS_STM_PINSET(GPIOB, GPIO_Pin_8, value & 1);
  }
  if( leds & 2 ) {
    MIOS32_SYS_STM_PINSET(GPIOB, GPIO_Pin_9, value & 2);
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
#elif defined(MIOS32_BOARD_BLUE_PILL)
  // only one LED, connected to PC13
  if( GPIOC->ODR & GPIO_Pin_13 )
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
//!     <LI>MIOS32_BOARD_PIN_MODE_IGNORE: configuration shouldn't be touched
//!     <LI>MIOS32_BOARD_PIN_MODE_ANALOG: select analog input mode (default)
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
#if J5_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J5 not supported
#else
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
      case MIOS32_BOARD_PIN_MODE_ANALOG:
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	break;
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

#if 0
    // set pin value to 0
    // This should be done before IO mode configuration, because
    // in input mode, this bit will control Pull Up/Down (configured
    // by GPIO_Init)
    j5_pin[pin].port->BRR = j5_pin[pin].pin_mask;
    // TK: disabled since there are application which have to switch between Input/Output
    // without destroying the current pin value
#endif

    // init IO mode
    GPIO_Init(j5_pin[pin].port, &GPIO_InitStructure);
  }

  return 0; // no error
#endif
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
# warning "Not prepared for this MIOS32_BOARD"
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

  MIOS32_SYS_STM_PINSET(j5_pin[pin].port, j5_pin[pin].pin_mask, value);

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
     ((GPIOA->IDR & 0x000f) <<  4) |
     ((GPIOC->IDR & 0x0030) <<  4) |
     ((GPIOB->IDR & 0x0003) << 10)) & j5_enable_mask;
# else
# warning "Not prepared for this MIOS32_BOARD"
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

  return MIOS32_SYS_STM_PINGET(j5_pin[pin].port, j5_pin[pin].pin_mask);
#endif
}



/////////////////////////////////////////////////////////////////////////////
//! Initializes a J10 pin
//! \param[in] pin the pin number (0..7)
//! \param[in] mode the pin mode
//!   <UL>
//!     <LI>MIOS32_BOARD_PIN_MODE_IGNORE: configuration shouldn't be touched
//!     <LI>MIOS32_BOARD_PIN_MODE_ANALOG: select analog input mode (default)
//!     <LI>MIOS32_BOARD_PIN_MODE_INPUT: pin is used as input w/o pull device (floating)
//!     <LI>MIOS32_BOARD_PIN_MODE_INPUT_PD: pin is used as input, internal pull down enabled
//!     <LI>MIOS32_BOARD_PIN_MODE_INPUT_PU: pin is used as input, internal pull up enabled
//!     <LI>MIOS32_BOARD_PIN_MODE_OUTPUT_PP: pin is used as output in push-pull mode
//!     <LI>MIOS32_BOARD_PIN_MODE_OUTPUT_OD: pin is used as output in open drain mode
//!   </UL>
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J10_PinInit(u8 pin, mios32_board_pin_mode_t mode)
{
#if J10_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J10 not supported
#else
# error "Not prepared for STM32"
  return -1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets all pins of J10 at once
//! \param[in] value 8 bits which are forwarded to J10
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J10_Set(u16 value)
{
#if J10_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J10 not supported
#else
# error "Not prepared for STM32"
  return -1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets a single pin of J10
//! \param[in] pin the pin number (0..7)
//! \param[in] value the pin value (0 or 1)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J10_PinSet(u8 pin, u8 value)
{
#if J10_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J10 not supported
#else
# error "Not prepared for STM32"
  return -1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the state of all pins of J10
//! \return 8 bits which are forwarded from J10
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J10_Get(void)
{
#if J10_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J10 not supported
#else
# error "Not prepared for STM32"
  return -1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the state of a single pin of J10
//! \param[in] pin the pin number (0..7)
//! \return < 0 if pin not available
//! \return >= 0: input state of pin
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J10_PinGet(u8 pin)
{
#if J10_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J10 not supported
#else
# error "Not prepared for STM32"
  return -1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the state of all pins of J10A (J10[7:0])
//! \return 8 bits which are forwarded from J10A
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J10A_Get(void)
{
#if J10_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J10 not supported
#else
# error "Not prepared for STM32"
  return -1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets all pins of J10A (J10[7:0]) at once
//! \param[in] value 8 bits which are forwarded to J10A
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J10A_Set(u8 value)
{
#if J10_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J10 not supported
#else
# error "Not prepared for STM32"
  return -1;
#endif
}



/////////////////////////////////////////////////////////////////////////////
//! This function returns the state of all pins of J10B (J10[15:8])
//! \return 8 bits which are forwarded from J10B
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J10B_Get(void)
{
#if J10_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J10 not supported
#else
# error "Not prepared for STM32"
  return -1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets all pins of J10B (J10[15:8]) at once
//! \param[in] value 8 bits which are forwarded to J10B
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J10B_Set(u8 value)
{
#if J10_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J10 not supported
#else
# error "Not prepared for STM32"
  return -1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes a J28 pin
//! \param[in] pin the pin number (0..7)
//! \param[in] mode the pin mode
//!   <UL>
//!     <LI>MIOS32_BOARD_PIN_MODE_IGNORE: configuration shouldn't be touched
//!     <LI>MIOS32_BOARD_PIN_MODE_ANALOG: select analog input mode (default)
//!     <LI>MIOS32_BOARD_PIN_MODE_INPUT: pin is used as input w/o pull device (floating)
//!     <LI>MIOS32_BOARD_PIN_MODE_INPUT_PD: pin is used as input, internal pull down enabled
//!     <LI>MIOS32_BOARD_PIN_MODE_INPUT_PU: pin is used as input, internal pull up enabled
//!     <LI>MIOS32_BOARD_PIN_MODE_OUTPUT_PP: pin is used as output in push-pull mode
//!     <LI>MIOS32_BOARD_PIN_MODE_OUTPUT_OD: pin is used as output in open drain mode
//!   </UL>
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J28_PinInit(u8 pin, mios32_board_pin_mode_t mode)
{
#if J28_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J28 not supported
#else
# error "Not prepared for STM32"
  return -1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets all pins of J28 at once
//! \param[in] value 8 bits which are forwarded to J28
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J28_Set(u16 value)
{
#if J28_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J28 not supported
#else
# error "Not prepared for STM32"
  return -1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets a single pin of J28
//! \param[in] pin the pin number (0..7)
//! \param[in] value the pin value (0 or 1)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J28_PinSet(u8 pin, u8 value)
{
#if J28_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J28 not supported
#else
# error "Not prepared for STM32"
  return -1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the state of all pins of J28
//! \return 8 bits which are forwarded from J28
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J28_Get(void)
{
#if J28_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J28 not supported
#else
# error "Not prepared for STM32"
  return -1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the state of a single pin of J28
//! \param[in] pin the pin number (0..7)
//! \return < 0 if pin not available
//! \return >= 0: input state of pin
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J28_PinGet(u8 pin)
{
#if J28_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J28 not supported
#else
# error "Not prepared for STM32"
  return -1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes the J15 port
//! \param[in] mode 
//! <UL>
//!   <LI>0: J15 pins are configured in Push Pull Mode (3.3V)
//!   <LI>1: J15 pins are configured in Open Drain mode (perfect for 3.3V->5V levelshifting)
//! </UL>
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J15_PortInit(u32 mode)
{
#if J15_AVAILABLE == 0
  return -1; // LCD port not available
#else
  // currently only mode 0 and 1 supported
  if( mode != 0 && mode != 1 )
    return -1; // unsupported mode

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);

  J15_PIN_SCLK_0;
  J15_PIN_RCLK_0;
  J15_PIN_RW(0);
  J15_PIN_E1(0);
  J15_PIN_E2(0);
#ifdef MIOS32_BOARD_LCD_E3_PORT
  LCD_PIN_E3(0);
#endif
#ifdef MIOS32_BOARD_LCD_E4_PORT
  LCD_PIN_E4(0);
#endif

  // configure push-pull pins
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; // weak driver to reduce transients

  GPIO_InitStructure.GPIO_Pin = J15_SCLK_PIN;
  GPIO_Init(J15_SCLK_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = J15_RCLK_PIN;
  GPIO_Init(J15_RCLK_PORT, &GPIO_InitStructure);

  // configure open-drain pins (if OD option enabled)
  if( mode )
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;

  GPIO_InitStructure.GPIO_Pin = J15_SER_PIN;
  GPIO_Init(J15_SER_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = J15_E1_PIN;
  GPIO_Init(J15_E1_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = J15_E2_PIN;
  GPIO_Init(J15_E2_PORT, &GPIO_InitStructure);

#ifdef MIOS32_BOARD_LCD_E3_PORT
  GPIO_InitStructure.GPIO_Pin = LCD_E3_PIN;
  GPIO_Init(LCD_E3_PORT, &GPIO_InitStructure);
#endif

#ifdef MIOS32_BOARD_LCD_E4_PORT
  GPIO_InitStructure.GPIO_Pin = LCD_E4_PIN;
  GPIO_Init(LCD_E4_PORT, &GPIO_InitStructure);
#endif

  GPIO_InitStructure.GPIO_Pin = J15_RW_PIN;
  GPIO_Init(J15_RW_PORT, &GPIO_InitStructure);

  // configure "busy" input with pull-up
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = J15_D7_PIN;
  GPIO_Init(J15_D7_PORT, &GPIO_InitStructure);

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function is used by LCD drivers under $MIOS32_PATH/modules/app_lcd
//! to output an 8bit value on the data lines
//! \param[in] data the 8bit value
//! \return < 0 if access to data port not supported by board
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J15_DataSet(u8 data)
{
#if J15_AVAILABLE == 0
  return -1; // LCD port not available
#else
  // shift in 8bit data
  // whole function takes ca. 1.5 uS @ 72MHz
  // thats acceptable for a (C)LCD, which is normaly busy after each access for ca. 20..40 uS

  J15_PIN_SER(data & 0x80); // D7
  J15_PIN_SCLK_0; // setup delay
  J15_PIN_SCLK_1;
  J15_PIN_SER(data & 0x40); // D6
  J15_PIN_SCLK_0; // setup delay
  J15_PIN_SCLK_1;
  J15_PIN_SER(data & 0x20); // D5
  J15_PIN_SCLK_0; // setup delay
  J15_PIN_SCLK_1;
  J15_PIN_SER(data & 0x10); // D4
  J15_PIN_SCLK_0; // setup delay
  J15_PIN_SCLK_1;
  J15_PIN_SER(data & 0x08); // D3
  J15_PIN_SCLK_0; // setup delay
  J15_PIN_SCLK_1;
  J15_PIN_SER(data & 0x04); // D2
  J15_PIN_SCLK_0; // setup delay
  J15_PIN_SCLK_1;
  J15_PIN_SER(data & 0x02); // D1
  J15_PIN_SCLK_0; // setup delay
  J15_PIN_SCLK_1;
  J15_PIN_SER(data & 0x01); // D0
  J15_PIN_SCLK_0; // setup delay
  J15_PIN_SCLK_1;
  J15_PIN_SCLK_1;

  // transfer to output register
  J15_PIN_RCLK_1;
  J15_PIN_RCLK_1;
  J15_PIN_RCLK_1;
  J15_PIN_RCLK_0;

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function is used by LCD drivers under $MIOS32_PATH/modules/app_lcd
//! to shift an 8bit data value to LCDs with serial interface
//! (SCLK connected to J15A:E, Data line connected to J15A:RW)
//! \param[in] data the 8bit value
//! \return < 0 if access to data port not supported by board
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J15_SerDataShift(u8 data)
{
#if J15_AVAILABLE == 0
  return -1; // LCD port not available
#else
  MIOS32_IRQ_Disable();

  J15_PIN_SERLCD_DATAOUT(data & 0x80); // D7
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_DATAOUT(data & 0x40); // D6
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_DATAOUT(data & 0x20); // D5
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_DATAOUT(data & 0x10); // D4
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_DATAOUT(data & 0x08); // D3
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_DATAOUT(data & 0x04); // D2
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_DATAOUT(data & 0x02); // D1
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_DATAOUT(data & 0x01); // D0
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_SCLK_1;

  // set SERLCD_DATAOUT (=J15.RW) to an active level
  // RW is unfortunately also connected to OE# of the 74HC595, which drives the CS lines.
  // With RW=1 the CS lines will go into high impedance state, and after 100..200 uS we will
  // see on the scope how CS line levels are changing, e.g. from high to mid level.
  // Although I assume that this can't cause an issue (because CS is only relevant when
  // the display gets a SCLK_0->SCLK_1 clock), I feel better when we ensure a proper CS state
  // after each MIOS32_BOARD_J15_SerDataShift call.
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_DATAOUT(0); 

  MIOS32_IRQ_Enable();

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function is used by LCD drivers under $MIOS32_PATH/modules/app_lcd
//! to set the RS pin
//! \param[in] rs state of the RS pin
//! \return < 0 if access to RS pin not supported by board
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J15_RS_Set(u8 rs)
{
#if J15_AVAILABLE == 0
  return -1; // LCD port not available
#else
  J15_PIN_SER(rs);

#if J15_RS_AT_D7APOSTROPHE
  // RS connected to D7' output of the 74HC595 register (only required if no open drain mode is used, and a 5V RS signal is needed)
  // shift RS to D7' 
  // These 8 shifts take ca. 500 nS @ 72MHz, they don't really hurt
  J15_PIN_SCLK_0;
  J15_PIN_SCLK_1;
  J15_PIN_SCLK_0;
  J15_PIN_SCLK_1;
  J15_PIN_SCLK_0;
  J15_PIN_SCLK_1;
  J15_PIN_SCLK_0;
  J15_PIN_SCLK_1;
  J15_PIN_SCLK_0;
  J15_PIN_SCLK_1;
  J15_PIN_SCLK_0;
  J15_PIN_SCLK_1;
  J15_PIN_SCLK_0;
  J15_PIN_SCLK_1;
  J15_PIN_SCLK_0;
  J15_PIN_SCLK_1;
  J15_PIN_SCLK_0;
#endif

  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! This function is used by LCD drivers under $MIOS32_PATH/modules/app_lcd
//! to set the RW pin
//! \param[in] rw state of the RW pin
//! \return < 0 if access to RW pin not supported by board
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J15_RW_Set(u8 rw)
{
#if J15_AVAILABLE == 0
  return -1; // LCD port not available
#else
  J15_PIN_RW(rw);

  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! This function is used by LCD drivers under $MIOS32_PATH/modules/app_lcd
//! to set the E pin
//! \param[in] lcd display port (0=J15A, 1=J15B)
//! \param[in] e state of the E pin
//! \return < 0 if access to E pin not supported by board
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J15_E_Set(u8 lcd, u8 e)
{
#if J15_AVAILABLE == 0
  return -1; // LCD port not available
#else
  switch( lcd ) {
    case 0: 
      J15_PIN_E1(e);
      return 0; // no error

    case 1: 
      J15_PIN_E2(e);
      return 0; // no error

#ifdef MIOS32_BOARD_LCD_E3_PORT
    case 2: 
      LCD_PIN_E3(e);
      return 0; // no error
#endif

#ifdef MIOS32_BOARD_LCD_E4_PORT
    case 3: 
      LCD_PIN_E4(e);
      return 0; // no error
#endif

  }

  return -1; // pin not available
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the state of the D7_IN pin
//! return < 0 if LCD port not available
//! return 0 if logic level is 0
//! return 1 if logic level is 1
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J15_GetD7In(void)
{
#if J15_AVAILABLE == 0
  return -1; // LCD port not available
#else
  return J15_PIN_D7_IN ? 1 : 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function enables/disables the pull up on the D7 pin
//! return < 0 if LCD port not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J15_D7InPullUpEnable(u8 enable)
{
#if J15_AVAILABLE == 0
  return -1; // LCD port not available
#else
  return 0; // not relevant for MBHP_CORE_STM32 board
  // TODO: check if this is true 
#endif
}



/////////////////////////////////////////////////////////////////////////////
//! This function is used by LCD drivers under $MIOS32_PATH/modules/app_lcd
//! to poll the busy bit (D7) of a LCD
//! \param[in] lcd display port (0=J15A, 1=J15B)
//! \param[in] time_out how many times should the busy bit be polled?
//! \return -1 if LCD not available
//! \return -2 on timeout
//! return >= 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J15_PollUnbusy(u8 lcd, u32 time_out)
{
#if J15_AVAILABLE == 0
  return -1; // LCD port not available
#else
  u32 poll_ctr;
  u32 delay_ctr;

  // select command register (RS=0)
  MIOS32_BOARD_J15_RS_Set(0);

  // enable pull-up
  MIOS32_BOARD_J15_D7InPullUpEnable(1);

  // select read (will also disable output buffer of 74HC595)
  MIOS32_BOARD_J15_RW_Set(1);

  // check if E pin is available
  if( MIOS32_BOARD_J15_E_Set(lcd, 1) < 0 )
    return -1; // LCD port not available

  // poll busy flag, timeout after 10 mS
  // each loop takes ca. 4 uS @ 72MHz, this has to be considered when defining the time_out value
  for(poll_ctr=time_out; poll_ctr>0; --poll_ctr) {
    MIOS32_BOARD_J15_E_Set(lcd, 1);

    // due to slow slope we should wait at least for 1 uS
    for(delay_ctr=0; delay_ctr<10; ++delay_ctr)
      MIOS32_BOARD_J15_RW_Set(1);

    u32 busy = MIOS32_BOARD_J15_GetD7In();
    MIOS32_BOARD_J15_E_Set(lcd, 0);
    if( !busy )
      break;
  }

  // disable pull-up
  MIOS32_BOARD_J15_D7InPullUpEnable(0);

  // deselect read (output buffers of 74HC595 enabled again)
  MIOS32_BOARD_J15_RW_Set(0);

  // timeout?
  if( poll_ctr == 0 )
    return -2; // timeout error

  return 0; // no error
#endif
}



/////////////////////////////////////////////////////////////////////////////
//! This function enables or disables one of the two DAC channels provided by 
//! STM32F103RE (and not by STM32F103RB).
//!
//! <UL>
//!  <LI>the first channel (chn == 0) is output at pin RA4 (J16:RC1 of the MBHP_CORE_STM32 module)
//!  <LI>the second channel (chn == 1) is output at pin RA5 (J16:SC of the MBHP_CORE_STM32 module)
//! </UL>
//! 
//! \param[in] chn channel number (0 or 1)
//! \param[in] enable 0: channel disabled, 1: channel enabled.
//! \return < 0 if DAC channel not supported (e.g. STM32F103RB)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_DAC_PinInit(u8 chn, u8 enable)
{
#if defined(MIOS32_PROCESSOR_STM32F103RB)
  return -1; // generally not supported. Try DAC access for all other processors
#else
  if( chn >= 2 )
    return -1; // channel not supported

  if( enable ) {
    // enable DAC clock
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

    // Once the DAC channel is enabled, the corresponding GPIO pin is automatically 
    // connected to the DAC converter. In order to avoid parasitic consumption, 
    // the GPIO pin should be configured in analog
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;

    // init DAC
    DAC_InitTypeDef            DAC_InitStructure;
    DAC_InitStructure.DAC_Trigger = DAC_Trigger_Software;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;

    switch( chn ) {
      case 0:
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	DAC_Init(DAC_Channel_1, &DAC_InitStructure);
	DAC_Cmd(DAC_Channel_1, ENABLE);
	break;

      case 1:
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_5;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	DAC_Init(DAC_Channel_2, &DAC_InitStructure);
	DAC_Cmd(DAC_Channel_2, ENABLE);
	break;

      default:
	return -2; // unexpected (chn already checked above)
    }
    
  } else {
    // disable DAC channel
    switch( chn ) {
      case 0: DAC_Cmd(DAC_Channel_1, DISABLE); break;
      case 1: DAC_Cmd(DAC_Channel_2, DISABLE); break;
      default:
	return -2; // unexpected (chn already checked above)
    }
  }

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets an output channel to a given 16-bit value.
//!
//! Note that actually the DAC will work at 12-bit resolution. The lowest
//! 4 bits are ignored (reserved for future STM chips).
//! \param[in] chn channel number (0 or 1)
//! \param[in] value the 16-bit value (0..65535). Lowest 4 bits are ignored.
//! \return < 0 if DAC channel not supported (e.g. STM32F103RB)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_DAC_PinSet(u8 chn, u16 value)
{
#if defined(MIOS32_PROCESSOR_STM32F103RB)
  return -1; // generally not supported. Try DAC access for all other processors
#else
  switch( chn ) {
    case 0:
      DAC_SetChannel1Data(DAC_Align_12b_L, value);
      DAC_SoftwareTriggerCmd(DAC_Channel_1, ENABLE);
      break;

    case 1:
      DAC_SetChannel2Data(DAC_Align_12b_L, value);
      DAC_SoftwareTriggerCmd(DAC_Channel_2, ENABLE);
      break;

    default:
      return -1; // channel not supported
  }

  return 0; // no error
#endif
}

//! \}

#endif /* MIOS32_DONT_USE_BOARD */
