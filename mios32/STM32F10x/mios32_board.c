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
// J15 (LCD) pin mapping
/////////////////////////////////////////////////////////////////////////////

#if defined(MIOS32_BOARD_MBHP_CORE_STM32)

#define J15_AVAILABLE 1

#define J15_SCLK_PORT      GPIOA
#define J15_SCLK_PIN       GPIO_Pin_8

#define J15_RCLK_PORT      GPIOC
#define J15_RCLK_PIN       GPIO_Pin_9

#define J15_SER_PORT       GPIOC
#define J15_SER_PIN        GPIO_Pin_8

#define J15_E1_PORT        GPIOC        // also used to control SCLK of serial interfaces
#define J15_E1_PIN         GPIO_Pin_7

#define J15_E2_PORT        GPIOC
#define J15_E2_PIN         GPIO_Pin_6

#define J15_RW_PORT        GPIOB        // also used to control data output of serial interfaces
#define J15_RW_PIN         GPIO_Pin_2

#define J15_D7_PORT        GPIOC
#define J15_D7_PIN         GPIO_Pin_12

// 0: RS connected to SER input of 74HC595 shift register
// 1: RS connected to D7' output of the 74HC595 register (only required if no open drain mode is used, and a 5V RS signal is needed)
#define J15_RS_AT_D7APOSTROPHE 0

// following macros simplify the access to J15 pins
#define J15_PIN_SER(b)  { J15_SER_PORT->BSRR = (b) ? J15_SER_PIN : (J15_SER_PIN << 16); }
#define J15_PIN_E1(b)   { J15_E1_PORT->BSRR  = (b) ? J15_E1_PIN  : (J15_E1_PIN << 16); }
#define J15_PIN_E2(b)   { J15_E2_PORT->BSRR  = (b) ? J15_E2_PIN  : (J15_E2_PIN << 16); }
#define J15_PIN_RW(b)   { J15_RW_PORT->BSRR  = (b) ? J15_RW_PIN  : (J15_RW_PIN << 16); }

#define J15_PIN_SERLCD_DATAOUT(b) { J15_RW_PORT->BSRR = (b) ? J15_RW_PIN  : (J15_RW_PIN << 16); }
#define J15_PIN_SERLCD_SCLK_0     { J15_E1_PORT->BRR  = J15_E1_PIN; }
#define J15_PIN_SERLCD_SCLK_1     { J15_E1_PORT->BSRR = J15_E1_PIN; }

#define J15_PIN_RCLK_0  { J15_RCLK_PORT->BRR  = J15_RCLK_PIN; }
#define J15_PIN_RCLK_1  { J15_RCLK_PORT->BSRR = J15_RCLK_PIN; }

#define J15_PIN_SCLK_0  { J15_SCLK_PORT->BRR  = J15_SCLK_PIN; }
#define J15_PIN_SCLK_1  { J15_SCLK_PORT->BSRR = J15_SCLK_PIN; }

#define J15_PIN_D7_IN   (J15_D7_PORT->IDR & J15_D7_PIN)

#else
#define J15_AVAILABLE 0
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

    // set pin value to 0
    // This should be done before IO mode configuration, because
    // in input mode, this bit will control Pull Up/Down (configured
    // by GPIO_Init)
    j5_pin[pin].port->BRR = j5_pin[pin].pin_mask;

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
  }

  return -1; // pin not available
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

    u32 busy = J15_PIN_D7_IN;
    MIOS32_BOARD_J15_E_Set(lcd, 0);
    if( !busy )
      break;
  }

  // deselect read (output buffers of 74HC595 enabled again)
  MIOS32_BOARD_J15_RW_Set(0);

  // timeout?
  if( poll_ctr == 0 )
    return -2; // timeout error

  return 0; // no error
#endif
}

//! \}

#endif /* MIOS32_DONT_USE_BOARD */
