// $Id: mbng_dio.c 2035 2014-08-18 20:16:47Z tk $
//! \defgroup MBNG_DIO
//! Digital IO access functions for MIDIbox NG
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2014 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
//! Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "app.h"
#include "mbng_dio.h"
#include "mbng_patch.h"


/////////////////////////////////////////////////////////////////////////////
//! local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! This function initializes the DIO handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DIO_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the name of a port
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_DIO_PortNameGet(u8 port)
{
#if MBNG_PATCH_NUM_DIO > 0
  const char *port_name_table[MBNG_PATCH_NUM_DIO] = {
  "J10A",
  "J10B"
#if MBNG_PATCH_NUM_DIO > 2
# error "Please add new DIO port name here!"
#endif
  };

  if( port < MBNG_PATCH_NUM_DIO )
    return port_name_table[port];
#endif

  return "???";
}


/////////////////////////////////////////////////////////////////////////////
//! Initialises a DIO port
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DIO_PortInit(u8 port, mios32_board_pin_mode_t mode)
{
#if MBNG_PATCH_NUM_DIO == 0
  return -1; // not supported
#else
  switch( port ) {
  case 0: {
    int i;
    for(i=0; i<8; ++i)
      MIOS32_BOARD_J10_PinInit(i, mode);
  } break;

  case 1: {
    int i;
#if defined(MIOS32_FAMILY_STM32F4xx)
    for(i=8; i<16; ++i)
      MIOS32_BOARD_J10_PinInit(i, mode);
#elif defined(MIOS32_FAMILY_LPC17xx)
    for(i=0; i<8; ++i)
      MIOS32_BOARD_J28_PinInit(i, mode);
#else
# error "Please adapt J10B port assignments here"
#endif
  } break;

  default:
    return -1; // invalid port
#if MBNG_PATCH_NUM_DIO > 2
# error "Add pin init function here"
#endif
  }

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the 8bit value of a DIO port
/////////////////////////////////////////////////////////////////////////////
u8 MBNG_DIO_PortGet(u8 port)
{
#if MBNG_PATCH_NUM_DIO == 0
  return -1; // not supported
#else
  switch( port ) {
  case 0: {
#if defined(MIOS32_FAMILY_STM32F4xx)
    return MIOS32_BOARD_J10A_Get();
#elif defined(MIOS32_FAMILY_LPC17xx)
    return MIOS32_BOARD_J10_Get();
#else
# error "Please adapt J10 port assignments here"
#endif
  } break;

  case 1: {
#if defined(MIOS32_FAMILY_STM32F4xx)
    return MIOS32_BOARD_J10B_Get();
#elif defined(MIOS32_FAMILY_LPC17xx)
    return MIOS32_BOARD_J28_Get();
#else
# error "Please adapt J10 port assignments here"
#endif
  } break;

#if MBNG_PATCH_NUM_DIO > 2
# error "Add pin get function here"
#endif
  }

  return 0xff; // invalid port
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Sets the 8bit value of a DIO port
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DIO_PortSet(u8 port, u8 value)
{
#if MBNG_PATCH_NUM_DIO == 0
  return -1; // not supported
#else
  switch( port ) {
  case 0: {
#if defined(MIOS32_FAMILY_STM32F4xx)
    MIOS32_BOARD_J10A_Set(value);
#elif defined(MIOS32_FAMILY_LPC17xx)
    MIOS32_BOARD_J10_Set(value);
#else
# error "Please adapt J10 port assignments here"
#endif
  } break;

  case 1: {
#if defined(MIOS32_FAMILY_STM32F4xx)
    MIOS32_BOARD_J10B_Set(value);
#elif defined(MIOS32_FAMILY_LPC17xx)
    MIOS32_BOARD_J28_Set(value);
#else
# error "Please adapt J10 port assignments here"
#endif
  } break;

  default:
    return -1; // invalid port
#if MBNG_PATCH_NUM_DIO > 2
# error "Add pin set function here"
#endif
  }

  return 0; // no error
#endif
}

//! \}
