// $Id$
/*
 * Header file for Keyboard handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of keyboards handled in parallel
#define KEYBOARD_NUM 2


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


typedef struct {

  u16 enabled_ports;
  u8  midi_chn;
  u8  note_offset;

  u8  dout_sr1;
  u8  dout_sr2;
  u8  din_sr1;
  u8  din_sr2;

  u8  scan_velocity:1;

  u16 delay_fastest;
  u16 delay_slowest;
} keyboard_config_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 KEYBOARD_Init(u32 mode);

extern s32 KEYBOARD_VerboseLevelSet(u8 mode);
extern u8  KEYBOARD_VerboseLevelGet(void);

extern s32 KEYBOARD_ConnectedNumSet(u8 num);
extern u8  KEYBOARD_ConnectedNumGet(void);

extern void KEYBOARD_SRIO_ServicePrepare(void);
extern void KEYBOARD_SRIO_ServiceFinish(void);
extern void KEYBOARD_Periodic_1mS(void);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern keyboard_config_t keyboard_config[KEYBOARD_NUM];


#endif /* _KEYBOARD_H */
