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


/////////////////////////////////////////////////////////////////////////////
//! This function installs an interrupt service.
//! \param[in] IRQn the interrupt number as defined in the CMSIS (e.g. CAN_IRQn)
//! \param[in] priority the priority from 0..15 - than lower the value, than higher the priority.\n
//! Please prefer the usage of MIOS32_IRQ_PRIO_LOW .. MID .. HIGH .. HIGHEST
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IRQ_Install(u8 IRQn, u8 priority)
{
  // no check for IRQn as it's device dependent

  if( priority >= 16 )
    return -1; // invalid priority

  u32 tmppriority = (0x700 - ((SCB->AIRCR) & (uint32_t)0x700)) >> 8;
  u32 tmppre = (4 - tmppriority);
  tmppriority = priority << tmppre;
  tmppriority = tmppriority << 4;
  NVIC->IP[IRQn] = tmppriority;

  NVIC_EnableIRQ(IRQn);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function deinstalls an interrupt service.
//! \param[in] IRQn the interrupt number as defined in the CMSIS (e.g. CAN_IRQn)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IRQ_DeInstall(u8 IRQn)
{
  NVIC_DisableIRQ(IRQn);

  return 0; // no error
}

//! \}

#endif /* MIOS32_DONT_USE_IRQ */

