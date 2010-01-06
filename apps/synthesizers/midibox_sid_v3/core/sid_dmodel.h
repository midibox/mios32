// $Id$
/*
 * Header file for drum models
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SID_DMODEL_H
#define _SID_DMODEL_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define SID_DMODEL_NUM 20

/////////////////////////////////////////////////////////////////////////////
// Global types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  SID_DMODEL_WT = 0, // wavetable based model.    PAR1: GL offset, PAR2: Speed offset
} sid_dmodel_type_t ;

typedef struct sid_dmodel_t {
  char name[5];
  u8 type; // sid_dmodel_type_t
  u8 base_note;
  u8 gatelength;
  u8 waveform;
  u8 pulsewidth;
  u8 wt_speed;
  u8 wt_loop;
  u8 *wavetable;
} sid_dmodel_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern const sid_dmodel_t sid_dmodel[SID_DMODEL_NUM];

#endif /* _SID_DMODEL_H */
