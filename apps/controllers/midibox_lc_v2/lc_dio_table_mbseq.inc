// $Id$
/*
 * MIDIbox LC V2
 * Special Configuration for MBSEQ Hardware
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
// Included from lc_dio_table.c
// See also the comments there!
/////////////////////////////////////////////////////////////////////////////


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
const unsigned char lc_dio_table_layer0[2*128] = {
  // 1st shift register
  // button             LED
  ID_IGNORE,            ID_ASSIGN_TRACK,        // datawheel is connected to DIN, "Track 1" LED
  ID_IGNORE,            ID_ASSIGN_SEND,         // datawheel is connected to DIN, "Track 2" LED
  ID_SCRUB,             ID_ASSIGN_PAN_SURROUND, // "Scrub" Button, "Track 3" LED
  ID_CLICK,             ID_ASSIGN_PLUG_IN,      // "Metronome" Button, "Track 4" LED
  ID_STOP,              ID_ASSIGN_EQ,           // "Stop" Button, "Layer A" LED
  ID_RECORD,            ID_ASSIGN_INSTRUMENT,   // "Pause" Button, "Layer B" LED
  ID_PLAY,              ID_MBLC_TOGGLE_GPC,     // "Play" Button, "Layer C" LED
  ID_REWIND,            ID_PLAY,                // "Rew" Button, "Beat Indicator" LED

  // 2nd shift register
  // button             LED
  ID_FAST_FWD,          ID_MARKER,              // "Fwd" Button, "Edit" LED
  ID_BANK_LEFT,         ID_NUDGE,               // "F1" Button, "Mute" LED
  ID_BANK_RIGHT,        ID_CYCLE,               // "F2" Button, "Pattern" LED
  ID_BANK_CHANNEL_LEFT, ID_DROP,                // "F3" Button, "Song" LED
  ID_BANK_CHANNEL_RIGHT,ID_SOLO,                // "F4" Button, "Solo" LED
  ID_GLOBAL_VIEW,       ID_MBLC_TOGGLE_LAYER1,  // "Menu" Button, "Fast" LED
  ID_SHIFT,             ID_MBLC_TOGGLE_LAYER2,  // "Select" Button, "Song" LED
  ID_OPTION,            ID_IGNORE,              // "Exit" Button, no LED

  // 3rd shift register
  // button             LED
  ID_ASSIGN_TRACK,      ID_MUTE_CHN1,           // "Track 1" Button, "1" LED
  ID_ASSIGN_SEND,       ID_VPOT_SELECT_CHN1,    // "Track 2" Button, "2" LED
  ID_ASSIGN_PAN_SURROUND, ID_MUTE_CHN2,         // "Track 3" Button, "3" LED
  ID_ASSIGN_PLUG_IN,    ID_VPOT_SELECT_CHN2,    // "Track 4" Button, "4" LED
  ID_ASSIGN_EQ,         ID_MUTE_CHN3,           // "Layer A" Button, "5" LED
  ID_ASSIGN_INSTRUMENT, ID_VPOT_SELECT_CHN3,    // "Layer B" Button, "6" LED
  ID_MBLC_TOGGLE_GPC,   ID_MUTE_CHN4,           // "Layer C" Button, "7" LED
  ID_IGNORE,            ID_VPOT_SELECT_CHN4,    // no button, "8" LED

  // 4th shift register
  // button             LED
  ID_MARKER,            ID_MUTE_CHN5,           // "Edit" Button, "9" LED
  ID_NUDGE,             ID_VPOT_SELECT_CHN5,    // "Mute" Button, "10" LED
  ID_CYCLE,             ID_MUTE_CHN6,           // "Pattern" Button, "11" LED
  ID_DROP,              ID_VPOT_SELECT_CHN6,    // "Song" Button, "12" LED
  ID_SOLO,              ID_MUTE_CHN7,           // "Solo" Button, "13" LED
  ID_MBLC_TOGGLE_LAYER1,ID_VPOT_SELECT_CHN7,    // "Fast" Button, "14" LED
  ID_MBLC_TOGGLE_LAYER2,ID_MUTE_CHN8,           // "All" Button, "15" LED
  ID_IGNORE,            ID_VPOT_SELECT_CHN8,    // no button, "16" LED

  // 5th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)

  // 6th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)

  // 7th shift register
  // button             LED
  ID_MUTE_CHN1,         ID_IGNORE,              // "1" Button, no LED
  ID_VPOT_SELECT_CHN1,  ID_IGNORE,              // "2" Button, no LED
  ID_MUTE_CHN2,         ID_IGNORE,              // "3" Button, no LED
  ID_VPOT_SELECT_CHN2,  ID_IGNORE,              // "4" Button, no LED
  ID_MUTE_CHN3,         ID_IGNORE,              // "5" Button, no LED
  ID_VPOT_SELECT_CHN3,  ID_IGNORE,              // "6" Button, no LED
  ID_MUTE_CHN4,         ID_IGNORE,              // "7" Button, no LED
  ID_VPOT_SELECT_CHN4,  ID_IGNORE,              // "8" Button, no LED

  // 8th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)

  // 9th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)

  // 10th shift register
  // button             LED
  ID_MUTE_CHN5,         ID_IGNORE,              // "9" Button, no LED
  ID_VPOT_SELECT_CHN5,  ID_IGNORE,              // "10" Button, no LED
  ID_MUTE_CHN6,         ID_IGNORE,              // "11" Button, no LED
  ID_VPOT_SELECT_CHN6,  ID_IGNORE,              // "12" Button, no LED
  ID_MUTE_CHN7,         ID_IGNORE,              // "13" Button, no LED
  ID_VPOT_SELECT_CHN7,  ID_IGNORE,              // "14" Button, no LED
  ID_MUTE_CHN8,         ID_IGNORE,              // "15" Button, no LED
  ID_VPOT_SELECT_CHN8,  ID_IGNORE,              // "16" Button, no LED

  // 11th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 12th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 13th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 14th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 15th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 16th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
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
const unsigned char lc_dio_table_layer1[2*128] = {
  // 1st shift register
  // button             LED
  ID_IGNORE,            ID_UTIL_SAVE,           // datawheel is connected to DIN, "Track 1" LED
  ID_IGNORE,            ID_UTIL_UNDO,           // datawheel is connected to DIN, "Track 2" LED
  ID_SCRUB,             ID_UTIL_CANCEL,         // "Scrub" Button, "Track 3" LED
  ID_CLICK,             ID_UTIL_ENTER,          // "Metronome" Button, "Track 4" LED
  ID_STOP,              ID_AUTOM_READ_OFF,      // "Stop" Button, "Layer A" LED
  ID_RECORD,            ID_AUTOM_TOUCH,         // "Pause" Button, "Layer B" LED
  ID_PLAY,              ID_MBLC_TOGGLE_GPC,     // "Play" Button, "Layer C" LED
  ID_REWIND,            ID_PLAY,                // "Rew" Button, "Beat Indicator" LED

  // 2nd shift register
  // button             LED
  ID_FAST_FWD,          ID_MBLC_DISPLAY_PAGE0,  // "Fwd" Button, "Edit" LED
  ID_CURSOR_UP,         ID_MBLC_DISPLAY_PAGE1,  // "F1" Button, "Mute" LED
  ID_CURSOR_DOWN,       ID_MBLC_DISPLAY_PAGE2,  // "F2" Button, "Pattern" LED
  ID_CURSOR_LEFT,       ID_MBLC_DISPLAY_PAGE3,  // "F3" Button, "Song" LED
  ID_CURSOR_RIGHT,      ID_SOLO,                // "F4" Button, "Solo" LED
  ID_ZOOM,              ID_MBLC_TOGGLE_LAYER1,  // "Menu" Button, "Fast" LED
  ID_CONTROL,           ID_MBLC_TOGGLE_LAYER2,  // "Select" Button, "Song" LED
  ID_CMD_ALT,           ID_IGNORE,              // "Exit" Button, no LED

  // 3rd shift register
  // button             LED
  ID_UTIL_SAVE,         ID_SOLO_CHN1,           // "Track 1" Button, "1" LED
  ID_UTIL_UNDO,         ID_SELECT_CHN1 ,        // "Track 2" Button, "2" LED
  ID_UTIL_CANCEL,       ID_SOLO_CHN2,           // "Track 3" Button, "3" LED
  ID_UTIL_ENTER,        ID_SELECT_CHN2,         // "Track 4" Button, "4" LED
  ID_AUTOM_READ_OFF,    ID_SOLO_CHN3,           // "Layer A" Button, "5" LED
  ID_AUTOM_TOUCH,       ID_SELECT_CHN3,         // "Layer B" Button, "6" LED
  ID_MBLC_TOGGLE_GPC,   ID_SOLO_CHN4,           // "Layer C" Button, "7" LED
  ID_IGNORE,            ID_SELECT_CHN4,         // no button, "8" LED

  // 4th shift register
  // button             LED
  ID_MBLC_DISPLAY_PAGE0,ID_SOLO_CHN5,           // "Edit" Button, "9" LED
  ID_MBLC_DISPLAY_PAGE1,ID_SELECT_CHN5,         // "Mute" Button, "10" LED
  ID_MBLC_DISPLAY_PAGE2,ID_SOLO_CHN6,           // "Pattern" Button, "11" LED
  ID_MBLC_DISPLAY_PAGE3,ID_SELECT_CHN6,         // "Song" Button, "12" LED
  ID_SOLO,              ID_SOLO_CHN7,           // "Solo" Button, "13" LED
  ID_MBLC_TOGGLE_LAYER1,ID_SELECT_CHN7,         // "Fast" Button, "14" LED
  ID_MBLC_TOGGLE_LAYER2,ID_SOLO_CHN8,           // "All" Button, "15" LED
  ID_IGNORE,            ID_SELECT_CHN8,         // no button, "16" LED

  // 5th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)

  // 6th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)

  // 7th shift register
  // button             LED
  ID_SOLO_CHN1,         ID_IGNORE,              // "1" Button, no LED
  ID_SELECT_CHN1,       ID_IGNORE,              // "2" Button, no LED
  ID_SOLO_CHN2,         ID_IGNORE,              // "3" Button, no LED
  ID_SELECT_CHN2,       ID_IGNORE,              // "4" Button, no LED
  ID_SOLO_CHN3,         ID_IGNORE,              // "5" Button, no LED
  ID_SELECT_CHN3,       ID_IGNORE,              // "6" Button, no LED
  ID_SOLO_CHN4,         ID_IGNORE,              // "7" Button, no LED
  ID_SELECT_CHN4,       ID_IGNORE,              // "8" Button, no LED

  // 8th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)

  // 9th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)

  // 10th shift register
  // button             LED
  ID_SOLO_CHN5,         ID_IGNORE,              // "9" Button, no LED
  ID_SELECT_CHN5,       ID_IGNORE,              // "10" Button, no LED
  ID_SOLO_CHN6,         ID_IGNORE,              // "11" Button, no LED
  ID_SELECT_CHN6,       ID_IGNORE,              // "12" Button, no LED
  ID_SOLO_CHN7,         ID_IGNORE,              // "13" Button, no LED
  ID_SELECT_CHN7,       ID_IGNORE,              // "14" Button, no LED
  ID_SOLO_CHN8,         ID_IGNORE,              // "15" Button, no LED
  ID_SELECT_CHN8,       ID_IGNORE,              // "16" Button, no LED

  // 11th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 12th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 13th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 14th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 15th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 16th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
};

// ==========================================================================
//  This table is used for a second layer which can be selected by a
//  special function button (ID_MBLC_LAYER_*, see #define section above)
//
//  Tips:
//     o mostly used button/led functions like cursor, V-pot select, etc
//       should be presented in both tables at the same place
//     o the layer select buttons should be defined at the same place!
//     o use either ID_MBLC_HOLD_LAYER2 or ID_MBLC_TOGGLE_LAYER2 to
//       select the layer
// ==========================================================================
const unsigned char lc_dio_table_layer2[2*128] = {
  // 1st shift register
  // button             LED
  ID_IGNORE,            ID_UTIL_SAVE,           // datawheel is connected to DIN, "Track 1" LED
  ID_IGNORE,            ID_UTIL_UNDO,           // datawheel is connected to DIN, "Track 2" LED
  ID_SCRUB,             ID_UTIL_CANCEL,         // "Scrub" Button, "Track 3" LED
  ID_CLICK,             ID_UTIL_ENTER,          // "Metronome" Button, "Track 4" LED
  ID_STOP,              ID_AUTOM_READ_OFF,      // "Stop" Button, "Layer A" LED
  ID_RECORD,            ID_AUTOM_TOUCH,         // "Pause" Button, "Layer B" LED
  ID_PLAY,              ID_MBLC_TOGGLE_GPC,     // "Play" Button, "Layer C" LED
  ID_REWIND,            ID_PLAY,                // "Rew" Button, "Beat Indicator" LED

  // 2nd shift register
  // button             LED
  ID_FAST_FWD,          ID_MBLC_DISPLAY_PAGE0,  // "Fwd" Button, "Edit" LED
  ID_CURSOR_UP,         ID_MBLC_DISPLAY_PAGE1,  // "F1" Button, "Mute" LED
  ID_CURSOR_DOWN,       ID_MBLC_DISPLAY_PAGE2,  // "F2" Button, "Pattern" LED
  ID_CURSOR_LEFT,       ID_MBLC_DISPLAY_PAGE3,  // "F3" Button, "Song" LED
  ID_CURSOR_RIGHT,      ID_SOLO,                // "F4" Button, "Solo" LED
  ID_ZOOM,              ID_MBLC_TOGGLE_LAYER1,  // "Menu" Button, "Fast" LED
  ID_CONTROL,           ID_MBLC_TOGGLE_LAYER2,  // "Select" Button, "Song" LED
  ID_CMD_ALT,           ID_IGNORE,              // "Exit" Button, no LED

  // 3rd shift register
  // button             LED
  ID_UTIL_SAVE,         ID_REC_RDY_CHN1,        // "Track 1" Button, "1" LED
  ID_UTIL_UNDO,         ID_SELECT_CHN1 ,        // "Track 2" Button, "2" LED
  ID_UTIL_CANCEL,       ID_REC_RDY_CHN2,        // "Track 3" Button, "3" LED
  ID_UTIL_ENTER,        ID_SELECT_CHN2,         // "Track 4" Button, "4" LED
  ID_AUTOM_READ_OFF,    ID_REC_RDY_CHN3,        // "Layer A" Button, "5" LED
  ID_AUTOM_TOUCH,       ID_SELECT_CHN3,         // "Layer B" Button, "6" LED
  ID_MBLC_TOGGLE_GPC,   ID_REC_RDY_CHN4,        // "Layer C" Button, "7" LED
  ID_IGNORE,            ID_SELECT_CHN4,         // no button, "8" LED

  // 4th shift register
  // button             LED
  ID_MBLC_DISPLAY_PAGE0,ID_REC_RDY_CHN5,        // "Edit" Button, "9" LED
  ID_MBLC_DISPLAY_PAGE1,ID_SELECT_CHN5,         // "Mute" Button, "10" LED
  ID_MBLC_DISPLAY_PAGE2,ID_REC_RDY_CHN6,        // "Pattern" Button, "11" LED
  ID_MBLC_DISPLAY_PAGE3,ID_SELECT_CHN6,         // "Song" Button, "12" LED
  ID_SOLO,              ID_REC_RDY_CHN7,        // "Solo" Button, "13" LED
  ID_MBLC_TOGGLE_LAYER1,ID_SELECT_CHN7,         // "Fast" Button, "14" LED
  ID_MBLC_TOGGLE_LAYER2,ID_REC_RDY_CHN8,        // "All" Button, "15" LED
  ID_IGNORE,            ID_SELECT_CHN8,         // no button, "16" LED

  // 5th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)

  // 6th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)

  // 7th shift register
  // button             LED
  ID_REC_RDY_CHN1,      ID_IGNORE,              // "1" Button, no LED
  ID_SELECT_CHN1,       ID_IGNORE,              // "2" Button, no LED
  ID_REC_RDY_CHN2,      ID_IGNORE,              // "3" Button, no LED
  ID_SELECT_CHN2,       ID_IGNORE,              // "4" Button, no LED
  ID_REC_RDY_CHN3,      ID_IGNORE,              // "5" Button, no LED
  ID_SELECT_CHN3,       ID_IGNORE,              // "6" Button, no LED
  ID_REC_RDY_CHN4,      ID_IGNORE,              // "7" Button, no LED
  ID_SELECT_CHN4,       ID_IGNORE,              // "8" Button, no LED

  // 8th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)

  // 9th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)
  ID_IGNORE,            ID_IGNORE,              // (in the MBSEQ setup encoders are connected here)

  // 10th shift register
  // button             LED
  ID_REC_RDY_CHN5,      ID_IGNORE,              // "9" Button, no LED
  ID_SELECT_CHN5,       ID_IGNORE,              // "10" Button, no LED
  ID_REC_RDY_CHN6,      ID_IGNORE,              // "11" Button, no LED
  ID_SELECT_CHN6,       ID_IGNORE,              // "12" Button, no LED
  ID_REC_RDY_CHN7,      ID_IGNORE,              // "13" Button, no LED
  ID_SELECT_CHN7,       ID_IGNORE,              // "14" Button, no LED
  ID_REC_RDY_CHN8,      ID_IGNORE,              // "15" Button, no LED
  ID_SELECT_CHN8,       ID_IGNORE,              // "16" Button, no LED

  // 11th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 12th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 13th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 14th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 15th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)

  // 16th shift register
  // button             LED
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
  ID_IGNORE,            ID_IGNORE,              // (no button, no LED)
};

