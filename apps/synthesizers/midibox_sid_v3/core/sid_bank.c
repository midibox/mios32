// $Id$
/*
 * MBSID Bank Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>
#include "sid_bank.h"
#include "sid_patch.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#include "sid_bank_preset_a.inc"


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SID_BANK_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Write a patch
/////////////////////////////////////////////////////////////////////////////
s32 SID_BANK_PatchWrite(sid_patch_ref_t *pr)
{
  return -1; // not supported yet
}


/////////////////////////////////////////////////////////////////////////////
// Read a patch
/////////////////////////////////////////////////////////////////////////////
s32 SID_BANK_PatchRead(sid_patch_ref_t *pr)
{
  if( pr->bank >= SID_BANK_NUM )
    return -1; // invalid bank

  if( pr->patch >= 128 )
    return -2; // invalid patch

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SID_BANK_PatchRead] SID %d reads patch %c%03d\n", pr->sid, 'A'+pr->bank, pr->patch);
#endif

  switch( pr->bank ) {
    case 0:
      memcpy(pr->p, (u8 *)&sid_bank_preset_0[pr->patch], 512);
      break;

    default:
      return -3; // no bank in ROM
  }

  return 0; // no error
}
