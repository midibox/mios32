// $Id$
/*
 * MIDIbox LC V2
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
#include "lc_hwcfg.h"

// all provided button/led definitions can be found in this include file:
#include "lc_dio_table.h"


// the DIO table for MIDIbox SEQ Hardware is located in a seperate file
// to improve the readability. 
#if MBSEQ_HARDWARE_OPTION
#  include "lc_dio_table_mbseq.inc"
#else

// default setup when MBSEQ Hardware option is not selected

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
const u8 lc_dio_table_layer0[2*128] = {
  // 1st shift register
  // button             LED
  ID_SELECT_CHN1,       ID_SELECT_CHN1,
  ID_SELECT_CHN2,       ID_SELECT_CHN2,
  ID_SELECT_CHN3,       ID_SELECT_CHN3,
  ID_SELECT_CHN4,       ID_SELECT_CHN4,
  ID_SELECT_CHN5,       ID_SELECT_CHN5,
  ID_SELECT_CHN6,       ID_SELECT_CHN6,
  ID_SELECT_CHN7,       ID_SELECT_CHN7,
  ID_SELECT_CHN8,       ID_SELECT_CHN8,

  // 2nd shift register
  // button             LED
  ID_REC_RDY_CHN1,      ID_REC_RDY_CHN1,
  ID_REC_RDY_CHN2,      ID_REC_RDY_CHN2,
  ID_REC_RDY_CHN3,      ID_REC_RDY_CHN3,
  ID_REC_RDY_CHN4,      ID_REC_RDY_CHN4,
  ID_REC_RDY_CHN5,      ID_REC_RDY_CHN5,
  ID_REC_RDY_CHN6,      ID_REC_RDY_CHN6,
  ID_REC_RDY_CHN7,      ID_REC_RDY_CHN7,
  ID_REC_RDY_CHN8,      ID_REC_RDY_CHN8,

  // 3rd shift register
  // button             LED
  ID_SOLO_CHN1,         ID_SOLO_CHN1,
  ID_SOLO_CHN2,         ID_SOLO_CHN2,
  ID_SOLO_CHN3,         ID_SOLO_CHN3,
  ID_SOLO_CHN4,         ID_SOLO_CHN4,
  ID_SOLO_CHN5,         ID_SOLO_CHN5,
  ID_SOLO_CHN6,         ID_SOLO_CHN6,
  ID_SOLO_CHN7,         ID_SOLO_CHN7,
  ID_SOLO_CHN8,         ID_SOLO_CHN8,

  // 4th shift register
  // button             LED
  ID_MUTE_CHN1,         ID_MUTE_CHN1,
  ID_MUTE_CHN2,         ID_MUTE_CHN2,
  ID_MUTE_CHN3,         ID_MUTE_CHN3,
  ID_MUTE_CHN4,         ID_MUTE_CHN4,
  ID_MUTE_CHN5,         ID_MUTE_CHN5,
  ID_MUTE_CHN6,         ID_MUTE_CHN6,
  ID_MUTE_CHN7,         ID_MUTE_CHN7,
  ID_MUTE_CHN8,         ID_MUTE_CHN8,

  // 5th shift register
  // button             LED
  ID_ASSIGN_TRACK,      ID_ASSIGN_TRACK,
  ID_ASSIGN_SEND,       ID_ASSIGN_SEND,
  ID_ASSIGN_PAN_SURROUND, ID_ASSIGN_PAN_SURROUND,
  ID_ASSIGN_PLUG_IN,    ID_ASSIGN_PLUG_IN,
  ID_ASSIGN_EQ,         ID_ASSIGN_EQ,
  ID_ASSIGN_INSTRUMENT, ID_ASSIGN_INSTRUMENT,
  ID_BANK_LEFT,         ID_IGNORE,              // (no LED supported by host)
  ID_BANK_RIGHT,        ID_IGNORE,              // (no LED supported by host)

  // 6th shift register
  // button             LED
  ID_BANK_CHANNEL_LEFT, ID_IGNORE,              // (no LED supported by host)
  ID_BANK_CHANNEL_RIGHT,ID_IGNORE,              // (no LED supported by host)
  ID_FLIP,              ID_FLIP,
  ID_GLOBAL_VIEW,       ID_GLOBAL_VIEW,
  ID_SHIFT,             ID_IGNORE,              // (no LED supported by host)
  ID_OPTION,            ID_IGNORE,              // (no LED supported by host)
  ID_CONTROL,           ID_IGNORE,              // (no LED supported by host)
  ID_CMD_ALT,           ID_IGNORE,              // (no LED supported by host)

  // 7th shift register
  // button             LED
  ID_MARKER,            ID_MARKER,
  ID_NUDGE,             ID_NUDGE,
  ID_CYCLE,             ID_CYCLE,
  ID_DROP,              ID_DROP,
  ID_REPLACE,           ID_REPLACE,
  ID_CLICK,             ID_CLICK,
  ID_REWIND,            ID_REWIND,
  ID_FAST_FWD,          ID_FAST_FWD,

  // 8th shift register
  // button             LED
  ID_STOP,              ID_STOP,
  ID_PLAY,              ID_PLAY,
  ID_RECORD,            ID_RECORD,
  ID_CURSOR_UP,         ID_IGNORE,              // (no LED supported by host)
  ID_CURSOR_LEFT,       ID_IGNORE,              // (no LED supported by host)
  ID_ZOOM,              ID_ZOOM,
  ID_CURSOR_RIGHT,      ID_IGNORE,              // (no LED supported by host)
  ID_CURSOR_DOWN,       ID_IGNORE,              // (no LED supported by host)

  // 9th shift register
  // button             LED
  ID_VPOT_SELECT_CHN1,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN2,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN3,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN4,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN5,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN6,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN7,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN8,  ID_IGNORE,              // (no LED supported by host)

  // 10th shift register
  // button             LED
  ID_GLOBAL_TRACKS,     ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_INPUTS,     ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_AUDIO_TRCK, ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_AUDIO_INSTR,ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_AUX,        ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_BUSSES,     ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_OUTPUTS,    ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_USER,       ID_IGNORE,              // (no LED supported by host)

  // 11th shift register
  // button             LED
  ID_AUTOM_READ_OFF,    ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_AUTOM_WRITE,       ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_AUTOM_TRIM,        ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_AUTOM_TOUCH,       ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_AUTOM_LATCH,       ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_UTIL_CANCEL,       ID_IGNORE,              // (no LED supported by host)
  ID_UTIL_ENTER,        ID_IGNORE,              // (no LED supported by host)
  ID_MBLC_HOLD_LAYER1,  ID_IGNORE,              // (MIDIbox LC specific: switch to other layer when button (de)pressed)

  // 12th shift register
  // button             LED
  ID_F1,                ID_IGNORE,              // (no LED supported by host)
  ID_F2,                ID_IGNORE,              // (no LED supported by host)
  ID_F3,                ID_IGNORE,              // (no LED supported by host)
  ID_F4,                ID_IGNORE,              // (no LED supported by host)
  ID_F5,                ID_IGNORE,              // (no LED supported by host)
  ID_F6,                ID_IGNORE,              // (no LED supported by host)
  ID_F7,                ID_IGNORE,              // (no LED supported by host)
  ID_F8,                ID_IGNORE,              // (no LED supported by host)

  // 13th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)

  // 14th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)

  // 15th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)

  // 16th shift register
  // button             LED
  ID_FADER_TOUCH_CHN1,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN2,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN3,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN4,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN5,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN6,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN7,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN8,  ID_IGNORE,              // (no LED supported by host)
};

// ==========================================================================
//  This table is used for a second layer which can be selected by a
//  special function button (ID_MBLC_LAYER_*, see #define section above)
//
//  Tips:
//     o mostly used button/led functions like cursor, V-pot select, etc
//       should be presented in both tables at the same place
//     o the layer select buttons should be defined at the same place!
//     o use either ID_MBLC_HOLD_LAYER1 or ID_MBLC_TOGGLE_LAYER1 to
//       select the layer
// ==========================================================================
const u8 lc_dio_table_layer1[2*128] = {

  // in second layer the MBLC specific buttons are assigned here instead of ID_REC_RDY_CHNx

  // 1st shift register
  // button             LED
  ID_MBLC_DISPLAY_PAGE0,ID_MBLC_DISPLAY_PAGE0,
  ID_MBLC_DISPLAY_PAGE1,ID_MBLC_DISPLAY_PAGE1,
  ID_MBLC_DISPLAY_PAGE2,ID_MBLC_DISPLAY_PAGE2,
  ID_MBLC_DISPLAY_PAGE3,ID_MBLC_DISPLAY_PAGE3,
  ID_MBLC_SWITCH_LEDMETER0, ID_MBLC_SWITCH_LEDMETER0,
  ID_MBLC_SWITCH_LEDMETER1, ID_MBLC_SWITCH_LEDMETER1,
  ID_IGNORE,            ID_IGNORE,
  ID_GROUP,             ID_GROUP,

  // in second layer some additional LC functions are assigned here instead of ID_SOLO_CHNx

  // 2nd shift register
  // button             LED
  ID_UTIL_SAVE,		ID_UTIL_SAVE,
  ID_UTIL_UNDO,		ID_UTIL_UNDO,
  ID_UTIL_CANCEL,       ID_UTIL_CANCEL,
  ID_UTIL_ENTER,        ID_UTIL_ENTER,
  ID_IGNORE,	        ID_IGNORE,
  ID_MBLC_SWITCH_LC,    ID_MBLC_SWITCH_LC,  // logic control emulation mode
  ID_MBLC_SWITCH_GPC,   ID_MBLC_SWITCH_GPC, // general purpose controller mode
  ID_IGNORE,	        ID_IGNORE,

  // 3rd shift register
  // button             LED
  ID_SOLO_CHN1,         ID_SOLO_CHN1,
  ID_SOLO_CHN2,         ID_SOLO_CHN2,
  ID_SOLO_CHN3,         ID_SOLO_CHN3,
  ID_SOLO_CHN4,         ID_SOLO_CHN4,
  ID_SOLO_CHN5,         ID_SOLO_CHN5,
  ID_SOLO_CHN6,         ID_SOLO_CHN6,
  ID_SOLO_CHN7,         ID_SOLO_CHN7,
  ID_SOLO_CHN8,         ID_SOLO_CHN8,

  // 4th shift register
  // button             LED
  ID_MUTE_CHN1,         ID_MUTE_CHN1,
  ID_MUTE_CHN2,         ID_MUTE_CHN2,
  ID_MUTE_CHN3,         ID_MUTE_CHN3,
  ID_MUTE_CHN4,         ID_MUTE_CHN4,
  ID_MUTE_CHN5,         ID_MUTE_CHN5,
  ID_MUTE_CHN6,         ID_MUTE_CHN6,
  ID_MUTE_CHN7,         ID_MUTE_CHN7,
  ID_MUTE_CHN8,         ID_MUTE_CHN8,

  // 5th shift register
  // button             LED
  ID_ASSIGN_TRACK,      ID_ASSIGN_TRACK,
  ID_ASSIGN_SEND,       ID_ASSIGN_SEND,
  ID_ASSIGN_PAN_SURROUND, ID_ASSIGN_PAN_SURROUND,
  ID_ASSIGN_PLUG_IN,    ID_ASSIGN_PLUG_IN,
  ID_ASSIGN_EQ,         ID_ASSIGN_EQ,
  ID_ASSIGN_INSTRUMENT, ID_ASSIGN_INSTRUMENT,
  ID_BANK_LEFT,         ID_IGNORE,              // (no LED supported by host)
  ID_BANK_RIGHT,        ID_IGNORE,              // (no LED supported by host)

  // 6th shift register
  // button             LED
  ID_BANK_CHANNEL_LEFT, ID_IGNORE,              // (no LED supported by host)
  ID_BANK_CHANNEL_RIGHT,ID_IGNORE,              // (no LED supported by host)
  ID_FLIP,              ID_FLIP,
  ID_GLOBAL_VIEW,       ID_GLOBAL_VIEW,
  ID_SHIFT,             ID_IGNORE,              // (no LED supported by host)
  ID_OPTION,            ID_IGNORE,              // (no LED supported by host)
  ID_CONTROL,           ID_IGNORE,              // (no LED supported by host)
  ID_CMD_ALT,           ID_IGNORE,              // (no LED supported by host)

  // 7th shift register
  // button             LED
  ID_MARKER,            ID_MARKER,
  ID_NUDGE,             ID_NUDGE,
  ID_CYCLE,             ID_CYCLE,
  ID_DROP,              ID_DROP,
  ID_REPLACE,           ID_REPLACE,
  ID_CLICK,             ID_CLICK,
  ID_REWIND,            ID_REWIND,
  ID_FAST_FWD,          ID_FAST_FWD,

  // 8th shift register
  // button             LED
  ID_STOP,              ID_STOP,
  ID_PLAY,              ID_PLAY,
  ID_RECORD,            ID_RECORD,
  ID_CURSOR_UP,         ID_IGNORE,              // (no LED supported by host)
  ID_CURSOR_LEFT,       ID_IGNORE,              // (no LED supported by host)

  // in second layer, use "scrub" instead of "zoom"
  ID_SCRUB,             ID_SCRUB,

  ID_CURSOR_RIGHT,      ID_IGNORE,              // (no LED supported by host)
  ID_CURSOR_DOWN,       ID_IGNORE,              // (no LED supported by host)

  // 9th shift register
  // button             LED
  ID_VPOT_SELECT_CHN1,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN2,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN3,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN4,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN5,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN6,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN7,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN8,  ID_IGNORE,              // (no LED supported by host)

  // 10th shift register
  // button             LED
  ID_GLOBAL_TRACKS,     ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_INPUTS,     ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_AUDIO_TRCK, ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_AUDIO_INSTR,ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_AUX,        ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_BUSSES,     ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_OUTPUTS,    ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_USER,       ID_IGNORE,              // (no LED supported by host)

  // 11th shift register
  // button             LED
  ID_AUTOM_READ_OFF,    ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_AUTOM_WRITE,       ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_AUTOM_TRIM,        ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_AUTOM_TOUCH,       ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_AUTOM_LATCH,       ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_UTIL_CANCEL,       ID_IGNORE,              // (no LED supported by host)
  ID_UTIL_ENTER,        ID_IGNORE,              // (no LED supported by host)
  ID_MBLC_HOLD_LAYER1,  ID_IGNORE,              // (MIDIbox LC specific: switch to other layer when button (de)pressed)

  // 12th shift register
  // button             LED
  ID_F1,                ID_IGNORE,              // (no LED supported by host)
  ID_F2,                ID_IGNORE,              // (no LED supported by host)
  ID_F3,                ID_IGNORE,              // (no LED supported by host)
  ID_F4,                ID_IGNORE,              // (no LED supported by host)
  ID_F5,                ID_IGNORE,              // (no LED supported by host)
  ID_F6,                ID_IGNORE,              // (no LED supported by host)
  ID_F7,                ID_IGNORE,              // (no LED supported by host)
  ID_F8,                ID_IGNORE,              // (no LED supported by host)

  // 13th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)

  // 14th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)

  // 15th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)

  // 16th shift register
  // button             LED
  ID_FADER_TOUCH_CHN1,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN2,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN3,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN4,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN5,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN6,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN7,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN8,  ID_IGNORE,              // (no LED supported by host)
};

// ==========================================================================
//  This table is used for a third layer which can be selected by a
//  special function button (ID_MBLC_LAYER_*, see #define section above)
//
//  Tips:
//     o mostly used button/led functions like cursor, V-pot select, etc
//       should be presented in both tables at the same place
//     o the layer select buttons should be defined at the same place!
//     o use either ID_MBLC_HOLD_LAYER2 or ID_MBLC_TOGGLE_LAYER2 to
//       select the layer
// ==========================================================================
const u8 lc_dio_table_layer2[2*128] = {

  // THE THIRD LAYER IS NOT USED IN THIS DEFAULT SETUP
  // SEE lc_dio_table_mbseq.inc FOR A BETTER EXAMPLE !!!

  // 1st shift register
  // button             LED
  ID_MBLC_DISPLAY_PAGE0,ID_MBLC_DISPLAY_PAGE0,
  ID_MBLC_DISPLAY_PAGE1,ID_MBLC_DISPLAY_PAGE1,
  ID_MBLC_DISPLAY_PAGE2,ID_MBLC_DISPLAY_PAGE2,
  ID_MBLC_DISPLAY_PAGE3,ID_MBLC_DISPLAY_PAGE3,
  ID_MBLC_SWITCH_LEDMETER0, ID_MBLC_SWITCH_LEDMETER0,
  ID_MBLC_SWITCH_LEDMETER1, ID_MBLC_SWITCH_LEDMETER1,
  ID_IGNORE,            ID_IGNORE,
  ID_GROUP,             ID_GROUP,

  // in second layer some additional LC functions are assigned here instead of ID_SOLO_CHNx

  // 2nd shift register
  // button             LED
  ID_UTIL_SAVE,		ID_UTIL_SAVE,
  ID_UTIL_UNDO,		ID_UTIL_UNDO,
  ID_UTIL_CANCEL,       ID_UTIL_CANCEL,
  ID_UTIL_ENTER,        ID_UTIL_ENTER,
  ID_IGNORE,	        ID_IGNORE,
  ID_MBLC_SWITCH_LC,    ID_MBLC_SWITCH_LC,  // logic control emulation mode
  ID_MBLC_SWITCH_GPC,   ID_MBLC_SWITCH_GPC, // general purpose controller mode
  ID_IGNORE,	        ID_IGNORE,

  // 3rd shift register
  // button             LED
  ID_SOLO_CHN1,         ID_SOLO_CHN1,
  ID_SOLO_CHN2,         ID_SOLO_CHN2,
  ID_SOLO_CHN3,         ID_SOLO_CHN3,
  ID_SOLO_CHN4,         ID_SOLO_CHN4,
  ID_SOLO_CHN5,         ID_SOLO_CHN5,
  ID_SOLO_CHN6,         ID_SOLO_CHN6,
  ID_SOLO_CHN7,         ID_SOLO_CHN7,
  ID_SOLO_CHN8,         ID_SOLO_CHN8,

  // 4th shift register
  // button             LED
  ID_MUTE_CHN1,         ID_MUTE_CHN1,
  ID_MUTE_CHN2,         ID_MUTE_CHN2,
  ID_MUTE_CHN3,         ID_MUTE_CHN3,
  ID_MUTE_CHN4,         ID_MUTE_CHN4,
  ID_MUTE_CHN5,         ID_MUTE_CHN5,
  ID_MUTE_CHN6,         ID_MUTE_CHN6,
  ID_MUTE_CHN7,         ID_MUTE_CHN7,
  ID_MUTE_CHN8,         ID_MUTE_CHN8,

  // 5th shift register
  // button             LED
  ID_ASSIGN_TRACK,      ID_ASSIGN_TRACK,
  ID_ASSIGN_SEND,       ID_ASSIGN_SEND,
  ID_ASSIGN_PAN_SURROUND, ID_ASSIGN_PAN_SURROUND,
  ID_ASSIGN_PLUG_IN,    ID_ASSIGN_PLUG_IN,
  ID_ASSIGN_EQ,         ID_ASSIGN_EQ,
  ID_ASSIGN_INSTRUMENT, ID_ASSIGN_INSTRUMENT,
  ID_BANK_LEFT,         ID_IGNORE,              // (no LED supported by host)
  ID_BANK_RIGHT,        ID_IGNORE,              // (no LED supported by host)

  // 6th shift register
  // button             LED
  ID_BANK_CHANNEL_LEFT, ID_IGNORE,              // (no LED supported by host)
  ID_BANK_CHANNEL_RIGHT,ID_IGNORE,              // (no LED supported by host)
  ID_FLIP,              ID_FLIP,
  ID_GLOBAL_VIEW,       ID_GLOBAL_VIEW,
  ID_SHIFT,             ID_IGNORE,              // (no LED supported by host)
  ID_OPTION,            ID_IGNORE,              // (no LED supported by host)
  ID_CONTROL,           ID_IGNORE,              // (no LED supported by host)
  ID_CMD_ALT,           ID_IGNORE,              // (no LED supported by host)

  // 7th shift register
  // button             LED
  ID_MARKER,            ID_MARKER,
  ID_NUDGE,             ID_NUDGE,
  ID_CYCLE,             ID_CYCLE,
  ID_DROP,              ID_DROP,
  ID_REPLACE,           ID_REPLACE,
  ID_CLICK,             ID_CLICK,
  ID_REWIND,            ID_REWIND,
  ID_FAST_FWD,          ID_FAST_FWD,

  // 8th shift register
  // button             LED
  ID_STOP,              ID_STOP,
  ID_PLAY,              ID_PLAY,
  ID_RECORD,            ID_RECORD,
  ID_CURSOR_UP,         ID_IGNORE,              // (no LED supported by host)
  ID_CURSOR_LEFT,       ID_IGNORE,              // (no LED supported by host)

  // in second layer, use "scrub" instead of "zoom"
  ID_SCRUB,             ID_SCRUB,

  ID_CURSOR_RIGHT,      ID_IGNORE,              // (no LED supported by host)
  ID_CURSOR_DOWN,       ID_IGNORE,              // (no LED supported by host)

  // 9th shift register
  // button             LED
  ID_VPOT_SELECT_CHN1,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN2,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN3,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN4,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN5,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN6,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN7,  ID_IGNORE,              // (no LED supported by host)
  ID_VPOT_SELECT_CHN8,  ID_IGNORE,              // (no LED supported by host)

  // 10th shift register
  // button             LED
  ID_GLOBAL_TRACKS,     ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_INPUTS,     ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_AUDIO_TRCK, ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_AUDIO_INSTR,ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_AUX,        ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_BUSSES,     ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_OUTPUTS,    ID_IGNORE,              // (no LED supported by host)
  ID_GLOBAL_USER,       ID_IGNORE,              // (no LED supported by host)

  // 11th shift register
  // button             LED
  ID_AUTOM_READ_OFF,    ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_AUTOM_WRITE,       ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_AUTOM_TRIM,        ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_AUTOM_TOUCH,       ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_AUTOM_LATCH,       ID_IGNORE,              // (in my setup no LEDs are connected here)
  ID_UTIL_CANCEL,       ID_IGNORE,              // (no LED supported by host)
  ID_UTIL_ENTER,        ID_IGNORE,              // (no LED supported by host)
  ID_MBLC_HOLD_LAYER2,  ID_IGNORE,              // (MIDIbox LC specific: switch to other layer when button (de)pressed)

  // 12th shift register
  // button             LED
  ID_F1,                ID_IGNORE,              // (no LED supported by host)
  ID_F2,                ID_IGNORE,              // (no LED supported by host)
  ID_F3,                ID_IGNORE,              // (no LED supported by host)
  ID_F4,                ID_IGNORE,              // (no LED supported by host)
  ID_F5,                ID_IGNORE,              // (no LED supported by host)
  ID_F6,                ID_IGNORE,              // (no LED supported by host)
  ID_F7,                ID_IGNORE,              // (no LED supported by host)
  ID_F8,                ID_IGNORE,              // (no LED supported by host)

  // 13th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)

  // 14th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)

  // 15th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in my setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)
  ID_IGNORE,            ID_IGNORE,              // (spare pin in my setup)

  // 16th shift register
  // button             LED
  ID_FADER_TOUCH_CHN1,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN2,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN3,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN4,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN5,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN6,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN7,  ID_IGNORE,              // (no LED supported by host)
  ID_FADER_TOUCH_CHN8,  ID_IGNORE,              // (no LED supported by host)
};

#endif /* MBSEQ_HARDWARE_OPTION */
