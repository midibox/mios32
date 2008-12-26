// $Id$
//! \defgroup MIOS32_IRQ
//!
//! System Specific IRQ Enable/Disable routines
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
#if !defined(MIOS32_DONT_USE_IRQ)


// the nesting counter ensures, that interrupts won't be enabled so long
// nested functions disable them
static u32 nested_ctr;


/////////////////////////////////////////////////////////////////////////////
//! This function disables all interrupts (nested)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IRQ_Disable(void)
{
  __asm volatile ( \
		  "	mov r0, #1	\n" \
		  "	msr primask, r0	\n" \
		  :::"r0"	 \
		  );

  ++nested_ctr;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function enables all interrupts (nested)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IRQ_Enable(void)
{
  --nested_ctr;

  if( nested_ctr == 0 ) {
    __asm volatile ( \
		    "	mov r0, #0	\n" \
		    "	msr primask, r0	\n" \
		    :::"r0"		    \
		    );
  }

  return 0; // no error
}

//! \}

#endif /* MIOS32_DONT_USE_IRQ */

