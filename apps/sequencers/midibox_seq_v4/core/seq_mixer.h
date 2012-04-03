// $Id$
/*
 * Header file for mixer routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_MIXER_H
#define _SEQ_MIXER_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of mixer map
#define SEQ_MIXER_NUM 128

// number of channels and parameters per map
#define SEQ_MIXER_NUM_CHANNELS   16
#define SEQ_MIXER_NUM_PARAMETERS 16


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  SEQ_MIXER_PAR_PORT,
  SEQ_MIXER_PAR_CHANNEL,
  SEQ_MIXER_PAR_PRG,
  SEQ_MIXER_PAR_VOLUME,   // CC#7
  SEQ_MIXER_PAR_PANORAMA, // CC#10
  SEQ_MIXER_PAR_REVERB,   // CC#91
  SEQ_MIXER_PAR_CHORUS,   // CC#93
  SEQ_MIXER_PAR_MODWHEEL, // CC#01
  SEQ_MIXER_PAR_CC1,
  SEQ_MIXER_PAR_CC2,
  SEQ_MIXER_PAR_CC3,
  SEQ_MIXER_PAR_CC4,
  SEQ_MIXER_PAR_CC1_NUM,
  SEQ_MIXER_PAR_CC2_NUM,
  SEQ_MIXER_PAR_CC3_NUM,
  SEQ_MIXER_PAR_CC4_NUM
} seq_mixer_par_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MIXER_Init(u32 mode);

extern char *SEQ_MIXER_MapNameGet(void);

extern s32 SEQ_MIXER_NumGet(void);
extern s32 SEQ_MIXER_NumSet(u8 map);

extern s32 SEQ_MIXER_Set(u8 chn, seq_mixer_par_t par, u8 value);
extern s32 SEQ_MIXER_Get(u8 chn, seq_mixer_par_t par);

extern s32 SEQ_MIXER_Send(u8 chn, seq_mixer_par_t par);
extern s32 SEQ_MIXER_SendAll(void);
extern s32 SEQ_MIXER_SendAllByChannel(u8 chn);

extern s32 SEQ_MIXER_Clear(void);

extern s32 SEQ_MIXER_Load(u8 map);
extern s32 SEQ_MIXER_Save(u8 map);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// should only be directly accessed by SEQ_FILE_M, remaining functions should
// use SEQ_MIXER_Get/Set
extern u8 seq_mixer_value[SEQ_MIXER_NUM_CHANNELS][SEQ_MIXER_NUM_PARAMETERS];
extern char seq_mixer_map_name[21];

#endif /* _SEQ_MIXER_H */
