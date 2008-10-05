// $Id$
/*
 * Header file for SRIO Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_SRIO_H
#define _MIOS32_SRIO_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// 16 should be maximum, more registers would require buffers at the SCLK/RCLK lines
// and probably also a lower scan frequency
// The number of SRs can be optionally overruled from the local mios32_config.h file
#ifndef MIOS32_SRIO_NUM_SR
#define MIOS32_SRIO_NUM_SR 16
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_SRIO_Init(u32 mode);

extern s32 MIOS32_SRIO_ScanStart(void *notify_hook);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern volatile u8 mios32_srio_dout[MIOS32_SRIO_NUM_SR];
extern volatile u8 mios32_srio_din[MIOS32_SRIO_NUM_SR];
extern volatile u8 mios32_srio_din_changed[MIOS32_SRIO_NUM_SR];


#endif /* _MIOS32_SRIO_H */
