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

#define KEYBOARD_TYPE_NOVEL16x16        0
#define KEYBOARD_TYPE_NOVEL16x8         1
#define KEYBOARD_TYPE_NOVEL8x8          2
#define KEYBOARD_TYPE_VEL16x16          3
#define KEYBOARD_TYPE_VEL16x8           4
#define KEYBOARD_TYPE_VEL8x8            5
#define KEYBOARD_TYPE_KORG_MICROKONTROL 6


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 type;
  u8 midi_chn;
  u16 enabled_ports;

  u8 dout_sr1;
  u8 dout_sr2;
  u8 din_sr1;
  u8 din_sr2;

  u16 delay_fastest;
  u16 delay_slowest;
} keyboard_config_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 KEYBOARD_Init(u32 mode);

extern s32 KEYBOARD_VerboseLevelSet(u8 mode);
extern u8 KEYBOARD_VerboseLevelGet(void);

extern void KEYBOARD_SRIO_ServicePrepare(void);
extern void KEYBOARD_SRIO_ServiceFinish(void);
extern void KEYBOARD_Periodic_1mS(void);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern keyboard_config_t keyboard_config[KEYBOARD_NUM];


#endif /* _KEYBOARD_H */
