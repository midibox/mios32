// $Id$
//! \defgroup MIOS32_IRQ
//!
//! System Specific IRQ Enable/Disable routines
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
#if !defined(MIOS32_DONT_USE_IRQ)


// the nesting counter ensures, that interrupts won't be enabled as long as
// nested functions disable them
static u32 nested_ctr;

// stored priority level before IRQ has been disabled (important for co-existence with vPortEnterCritical)
static u32 prev_primask;


/////////////////////////////////////////////////////////////////////////////
//! This function disables all interrupts (nested)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IRQ_Disable(void)
{
  // get current priority if nested level == 0
  if( !nested_ctr ) {
    __asm volatile (			   \
		    "	mrs %0, primask\n" \
		    : "=r" (prev_primask)  \
		    );
  }

  // disable interrupts
  __asm volatile ( \
		  "	mov r0, #1     \n" \
		  "	msr primask, r0\n" \
		  :::"r0"	 \
		  );

  ++nested_ctr;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function enables all interrupts (nested)
//! \return < 0 on errors
//! \return -1 on nesting errors (MIOS32_IRQ_Disable() hasn't been called before)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IRQ_Enable(void)
{
  // check for nesting error
  if( nested_ctr == 0 )
    return -1; // nesting error

  // decrease nesting level
  --nested_ctr;

  // set back previous priority once nested level reached 0 again
  if( nested_ctr == 0 ) {
    __asm volatile ( \
		    "	msr primask, %0\n" \
		    :: "r" (prev_primask)  \
		    );
  }

  return 0; // no error
}

//! \}

#endif /* MIOS32_DONT_USE_IRQ */

