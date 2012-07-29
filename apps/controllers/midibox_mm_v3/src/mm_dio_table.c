// $Id$
/*
 * MIDIbox MM V3
 * IO Configuration
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
#include "mm_hwcfg.h"

// all provided button/led definitions can be found in this include file:
#include "mm_dio_table.h"


// ==========================================================================
//  In this table all functions are mapped to the DIN/DOUT pins
//  Every button/LED combination has it's own entry
//  First entry for first button and first LED
//  Second entry for second button and second LED
//
//  DIN and DOUT are working indepently, that means that you are allowed
//  to assign different functions to a button or LED of the same entry
//  number. In this way you can save some DOUT shift registers, since not
//  every function provides a response to the LED
//
//  Keep also in mind that the encoders and LED rings allocate some DIN
//  and DOUT pins, which have to be defined in main.h and mios_tables.inc
//
// IMPORTANT: this table MUST consist of 128 entries!!!
// unused lines have to be filled with "db ID_IGNORE, ID_IGNORE"
//
//  All available IDs are defined in mm_dio_table.h
// ==========================================================================
const u8 mm_dio_table_layer0[2*128] = {
  // 1st shift register
  // button             LED
  ID_REC_RDY_CHN1,      ID_REC_RDY_CHN1,
  ID_REC_RDY_CHN2,      ID_REC_RDY_CHN2,
  ID_REC_RDY_CHN3,      ID_REC_RDY_CHN3,
  ID_REC_RDY_CHN4,      ID_REC_RDY_CHN4,
  ID_REC_RDY_CHN5,      ID_REC_RDY_CHN5,
  ID_REC_RDY_CHN6,      ID_REC_RDY_CHN6,
  ID_REC_RDY_CHN7,      ID_REC_RDY_CHN7,
  ID_REC_RDY_CHN8,      ID_REC_RDY_CHN8,

  // 2nd shift register
  // button             LED
  ID_SOLO_CHN1,         ID_SOLO_CHN1,
  ID_SOLO_CHN2,         ID_SOLO_CHN2,
  ID_SOLO_CHN3,         ID_SOLO_CHN3,
  ID_SOLO_CHN4,         ID_SOLO_CHN4,
  ID_SOLO_CHN5,         ID_SOLO_CHN5,
  ID_SOLO_CHN6,         ID_SOLO_CHN6,
  ID_SOLO_CHN7,         ID_SOLO_CHN7,
  ID_SOLO_CHN8,         ID_SOLO_CHN8,

  // 3rd shift register
  // button             LED
  ID_MUTE_CHN1,         ID_MUTE_CHN1,
  ID_MUTE_CHN2,         ID_MUTE_CHN2,
  ID_MUTE_CHN3,         ID_MUTE_CHN3,
  ID_MUTE_CHN4,         ID_MUTE_CHN4,
  ID_MUTE_CHN5,         ID_MUTE_CHN5,
  ID_MUTE_CHN6,         ID_MUTE_CHN6,
  ID_MUTE_CHN7,         ID_MUTE_CHN7,
  ID_MUTE_CHN8,         ID_MUTE_CHN8,

  // 4th shift register
  // button             LED
  ID_SELECT_CHN1,       ID_SELECT_CHN1,
  ID_SELECT_CHN2,       ID_SELECT_CHN2,
  ID_SELECT_CHN3,       ID_SELECT_CHN3,
  ID_SELECT_CHN4,       ID_SELECT_CHN4,
  ID_SELECT_CHN5,       ID_SELECT_CHN5,
  ID_SELECT_CHN6,       ID_SELECT_CHN6,
  ID_SELECT_CHN7,       ID_SELECT_CHN7,
  ID_SELECT_CHN8,       ID_SELECT_CHN8,

  // 5th shift register
  // button             LED
  ID_LEFT_ARROW,        ID_IGNORE,    // (no LED supported by host)
  ID_RIGHT_ARROW,       ID_IGNORE,    // (no LED supported by host)
  ID_BANK,              ID_BANK,
  ID_GROUP,             ID_GROUP,
  ID_RECRDY_x_FUNCTA,   ID_RECRDY_x_FUNCTA,
  ID_WRITE_x_FUNCTB,    ID_WRITE_x_FUNCTB,
  ID_OTHER_x_FUNCTC,    ID_OTHER_x_FUNCTC,
  ID_IGNORE,            ID_IGNORE,    // (unused pin to keep it aligned with my MIDIbox LC)
  
  // 6th shift register
  // button             LED
  ID_FX_BYPASS_x_EFFECT1, ID_FX_BYPASS_x_EFFECT1,
  ID_SEND_MUTE_x_EFFECT2, ID_SEND_MUTE_x_EFFECT2,
  ID_PRE_POST_x_EFFECT3,ID_PRE_POST_x_EFFECT3,
  ID_SELECT_x_EFFECT4,  ID_SELECT_x_EFFECT4,
  ID_SHIFT,             ID_SHIFT,
  ID_UNDO_x_DISK,       ID_UNDO_x_DISK,
  ID_DEFAULT_x_BYPASS,  ID_DEFAULT_x_BYPASS,
  ID_ALL_x_FINE,        ID_ALL_x_FINE,

  // 7th shift register
  // button             LED
  ID_WINDOW_x_TOOLS,    ID_WINDOW_x_TOOLS,
  ID_PLUGIN_x_COMPARE,  ID_PLUGIN_x_COMPARE,
  ID_SUSPEND_x_CREATE,  ID_SUSPEND_x_CREATE,
  ID_AUTOENBL_x_MODE,   ID_AUTOENBL_x_MODE,
  ID_IGNORE,            ID_IGNORE,  // (unused pin to keep it aligned with my MIDIbox LC)
  ID_IGNORE,            ID_IGNORE,  // (unused pin to keep it aligned with my MIDIbox LC)
  ID_REWIND_x_STATUS,   ID_REWIND_x_STATUS,
  ID_FFWD_x_MONITOR,    ID_FFWD_x_MONITOR,

  // 8th shift register
  // button             LED
  ID_STOP_x_LOCATE,     ID_STOP_x_LOCATE,
  ID_PLAY_x_TRANSPORT,  ID_PLAY_x_TRANSPORT,
  ID_ENTER_x_UTILITY,   ID_ENTER_x_UTILITY,
  ID_LAST_x_ASSIGN,     ID_LAST_x_ASSIGN,
  ID_LEFT_ARROW,        ID_IGNORE,    // (again - no LED supported by host)
  ID_ESCAPE,            ID_ESCAPE,
  ID_RIGHT_ARROW,       ID_IGNORE,    // (again - no LED supported by host)
  ID_NEXT_x_CONFIGURE,  ID_NEXT_x_CONFIGURE,

  // 9th shift register
  // button             LED
  ID_MULTI_CHN1,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN2,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN3,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN4,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN5,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN6,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN7,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN8,        ID_IGNORE,    // (no LED supported by host)

  // 10th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)

  // 11th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)

  // 12th shift register
  // button             LED
  ID_MBMM_SWITCH_LAYER0,ID_MBMM_SWITCH_LAYER0,  // special function of MBMM: switch to layer 0
  ID_MBMM_SWITCH_LAYER1,ID_MBMM_SWITCH_LAYER1,  // special function of MBMM: switch to layer 1
  ID_MBMM_SWITCH_MM,    ID_MBMM_SWITCH_MM,  // special function of MBMM: switch to motormix mode
  ID_MBMM_SWITCH_GPC,   ID_MBMM_SWITCH_GPC,  // special function of MBMM: switch to GPC mode
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)

  // 13th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)

  // 14th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)

  // 15th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)

  // 16th shift register
  // button             LED
  ID_FADER_TOUCH_CHN1,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN2,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN3,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN4,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN5,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN6,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN7,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN8,  ID_IGNORE,    // (no LED supported by host)
};

// ==========================================================================
//  This table is used for a second layer which can be selected by a
//  special function button (ID_MBMM_LAYER_*, see #define section above)
//
//  Tips:
//     o mostly used button/led functions like cursor, V-pot select, etc
//       should be presented in both tables at the same place
//     o the layer select buttons should be defined at the same place!
//     o use either ID_MBMM_HOLD_LAYER1 or ID_MBMM_TOGGLE_LAYER1 to
//       select the layer
// ==========================================================================
const u8 mm_dio_table_layer1[2*128] = {
  // 1st shift register
  // button             LED
  ID_REC_RDY_CHN1,      ID_REC_RDY_CHN1,
  ID_REC_RDY_CHN2,      ID_REC_RDY_CHN2,
  ID_REC_RDY_CHN3,      ID_REC_RDY_CHN3,
  ID_REC_RDY_CHN4,      ID_REC_RDY_CHN4,
  ID_REC_RDY_CHN5,      ID_REC_RDY_CHN5,
  ID_REC_RDY_CHN6,      ID_REC_RDY_CHN6,
  ID_REC_RDY_CHN7,      ID_REC_RDY_CHN7,
  ID_REC_RDY_CHN8,      ID_REC_RDY_CHN8,

  // 2nd shift register
  // button             LED
  ID_SOLO_CHN1,         ID_SOLO_CHN1,
  ID_SOLO_CHN2,         ID_SOLO_CHN2,
  ID_SOLO_CHN3,         ID_SOLO_CHN3,
  ID_SOLO_CHN4,         ID_SOLO_CHN4,
  ID_SOLO_CHN5,         ID_SOLO_CHN5,
  ID_SOLO_CHN6,         ID_SOLO_CHN6,
  ID_SOLO_CHN7,         ID_SOLO_CHN7,
  ID_SOLO_CHN8,         ID_SOLO_CHN8,

  // 3rd shift register
  // button             LED
  ID_MUTE_CHN1,         ID_MUTE_CHN1,
  ID_MUTE_CHN2,         ID_MUTE_CHN2,
  ID_MUTE_CHN3,         ID_MUTE_CHN3,
  ID_MUTE_CHN4,         ID_MUTE_CHN4,
  ID_MUTE_CHN5,         ID_MUTE_CHN5,
  ID_MUTE_CHN6,         ID_MUTE_CHN6,
  ID_MUTE_CHN7,         ID_MUTE_CHN7,
  ID_MUTE_CHN8,         ID_MUTE_CHN8,

  // 4th shift register
  // button             LED
  ID_SELECT_CHN1,       ID_SELECT_CHN1,
  ID_SELECT_CHN2,       ID_SELECT_CHN2,
  ID_SELECT_CHN3,       ID_SELECT_CHN3,
  ID_SELECT_CHN4,       ID_SELECT_CHN4,
  ID_SELECT_CHN5,       ID_SELECT_CHN5,
  ID_SELECT_CHN6,       ID_SELECT_CHN6,
  ID_SELECT_CHN7,       ID_SELECT_CHN7,
  ID_SELECT_CHN8,       ID_SELECT_CHN8,

  // 5th shift register
  // button             LED
  ID_LEFT_ARROW,        ID_IGNORE,    // (no LED supported by host)
  ID_RIGHT_ARROW,       ID_IGNORE,    // (no LED supported by host)
  ID_BANK,              ID_BANK,
  ID_GROUP,             ID_GROUP,
  ID_RECRDY_x_FUNCTA,   ID_RECRDY_x_FUNCTA,
  ID_WRITE_x_FUNCTB,    ID_WRITE_x_FUNCTB,
  ID_OTHER_x_FUNCTC,    ID_OTHER_x_FUNCTC,
  ID_IGNORE,            ID_IGNORE,    // (unused pin to keep it aligned with my MIDIbox LC)
  
  // 6th shift register
  // button             LED
  ID_FX_BYPASS_x_EFFECT1, ID_FX_BYPASS_x_EFFECT1,
  ID_SEND_MUTE_x_EFFECT2, ID_SEND_MUTE_x_EFFECT2,
  ID_PRE_POST_x_EFFECT3,ID_PRE_POST_x_EFFECT3,
  ID_SELECT_x_EFFECT4,  ID_SELECT_x_EFFECT4,
  ID_SHIFT,             ID_SHIFT,
  ID_UNDO_x_DISK,       ID_UNDO_x_DISK,
  ID_DEFAULT_x_BYPASS,  ID_DEFAULT_x_BYPASS,
  ID_ALL_x_FINE,        ID_ALL_x_FINE,

  // 7th shift register
  // button             LED
  ID_WINDOW_x_TOOLS,    ID_WINDOW_x_TOOLS,
  ID_PLUGIN_x_COMPARE,  ID_PLUGIN_x_COMPARE,
  ID_SUSPEND_x_CREATE,  ID_SUSPEND_x_CREATE,
  ID_AUTOENBL_x_MODE,   ID_AUTOENBL_x_MODE,
  ID_IGNORE,            ID_IGNORE,  // (unused pin to keep it aligned with my MIDIbox LC)
  ID_IGNORE,            ID_IGNORE,  // (unused pin to keep it aligned with my MIDIbox LC)
  ID_REWIND_x_STATUS,   ID_REWIND_x_STATUS,
  ID_FFWD_x_MONITOR,    ID_FFWD_x_MONITOR,

  // 8th shift register
  // button             LED
  ID_STOP_x_LOCATE,     ID_STOP_x_LOCATE,
  ID_PLAY_x_TRANSPORT,  ID_PLAY_x_TRANSPORT,
  ID_ENTER_x_UTILITY,   ID_ENTER_x_UTILITY,
  ID_LAST_x_ASSIGN,     ID_LAST_x_ASSIGN,
  ID_LEFT_ARROW,        ID_IGNORE,    // (again - no LED supported by host)
  ID_ESCAPE,            ID_ESCAPE,
  ID_RIGHT_ARROW,       ID_IGNORE,    // (again - no LED supported by host)
  ID_NEXT_x_CONFIGURE,  ID_NEXT_x_CONFIGURE,

  // 9th shift register
  // button             LED
  ID_MULTI_CHN1,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN2,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN3,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN4,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN5,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN6,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN7,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN8,        ID_IGNORE,    // (no LED supported by host)

  // 10th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)

  // 11th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)

  // 12th shift register
  // button             LED
  ID_MBMM_SWITCH_LAYER0,ID_MBMM_SWITCH_LAYER0,  // special function of MBMM: switch to layer 0
  ID_MBMM_SWITCH_LAYER1,ID_MBMM_SWITCH_LAYER1,  // special function of MBMM: switch to layer 1
  ID_MBMM_SWITCH_MM,    ID_MBMM_SWITCH_MM,  // special function of MBMM: switch to motormix mode
  ID_MBMM_SWITCH_GPC,   ID_MBMM_SWITCH_GPC,  // special function of MBMM: switch to GPC mode
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)

  // 13th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)

  // 14th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)

  // 15th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)

  // 16th shift register
  // button             LED
  ID_FADER_TOUCH_CHN1,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN2,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN3,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN4,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN5,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN6,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN7,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN8,  ID_IGNORE,    // (no LED supported by host)
};

// ==========================================================================
//  This table is used for a third layer which can be selected by a
//  special function button (ID_MBMM_LAYER_*, see #define section above)
//
//  Tips:
//     o mostly used button/led functions like cursor, V-pot select, etc
//       should be presented in both tables at the same place
//     o the layer select buttons should be defined at the same place!
//     o use either ID_MBMM_HOLD_LAYER2 or ID_MBMM_TOGGLE_LAYER2 to
//       select the layer
// ==========================================================================
const u8 mm_dio_table_layer2[2*128] = {
  // 1st shift register
  // button             LED
  ID_REC_RDY_CHN1,      ID_REC_RDY_CHN1,
  ID_REC_RDY_CHN2,      ID_REC_RDY_CHN2,
  ID_REC_RDY_CHN3,      ID_REC_RDY_CHN3,
  ID_REC_RDY_CHN4,      ID_REC_RDY_CHN4,
  ID_REC_RDY_CHN5,      ID_REC_RDY_CHN5,
  ID_REC_RDY_CHN6,      ID_REC_RDY_CHN6,
  ID_REC_RDY_CHN7,      ID_REC_RDY_CHN7,
  ID_REC_RDY_CHN8,      ID_REC_RDY_CHN8,

  // 2nd shift register
  // button             LED
  ID_SOLO_CHN1,         ID_SOLO_CHN1,
  ID_SOLO_CHN2,         ID_SOLO_CHN2,
  ID_SOLO_CHN3,         ID_SOLO_CHN3,
  ID_SOLO_CHN4,         ID_SOLO_CHN4,
  ID_SOLO_CHN5,         ID_SOLO_CHN5,
  ID_SOLO_CHN6,         ID_SOLO_CHN6,
  ID_SOLO_CHN7,         ID_SOLO_CHN7,
  ID_SOLO_CHN8,         ID_SOLO_CHN8,

  // 3rd shift register
  // button             LED
  ID_MUTE_CHN1,         ID_MUTE_CHN1,
  ID_MUTE_CHN2,         ID_MUTE_CHN2,
  ID_MUTE_CHN3,         ID_MUTE_CHN3,
  ID_MUTE_CHN4,         ID_MUTE_CHN4,
  ID_MUTE_CHN5,         ID_MUTE_CHN5,
  ID_MUTE_CHN6,         ID_MUTE_CHN6,
  ID_MUTE_CHN7,         ID_MUTE_CHN7,
  ID_MUTE_CHN8,         ID_MUTE_CHN8,

  // 4th shift register
  // button             LED
  ID_SELECT_CHN1,       ID_SELECT_CHN1,
  ID_SELECT_CHN2,       ID_SELECT_CHN2,
  ID_SELECT_CHN3,       ID_SELECT_CHN3,
  ID_SELECT_CHN4,       ID_SELECT_CHN4,
  ID_SELECT_CHN5,       ID_SELECT_CHN5,
  ID_SELECT_CHN6,       ID_SELECT_CHN6,
  ID_SELECT_CHN7,       ID_SELECT_CHN7,
  ID_SELECT_CHN8,       ID_SELECT_CHN8,

  // 5th shift register
  // button             LED
  ID_LEFT_ARROW,        ID_IGNORE,    // (no LED supported by host)
  ID_RIGHT_ARROW,       ID_IGNORE,    // (no LED supported by host)
  ID_BANK,              ID_BANK,
  ID_GROUP,             ID_GROUP,
  ID_RECRDY_x_FUNCTA,   ID_RECRDY_x_FUNCTA,
  ID_WRITE_x_FUNCTB,    ID_WRITE_x_FUNCTB,
  ID_OTHER_x_FUNCTC,    ID_OTHER_x_FUNCTC,
  ID_IGNORE,            ID_IGNORE,    // (unused pin to keep it aligned with my MIDIbox LC)
  
  // 6th shift register
  // button             LED
  ID_FX_BYPASS_x_EFFECT1, ID_FX_BYPASS_x_EFFECT1,
  ID_SEND_MUTE_x_EFFECT2, ID_SEND_MUTE_x_EFFECT2,
  ID_PRE_POST_x_EFFECT3,ID_PRE_POST_x_EFFECT3,
  ID_SELECT_x_EFFECT4,  ID_SELECT_x_EFFECT4,
  ID_SHIFT,             ID_SHIFT,
  ID_UNDO_x_DISK,       ID_UNDO_x_DISK,
  ID_DEFAULT_x_BYPASS,  ID_DEFAULT_x_BYPASS,
  ID_ALL_x_FINE,        ID_ALL_x_FINE,

  // 7th shift register
  // button             LED
  ID_WINDOW_x_TOOLS,    ID_WINDOW_x_TOOLS,
  ID_PLUGIN_x_COMPARE,  ID_PLUGIN_x_COMPARE,
  ID_SUSPEND_x_CREATE,  ID_SUSPEND_x_CREATE,
  ID_AUTOENBL_x_MODE,   ID_AUTOENBL_x_MODE,
  ID_IGNORE,            ID_IGNORE,  // (unused pin to keep it aligned with my MIDIbox LC)
  ID_IGNORE,            ID_IGNORE,  // (unused pin to keep it aligned with my MIDIbox LC)
  ID_REWIND_x_STATUS,   ID_REWIND_x_STATUS,
  ID_FFWD_x_MONITOR,    ID_FFWD_x_MONITOR,

  // 8th shift register
  // button             LED
  ID_STOP_x_LOCATE,     ID_STOP_x_LOCATE,
  ID_PLAY_x_TRANSPORT,  ID_PLAY_x_TRANSPORT,
  ID_ENTER_x_UTILITY,   ID_ENTER_x_UTILITY,
  ID_LAST_x_ASSIGN,     ID_LAST_x_ASSIGN,
  ID_LEFT_ARROW,        ID_IGNORE,    // (again - no LED supported by host)
  ID_ESCAPE,            ID_ESCAPE,
  ID_RIGHT_ARROW,       ID_IGNORE,    // (again - no LED supported by host)
  ID_NEXT_x_CONFIGURE,  ID_NEXT_x_CONFIGURE,

  // 9th shift register
  // button             LED
  ID_MULTI_CHN1,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN2,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN3,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN4,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN5,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN6,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN7,        ID_IGNORE,    // (no LED supported by host)
  ID_MULTI_CHN8,        ID_IGNORE,    // (no LED supported by host)

  // 10th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)

  // 11th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)

  // 12th shift register
  // button             LED
  ID_MBMM_SWITCH_LAYER0,ID_MBMM_SWITCH_LAYER0,  // special function of MBMM: switch to layer 0
  ID_MBMM_SWITCH_LAYER1,ID_MBMM_SWITCH_LAYER1,  // special function of MBMM: switch to layer 1
  ID_MBMM_SWITCH_MM,    ID_MBMM_SWITCH_MM,  // special function of MBMM: switch to motormix mode
  ID_MBMM_SWITCH_GPC,   ID_MBMM_SWITCH_GPC,  // special function of MBMM: switch to GPC mode
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup I've a spare button here)

  // 13th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)

  // 14th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)

  // 15th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,    // (spare pin in my setup)

  // 16th shift register
  // button             LED
  ID_FADER_TOUCH_CHN1,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN2,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN3,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN4,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN5,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN6,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN7,  ID_IGNORE,    // (no LED supported by host)
  ID_FADER_TOUCH_CHN8,  ID_IGNORE,    // (no LED supported by host)
};
