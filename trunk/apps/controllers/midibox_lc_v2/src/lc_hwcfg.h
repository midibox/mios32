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

#ifndef _LC_HWCFG_H
#define _LC_HWCFG_H


/////////////////////////////////////////////////////////////////////////////
// This defition is normaly specified in make.bat to switch between 
// the common "MIDIbox LC" hardware, and the "MIDIbox SEQ" hardware
/////////////////////////////////////////////////////////////////////////////
#ifndef MBSEQ_HARDWARE_OPTION
#define MBSEQ_HARDWARE_OPTION 0
#endif


// define number of encoders
#if MBSEQ_HARDWARE_OPTION
# define LC_HWCFG_ENC_NUM 17
#else
# define LC_HWCFG_ENC_NUM 9
#endif


/////////////////////////////////////////////////////////////////////////////
// Remaining definitions are done in lc_hwcfg.c (and can be changed during runtime)
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// structs
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 initial_page_clcd;
  u8 initial_page_glcd;
} lc_hwcfg_display_cfg_t;

typedef struct {
  u8 cathodes_ledrings_sr;
  u8 cathodes_meters_sr;
  u8 anodes_sr1;
  u8 anodes_sr2;
} lc_hwcfg_ledrings_t;

typedef struct {
  u8 common_anode;
  u8 segments_sr1;
  u8 select_sr1;
  u8 segments_sr2;
  u8 select_sr2;
} lc_hwcfg_leddigits_t;

typedef struct {
  u8 enabled;
  u8 on_evnt[3];
  u8 off_evnt[3];
} lc_hwcfg_gpc_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 LC_HWCFG_Init(u32 mode);

extern s32 LC_HWCFG_TerminalHelp(void *_output_function);
extern s32 LC_HWCFG_TerminalParseLine(char *input, void *_output_function);
extern s32 LC_HWCFG_TerminalPrintConfig(void *_output_function);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

extern mios32_enc_config_t lc_hwcfg_encoders[LC_HWCFG_ENC_NUM];

extern u8 lc_hwcfg_verbose_level;

extern u8 lc_hwcfg_emulation_id;
extern lc_hwcfg_display_cfg_t lc_hwcfg_display_cfg;

extern lc_hwcfg_ledrings_t lc_hwcfg_ledrings;
extern lc_hwcfg_leddigits_t lc_hwcfg_leddigits;
extern lc_hwcfg_gpc_t lc_hwcfg_gpc;

#endif /* _LC_HWCFG_H */
