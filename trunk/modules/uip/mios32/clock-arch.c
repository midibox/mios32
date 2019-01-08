// $Id$
/*
 * Implementation of architecture-specific clock functionality
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <mios32.h>
#include "clock-arch.h"

static clock_time_t clock;

// TMP
// has to be called each mS
clock_time_t clock_time_tick(void)
{
  return ++clock;
}


/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
  return clock; // TODO - take value from RTC
}
/*---------------------------------------------------------------------------*/
