// $Id$
//! \defgroup MIOS32_BOARD
//!
//! Development Board specific functions for MIOS32
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
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

#if defined(MIOS32_BOARD_MBHP_CORE_LPC17) || defined(MIOS32_BOARD_LPCXPRESSO)
// note: adaptions also have to be done in MIOS32_BOARD_J5_(Set/Get),
// since these functions access the ports directly
typedef struct {
  u8 port;
  u8 pin;
} j5_pin_t;

#define J5_NUM_PINS 8
static const j5_pin_t j5_pin[J5_NUM_PINS] = {
  // J5A
  { 0, 23 },
  { 0, 24 },
  { 0, 25 },
  { 0, 26 },

  // J5BB
  { 1, 30 },
  { 1, 31 },
  { 0,  3 },
  { 0,  2 },
};

#else
#define J5_NUM_PINS 0
#warning "No J5 pins defined for this MIOS32_BOARD"
#endif


/////////////////////////////////////////////////////////////////////////////
// J10 pin mapping
/////////////////////////////////////////////////////////////////////////////

#if defined(MIOS32_BOARD_MBHP_CORE_LPC17) || defined(MIOS32_BOARD_LPCXPRESSO)
// note: adaptions also have to be done in MIOS32_BOARD_J10_(Set/Get),
// since these functions access the ports directly
typedef struct {
  u8 port;
  u8 pin;
} j10_pin_t;

#define J10_NUM_PINS 8
static const j10_pin_t j10_pin[J10_NUM_PINS] = {
  // J10
  { 2, 2 },
  { 2, 3 },
  { 2, 4 },
  { 2, 5 },
  { 2, 6 },
  { 2, 7 },
  { 2, 8 },
  { 1, 18 },
};

#else
#define J10_NUM_PINS 0
#warning "No J10 pins defined for this MIOS32_BOARD"
#endif


/////////////////////////////////////////////////////////////////////////////
// J15 (LCD) pin mapping
/////////////////////////////////////////////////////////////////////////////

#if defined(MIOS32_BOARD_MBHP_CORE_LPC17) || defined(MIOS32_BOARD_LPCXPRESSO)

#define J15_AVAILABLE 1

#define J15_SCLK_PORT      1
#define J15_SCLK_PIN       25

#define J15_RCLK_PORT      1
#define J15_RCLK_PIN       26

#define J15_SER_PORT       1        // also used as DC (data/command select) for serial interfaces
#define J15_SER_PIN        28

#define J15_E1_PORT        1        // also used to control SCLK of serial interfaces
#define J15_E1_PIN         29

#define J15_E2_PORT        3
#define J15_E2_PIN         25

#define J15_RW_PORT        1        // also used to control data output of serial interfaces
#define J15_RW_PIN         27

#define J15_D7_PORT        1
#define J15_D7_PIN         19

// following macros simplify the access to J15 pins
#define J15_PIN_SER(b)  MIOS32_SYS_LPC_PINSET(J15_SER_PORT, J15_SER_PIN, b)
#define J15_PIN_E1(b)   MIOS32_SYS_LPC_PINSET(J15_E1_PORT, J15_E1_PIN, b)
#define J15_PIN_E2(b)   MIOS32_SYS_LPC_PINSET(J15_E2_PORT, J15_E2_PIN, b)
#define J15_PIN_RW(b)   MIOS32_SYS_LPC_PINSET(J15_RW_PORT, J15_RW_PIN, b)

#define J15_PIN_SERLCD_DATAOUT(b) MIOS32_SYS_LPC_PINSET(J15_RW_PORT, J15_RW_PIN, b)
#define J15_PIN_SERLCD_SCLK_0     MIOS32_SYS_LPC_PINSET_0(J15_E1_PORT, J15_E1_PIN)
#define J15_PIN_SERLCD_SCLK_1     MIOS32_SYS_LPC_PINSET_1(J15_E1_PORT, J15_E1_PIN)

#define J15_PIN_RCLK_0  MIOS32_SYS_LPC_PINSET_0(J15_RCLK_PORT, J15_RCLK_PIN)
#define J15_PIN_RCLK_1  MIOS32_SYS_LPC_PINSET_1(J15_RCLK_PORT, J15_RCLK_PIN)

#define J15_PIN_SCLK_0  MIOS32_SYS_LPC_PINSET_0(J15_SCLK_PORT, J15_SCLK_PIN)
#define J15_PIN_SCLK_1  MIOS32_SYS_LPC_PINSET_1(J15_SCLK_PORT, J15_SCLK_PIN)

#define J15_PIN_D7_IN   MIOS32_SYS_LPC_PINGET(J15_D7_PORT, J15_D7_PIN)

#else
#define J15_AVAILABLE 0
#warning "No J15 (LCD) port defined for this MIOS32_BOARD"
#endif


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u16 j5_enable_mask;
static u8  j10_enable_mask;



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
  j10_enable_mask = 0;

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
#if defined(MIOS32_BOARD_LPCXPRESSO) || defined(MIOS32_BOARD_MBHP_CORE_LPC17)
  if( leds & 1 ) {
    // select GPIO for P0.22
    MIOS32_SYS_LPC_PINSEL(0, 22, 0);
    // enable output driver of P0.22
    MIOS32_SYS_LPC_PINDIR(0, 22, 1);
    // disable open drain
    MIOS32_SYS_LPC_PINMODE_OD(0, 22, 0);
  }

  if( leds & 0xfffffffe)
    return -2; // LED doesn't exist

  return 0; // no error
#else
#warning "no LED specified for this MIOS32_BOARD!"
  return -1;
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
#if defined(MIOS32_BOARD_LPCXPRESSO) || defined(MIOS32_BOARD_MBHP_CORE_LPC17)
  // only one LED, connected to P0.22
  if( leds & 1 ) {
    MIOS32_SYS_LPC_PINSET(0, 22, value & 1);
  }

  if( leds & 0xfffffffe)
    return -2; // LED doesn't exist

  return 0; // no error
#else
#warning "no LED specified for this MIOS32_BOARD!"
  return -1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the status of all LEDs
//! \return status of all LEDs
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_BOARD_LED_Get(void)
{
  u32 values = 0;

#if defined(MIOS32_BOARD_LPCXPRESSO) || defined(MIOS32_BOARD_MBHP_CORE_LPC17)
  // only one LED, connected to P0.22
  if( MIOS32_SYS_LPC_PINGET(0, 22) )
    values |= (1 << 0);
#else
#warning "no LED specified for this MIOS32_BOARD!"
  return -1;
#endif

  return values;
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes a J5 pin
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

    j5_pin_t *p = (j5_pin_t *)&j5_pin[pin];

    switch( mode ) {
      case MIOS32_BOARD_PIN_MODE_ANALOG:
	MIOS32_SYS_LPC_PINSEL(p->port, p->pin, 1); // select ADC
	MIOS32_SYS_LPC_PINMODE(p->port, p->pin, 2); // set to floating... doesn't matter, but also doesn't hurt
	MIOS32_SYS_LPC_PINDIR(p->port, p->pin, 0); // input mode
	break;
      case MIOS32_BOARD_PIN_MODE_INPUT:
	MIOS32_SYS_LPC_PINSEL(p->port, p->pin, 0);  // select GPIO
	MIOS32_SYS_LPC_PINMODE(p->port, p->pin, 2); // set to floating
	MIOS32_SYS_LPC_PINDIR(p->port, p->pin, 0); // input mode
	break;
      case MIOS32_BOARD_PIN_MODE_INPUT_PD:
	MIOS32_SYS_LPC_PINSEL(p->port, p->pin, 0);  // select GPIO
	MIOS32_SYS_LPC_PINMODE(p->port, p->pin, 3); // enable pull-down
	MIOS32_SYS_LPC_PINDIR(p->port, p->pin, 0); // input mode
	break;
      case MIOS32_BOARD_PIN_MODE_INPUT_PU:
	MIOS32_SYS_LPC_PINSEL(p->port, p->pin, 0);  // select GPIO
	MIOS32_SYS_LPC_PINMODE(p->port, p->pin, 0); // enable pull-up
	MIOS32_SYS_LPC_PINDIR(p->port, p->pin, 0); // input mode
	break;
      case MIOS32_BOARD_PIN_MODE_OUTPUT_PP:
	MIOS32_SYS_LPC_PINSEL(p->port, p->pin, 0);  // select GPIO
	MIOS32_SYS_LPC_PINDIR(p->port, p->pin, 1); // output mode
	MIOS32_SYS_LPC_PINMODE_OD(p->port, p->pin, 0); // disable open-drain
	break;
      case MIOS32_BOARD_PIN_MODE_OUTPUT_OD:
	MIOS32_SYS_LPC_PINSEL(p->port, p->pin, 0);  // select GPIO
	MIOS32_SYS_LPC_PINDIR(p->port, p->pin, 1); // output mode
	MIOS32_SYS_LPC_PINMODE_OD(p->port, p->pin, 1); // enable open-drain
	break;
      default:
	return -2; // invalid pin mode
    }

#if 0
    // set pin value to 0
    // This should be done before IO mode configuration, because
    // in input mode, this bit will control Pull Up/Down (configured
    // by GPIO_Init)
    MIOS32_SYS_LPC_PINSET(p->port, p->pin, 0);
    // TK: disabled since there are application which have to switch between Input/Output
    // without destroying the current pin value
#endif
  }

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets all pins of J5 at once
//! \param[in] value 8 bits which are forwarded to J5A/B
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J5_Set(u16 value)
{
#if J5_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J5 not supported
#else
# if defined(MIOS32_BOARD_MBHP_CORE_LPC17) || defined(MIOS32_BOARD_LPCXPRESSO)
  // J5A[3:0] -> P0[26:23]
  // J5B[1:0] -> P1[31:30]
  // J5B[2] -> P0[3]
  // J5B[3] -> P0[2]

  LPC_GPIO0->FIOSET = 
    (( value & j5_enable_mask & 0x000f) << 23) |  // set flags
    (( value & j5_enable_mask & 0x0040) >>  3) |  // set flags
    (( value & j5_enable_mask & 0x0080) >>  5);

  LPC_GPIO1->FIOSET = 
    (( value & j5_enable_mask & 0x0030) << (30-4)); // set flags

  LPC_GPIO0->FIOCLR = 
    ((~value & j5_enable_mask & 0x000f) << 23) |  // clear flags
    ((~value & j5_enable_mask & 0x0040) >>  3) |  // clear flags
    ((~value & j5_enable_mask & 0x0080) >>  5);

  LPC_GPIO1->FIOCLR = 
    ((~value & j5_enable_mask & 0x0030) << (30-4)); // clear flags

  return 0; // no error
# else
# warning "Not prepared for this MIOS32_BOARD"
  return -2; // board not supported
# endif
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets a single pin of J5
//! \param[in] pin the pin number (0..7)
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

  j5_pin_t *p = (j5_pin_t *)&j5_pin[pin];
  MIOS32_SYS_LPC_PINSET(p->port, p->pin, value);

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the state of all pins of J5
//! \return 8 bits which are forwarded from J5A/B
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_J5_Get(void)
{
#if J5_NUM_PINS == 0
  return -1; // MIOS32_BOARD_J5 not supported
#else
# if defined(MIOS32_BOARD_MBHP_CORE_LPC17) || defined(MIOS32_BOARD_LPCXPRESSO)
  // J5A[3:0] -> P0[26:23]
  // J5B[1:0] -> P1[31:30]
  // J5B[2] -> P0[3]
  // J5B[3] -> P0[2]

  u32 p0 = LPC_GPIO0->FIOPIN;
  u32 p1 = LPC_GPIO1->FIOPIN;
  return
    (((p0 >> (23-0)) & 0x000f) |
     ((p0 >> (30-4)) & 0x0030) |
     ((p1 << (6-3))  & 0x0040) |
     ((p1 << (7-2))  & 0x0080));
# else
# warning "Not prepared for this MIOS32_BOARD"
  return -2; // board not supported
# endif
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the state of a single pin of J5
//! \param[in] pin the pin number (0..7)
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

  j5_pin_t *p = (j5_pin_t *)&j5_pin[pin];
  return MIOS32_SYS_LPC_PINGET(p->port, p->pin) ? 1 : 0;
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
  if( pin >= J10_NUM_PINS )
    return -1; // pin not supported

  if( mode == MIOS32_BOARD_PIN_MODE_IGNORE ) {
    // don't touch
    j10_enable_mask &= ~(1 << pin);
  } else {
    // enable pin
    j10_enable_mask |= (1 << pin);

    j10_pin_t *p = (j10_pin_t *)&j10_pin[pin];

    switch( mode ) {
      case MIOS32_BOARD_PIN_MODE_ANALOG:
	MIOS32_SYS_LPC_PINSEL(p->port, p->pin, 1); // select ADC
	MIOS32_SYS_LPC_PINMODE(p->port, p->pin, 2); // set to floating... doesn't matter, but also doesn't hurt
	MIOS32_SYS_LPC_PINDIR(p->port, p->pin, 0); // input mode
	break;
      case MIOS32_BOARD_PIN_MODE_INPUT:
	MIOS32_SYS_LPC_PINSEL(p->port, p->pin, 0);  // select GPIO
	MIOS32_SYS_LPC_PINMODE(p->port, p->pin, 2); // set to floating
	MIOS32_SYS_LPC_PINDIR(p->port, p->pin, 0); // input mode
	break;
      case MIOS32_BOARD_PIN_MODE_INPUT_PD:
	MIOS32_SYS_LPC_PINSEL(p->port, p->pin, 0);  // select GPIO
	MIOS32_SYS_LPC_PINMODE(p->port, p->pin, 3); // enable pull-down
	MIOS32_SYS_LPC_PINDIR(p->port, p->pin, 0); // input mode
	break;
      case MIOS32_BOARD_PIN_MODE_INPUT_PU:
	MIOS32_SYS_LPC_PINSEL(p->port, p->pin, 0);  // select GPIO
	MIOS32_SYS_LPC_PINMODE(p->port, p->pin, 0); // enable pull-up
	MIOS32_SYS_LPC_PINDIR(p->port, p->pin, 0); // input mode
	break;
      case MIOS32_BOARD_PIN_MODE_OUTPUT_PP:
	MIOS32_SYS_LPC_PINSEL(p->port, p->pin, 0);  // select GPIO
	MIOS32_SYS_LPC_PINDIR(p->port, p->pin, 1); // output mode
	MIOS32_SYS_LPC_PINMODE_OD(p->port, p->pin, 0); // disable open-drain
	break;
      case MIOS32_BOARD_PIN_MODE_OUTPUT_OD:
	MIOS32_SYS_LPC_PINSEL(p->port, p->pin, 0);  // select GPIO
	MIOS32_SYS_LPC_PINDIR(p->port, p->pin, 1); // output mode
	MIOS32_SYS_LPC_PINMODE_OD(p->port, p->pin, 1); // enable open-drain
	break;
      default:
	return -2; // invalid pin mode
    }

#if 0
    // set pin value to 0
    // This should be done before IO mode configuration, because
    // in input mode, this bit will control Pull Up/Down (configured
    // by GPIO_Init)
    MIOS32_SYS_LPC_PINSET(p->port, p->pin, 0);
    // TK: disabled since there are application which have to switch between Input/Output
    // without destroying the current pin value
#endif
  }

  return 0; // no error
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
# if defined(MIOS32_BOARD_MBHP_CORE_LPC17) || defined(MIOS32_BOARD_LPCXPRESSO)
  // J10[6:0] -> P2[8:2]
  // J10[7] -> P1[18]

  LPC_GPIO2->FIOSET = 
    (( value & j10_enable_mask & 0x007f) << 2); // set flags

  LPC_GPIO1->FIOSET = 
    (( value & j10_enable_mask & 0x0080) << (18-7)); // set flags

  LPC_GPIO2->FIOCLR = 
    ((~value & j10_enable_mask & 0x007f) << 2); // clear flags

  LPC_GPIO1->FIOCLR = 
    ((~value & j10_enable_mask & 0x0080) << (18-7)); // clear flags

  return 0; // no error
# else
# warning "Not prepared for this MIOS32_BOARD"
  return -2; // board not supported
# endif
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
  if( pin >= J10_NUM_PINS )
    return -1; // pin not supported

  if( !(j10_enable_mask & (1 << pin)) )
    return -2; // pin disabled

  j10_pin_t *p = (j10_pin_t *)&j10_pin[pin];
  MIOS32_SYS_LPC_PINSET(p->port, p->pin, value);

  return 0; // no error
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
# if defined(MIOS32_BOARD_MBHP_CORE_LPC17) || defined(MIOS32_BOARD_LPCXPRESSO)
  // J10[6:0] -> P2[8:2]
  // J10[7] -> P1[18]

  u32 p2 = LPC_GPIO2->FIOPIN;
  u32 p1 = LPC_GPIO1->FIOPIN;
  return
    (((p2 >> (2)) & 0x007f) |
     ((p1 >> (18-7)) & 0x0080));
# else
# warning "Not prepared for this MIOS32_BOARD"
  return -2; // board not supported
# endif
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
  if( pin >= J10_NUM_PINS )
    return -1; // pin not supported

  if( !(j10_enable_mask & (1 << pin)) )
    return -2; // pin disabled

  j10_pin_t *p = (j10_pin_t *)&j10_pin[pin];
  return MIOS32_SYS_LPC_PINGET(p->port, p->pin) ? 1 : 0;
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

  J15_PIN_SCLK_0;
  J15_PIN_RCLK_0;
  J15_PIN_RW(0);
  J15_PIN_E1(0);
  J15_PIN_E2(0);

  // configure push-pull pins
  MIOS32_SYS_LPC_PINSEL(J15_SCLK_PORT, J15_SCLK_PIN, 0); // GPIO
  MIOS32_SYS_LPC_PINDIR(J15_SCLK_PORT, J15_SCLK_PIN, 1); // output
  MIOS32_SYS_LPC_PINMODE_OD(J15_SCLK_PORT, J15_SCLK_PIN, 0); // no open drain

  MIOS32_SYS_LPC_PINSEL(J15_RCLK_PORT, J15_RCLK_PIN, 0); // GPIO
  MIOS32_SYS_LPC_PINDIR(J15_RCLK_PORT, J15_RCLK_PIN, 1); // output
  MIOS32_SYS_LPC_PINMODE_OD(J15_RCLK_PORT, J15_RCLK_PIN, 0); // no open drain

  MIOS32_SYS_LPC_PINSEL(J15_SER_PORT, J15_SER_PIN, 0); // GPIO
  MIOS32_SYS_LPC_PINDIR(J15_SER_PORT, J15_SER_PIN, 1); // output
  MIOS32_SYS_LPC_PINMODE_OD(J15_SER_PORT, J15_SER_PIN, mode ? 1 : 0); // open drain?

  MIOS32_SYS_LPC_PINSEL(J15_E1_PORT, J15_E1_PIN, 0); // GPIO
  MIOS32_SYS_LPC_PINDIR(J15_E1_PORT, J15_E1_PIN, 1); // output
  MIOS32_SYS_LPC_PINMODE_OD(J15_E1_PORT, J15_E1_PIN, mode ? 1 : 0); // open drain?

  MIOS32_SYS_LPC_PINSEL(J15_E2_PORT, J15_E2_PIN, 0); // GPIO
  MIOS32_SYS_LPC_PINDIR(J15_E2_PORT, J15_E2_PIN, 1); // output
  MIOS32_SYS_LPC_PINMODE_OD(J15_E2_PORT, J15_E2_PIN, mode ? 1 : 0); // open drain?

  MIOS32_SYS_LPC_PINSEL(J15_RW_PORT, J15_RW_PIN, 0); // GPIO
  MIOS32_SYS_LPC_PINDIR(J15_RW_PORT, J15_RW_PIN, 1); // output
  MIOS32_SYS_LPC_PINMODE_OD(J15_RW_PORT, J15_RW_PIN, mode ? 1 : 0); // open drain?

  // configure "busy" input with pull-up
  MIOS32_SYS_LPC_PINSEL(J15_D7_PORT, J15_D7_PIN, 0); // GPIO
  MIOS32_SYS_LPC_PINMODE(J15_D7_PORT, J15_D7_PIN, 0); // pull-up enabled
  MIOS32_SYS_LPC_PINDIR(J15_D7_PORT, J15_D7_PIN, 0); // input

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
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_DATAOUT(data & 0x40); // D6
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_DATAOUT(data & 0x20); // D5
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_DATAOUT(data & 0x10); // D4
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_DATAOUT(data & 0x08); // D3
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_DATAOUT(data & 0x04); // D2
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_DATAOUT(data & 0x02); // D1
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_SCLK_1;
  J15_PIN_SERLCD_DATAOUT(data & 0x01); // D0
  J15_PIN_SERLCD_SCLK_0; // setup delay
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
  J15_PIN_SERLCD_SCLK_0;
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
#if 1
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
#if 1
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
