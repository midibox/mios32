// $Id$
/*
 * Header file for MIDIbox CV V2 Mapping functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBCV_MAP_H
#define _MBCV_MAP_H

#include <aout.h>

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of interfaces (taken over from AOUT driver)
#define MBCV_MAP_NUM_IF AOUT_NUM_IF

// selects V/Oct, Hz/V, Inv.
#define MBCV_MAP_NUM_CURVES 3

// selects calibration mode
#define MBCV_MAP_NUM_CALI_MODES AOUT_NUM_CALI_MODES


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

extern s32 MBCV_MAP_Init(u32 mode);

extern s32 MBCV_MAP_IfSet(aout_if_t if_type);
extern aout_if_t MBCV_MAP_IfGet(void);
extern const char* MBCV_MAP_IfNameGet(aout_if_t if_type);

extern s32 MBCV_MAP_CaliModeSet(u8 cv, aout_cali_mode_t mode);
extern aout_cali_mode_t MBCV_MAP_CaliModeGet(void);
extern const char* MBCV_MAP_CaliNameGet(void);

extern s32 MBCV_MAP_Update(void);

extern s32 MBCV_MAP_ResetAllChannels(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u8 mbcv_map_gate_sr;
extern u8 mbcv_map_din_sync_sr;

#ifdef __cplusplus
}
#endif

#endif /* _MBCV_MAP_H */
