// $Id$
/*
 * Header file of Synth Module
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SYNTH_H
#define _SYNTH_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define SYNTH_NUM_GROUPS         1 // tmp, could be more groups (synths running in parallel) later
#define SYNTH_NUM_PHRASES       64
#define SYNTH_PHRASE_MAX_LENGTH 16

#define SYNTH_GLOBAL_PAR_DOWNSAMPLING_FACTOR 0
#define SYNTH_GLOBAL_PAR_RESOLUTION          1
#define SYNTH_GLOBAL_PAR_XOR                 2

#define SYNTH_PHONEME_PAR_PH         0
#define SYNTH_PHONEME_PAR_ENV        1
#define SYNTH_PHONEME_PAR_TONE       2
#define SYNTH_PHONEME_PAR_TYPE       3
#define SYNTH_PHONEME_PAR_PREPAUSE   4
#define SYNTH_PHONEME_PAR_AMP        5
#define SYNTH_PHONEME_PAR_NEWWORD    6
#define SYNTH_PHONEME_PAR_FLAGS      7
#define SYNTH_PHONEME_PAR_LENGTH     8
#define SYNTH_PHONEME_PAR_PITCH1     9
#define SYNTH_PHONEME_PAR_PITCH2    10
#define SYNTH_PHONEME_PAR_SOURCE_IX 11

#define SYNTH_PHRASE_PAR_LENGTH       0
#define SYNTH_PHRASE_PAR_TUNE         1


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SYNTH_Init(u32 mode);

extern s32 SYNTH_SamplePlayed(void);
extern s32 SYNTH_Tick(void);

extern s32 SYNTH_GlobalParGet(u8 par);
extern s32 SYNTH_GlobalParSet(u8 par, u8 value);

extern s32 SYNTH_PhonemeParGet(u8 num, u8 ix, u8 par);
extern s32 SYNTH_PhonemeParSet(u8 num, u8 ix, u8 par, u8 value);

extern s32 SYNTH_PhraseParGet(u8 num, u8 par);
extern s32 SYNTH_PhraseParSet(u8 num, u8 par, u8 value);

extern s32 SYNTH_PhrasePlay(u8 num, u8 velocity);
extern s32 SYNTH_PhraseStop(u8 num);
extern s32 SYNTH_PhraseIsPlayed(u8 num);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// tmp.
extern char synth_patch_name[SYNTH_NUM_GROUPS][21];

#endif /* _SYNTH_H */
