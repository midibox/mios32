// $Id$
/*
 * MIDIbox LC V2
 * Hardware Configuration Layer
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

#include "app.h"
#include "lc_hwcfg.h"


/////////////////////////////////////////////////////////////////////////////
// Encoder Configuration
/////////////////////////////////////////////////////////////////////////////
const mios32_enc_config_t lc_hwcfg_encoders[ENC_NUM] = {
#if MBSEQ_HARDWARE_OPTION
  { .cfg.type=DETENTED2, .cfg.speed=ENC_JOGWHEEL_SPEED_MODE, .cfg.speed_par=ENC_JOGWHEEL_SPEED_PAR, .cfg.sr=1, .cfg.pos=0 },

  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=5, .cfg.pos=0 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=5, .cfg.pos=2 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=5, .cfg.pos=4 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=5, .cfg.pos=6 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=6, .cfg.pos=0 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=6, .cfg.pos=2 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=6, .cfg.pos=4 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=6, .cfg.pos=6 },

  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=8, .cfg.pos=0 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=8, .cfg.pos=2 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=8, .cfg.pos=4 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=8, .cfg.pos=6 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=9, .cfg.pos=0 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=9, .cfg.pos=2 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=9, .cfg.pos=4 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=9, .cfg.pos=6 },

#else
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_JOGWHEEL_SPEED_MODE, .cfg.speed_par=ENC_JOGWHEEL_SPEED_PAR, .cfg.sr=15, .cfg.pos=0 },

  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=13, .cfg.pos=0 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=13, .cfg.pos=2 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=13, .cfg.pos=4 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=13, .cfg.pos=6 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=14, .cfg.pos=0 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=14, .cfg.pos=2 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=14, .cfg.pos=4 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=14, .cfg.pos=6 },
#endif
};



/////////////////////////////////////////////////////////////////////////////
// This function initializes the HW config layer
/////////////////////////////////////////////////////////////////////////////
s32 LC_HWCFG_Init(u32 mode)
{
  return 0; // no error
}
