// $Id$
//! \defgroup MIOS32_I2S
//!
//! I2S Functions
//!
//! Not adapted to LPC17xx yet
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

// this module can be optionally *ENABLED* in a local mios32_config.h file (included from mios32.h)
#if defined(MIOS32_USE_I2S)


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static void (*buffer_reload_callback)(u32 state) = NULL;


/////////////////////////////////////////////////////////////////////////////
//! Initializes I2S interface
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_I2S_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  return -1; // not supported yet
}


/////////////////////////////////////////////////////////////////////////////
//! Starts DMA driven I2S transfers
//! \param[in] *buffer pointer to sample buffer (contains L/R halfword)
//! \param[in] len size of audio buffer
//! \param[in] _callback callback function:<BR>
//! \code
//!   void callback(u32 state)
//! \endcode
//!      called when the lower (state == 0) or upper (state == 1)
//!      range of the sample buffer has been transfered, so that it
//!      can be updated
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_I2S_Start(u32 *buffer, u16 len, void *_callback)
{
  return -1; // not supported yet
}


/////////////////////////////////////////////////////////////////////////////
//! Stops DMA driven I2S transfers
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_I2S_Stop(void)
{
  return -1; // not supported yet
}


//! \}

#endif /* MIOS32_USE_I2S */
