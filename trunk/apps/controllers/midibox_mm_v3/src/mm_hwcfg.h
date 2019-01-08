// $Id$
/*
 * Header File for Hardware Configuration
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MM_HWCFG_H
#define _MM_HWCFG_H


/////////////////////////////////////////////////////////////////////////////
// Definitions are done in mm_hwcfg.c (and can be changed during runtime)
/////////////////////////////////////////////////////////////////////////////

#define MM_HWCFG_ENC_NUM 9


/////////////////////////////////////////////////////////////////////////////
// structs
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 cathodes_ledrings_sr;
  u8 cathodes_meters_sr;
  u8 anodes_sr1;
  u8 anodes_sr2;
} mm_hwcfg_ledrings_t;

typedef struct {
  u8 segment_a_sr;
  u8 segment_b_sr;
} mm_hwcfg_leddigits_t;

typedef struct {
  u8 enabled;
  u8 on_evnt[3];
  u8 off_evnt[3];
} mm_hwcfg_gpc_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MM_HWCFG_Init(u32 mode);

extern s32 MM_HWCFG_TerminalHelp(void *_output_function);
extern s32 MM_HWCFG_TerminalParseLine(char *input, void *_output_function);
extern s32 MM_HWCFG_TerminalPrintConfig(void *_output_function);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

extern mios32_enc_config_t mm_hwcfg_encoders[MM_HWCFG_ENC_NUM];

extern u8 mm_hwcfg_verbose_level;

extern u8 mm_hwcfg_midi_channel;

extern mm_hwcfg_ledrings_t mm_hwcfg_ledrings;
extern mm_hwcfg_leddigits_t mm_hwcfg_leddigits;
extern mm_hwcfg_gpc_t mm_hwcfg_gpc;

#endif /* _MM_HWCFG_H */
