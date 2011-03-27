// $Id$
//! \defgroup MIOS32_AIN
//!
//! AIN driver for MIOS32
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

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_AIN)


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static s32 (*service_prepare_callback)(void);



/////////////////////////////////////////////////////////////////////////////
// Local constants
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//! Initializes AIN driver
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_AIN_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // disable service prepare callback function
  service_prepare_callback = NULL;

  return -1; // not supported yet
}


/////////////////////////////////////////////////////////////////////////////
//! Installs an optional "Service Prepare" callback function, which is called
//! before all ADC channels are scanned.
//!
//! It is useful to switch additional multiplexers, to reconfigure ADC
//! pins of touch panels, etc.
//!
//! The scan will be started if the callback function returns 0
//!
//! The scan will be skipped if the callback function returns a value >= 1
//! so that it is possible to insert setup times while switching analog inputs.
//!
//! An usage example can be found unter $MIOS32_PATH/apps/examples/dog_g_touchpanel
//!
//! \param[in] *_callback_service_prepare pointer to callback function:<BR>
//! \code
//!    s32 AIN_ServicePrepare(void);
//! \endcode
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_AIN_ServicePrepareCallback_Init(void *_service_prepare_callback)
{
  service_prepare_callback = _service_prepare_callback;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Returns value of an AIN Pin
//! \param[in] pin number
//! \return AIN pin value - resolution depends on the selected MIOS32_AIN_OVERSAMPLING_RATE!
//! \return -1 if pin doesn't exist
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_AIN_PinGet(u32 pin)
{
#if !MIOS32_AIN_CHANNEL_MASK
  return -1; // no analog input selected
#else
  return -1; // not supported yet
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Checks for pin changes, and calls given callback function with following parameters on pin changes:
//! \code
//!   void AIN_NotifyChanged(u32 pin, u16 value)
//! \endcode
//! \param[in] _callback pointer to callback function
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_AIN_Handler(void *_callback)
{
  // no callback function?
  if( _callback == NULL )
    return -1;

  return -1; // not supported yet
}

//! \}

#endif /* MIOS32_DONT_USE_AIN */
