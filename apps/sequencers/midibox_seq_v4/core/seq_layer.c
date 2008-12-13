// $Id$
/*
 * Sequencer Parameter Layer Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "seq_layer.h"
#include "seq_core.h"
#include "seq_cc.h"
#include "seq_trg.h"
#include "seq_par.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions and arrays
/////////////////////////////////////////////////////////////////////////////

// variable types (each step has a selectable value, embedded in the three available layers)
// Bit 3-0 is a unique number (0x0...0xf)
// Bit 6-4 select the MIDI event status byte, bit 7 must be 0!
//    0x0: not relevant for status byte (e.g. note length)
//    0x1: Note
//    0x3: CC
#define SEQ_LAYER_V_NONE	0x00
#define SEQ_LAYER_V_NOTE	0x11
#define SEQ_LAYER_V_CHORD1	0x12
#define SEQ_LAYER_V_CHORD2	0x13
#define SEQ_LAYER_V_VEL		0x14
#define SEQ_LAYER_V_CHORD1_VEL	0x15
#define SEQ_LAYER_V_CHORD2_VEL	0x16
#define SEQ_LAYER_V_LEN		0x07
#define SEQ_LAYER_V_CC		0x38

static const char seq_layer_v_names[9][5] = {
  "----",	// SEQ_LAYER_V_NONE
  "Note",	// SEQ_LAYER_V_NOTE
  "Crd1",	// SEQ_LAYER_V_CHORD1
  "Crd2",	// SEQ_LAYER_V_CHORD2
  "Vel.",	// SEQ_LAYER_V_VEL
  "Vel.",	// SEQ_LAYER_V_CHORD1_VEL
  "Vel.",	// SEQ_LAYER_V_CHORD2_VEL
  "Len.",	// SEQ_LAYER_V_LEN
  " CC "	// SEQ_LAYER_V_CC
};

static const seq_layer_ctrl_type_t seq_layer_v_ctrl_type_table[9] = {
  SEQ_LAYER_ControlType_None,      // SEQ_LAYER_V_NONE
  SEQ_LAYER_ControlType_Note,      // SEQ_LAYER_V_NOTE
  SEQ_LAYER_ControlType_Chord1,    // SEQ_LAYER_V_CHORD1
  SEQ_LAYER_ControlType_Chord2,    // SEQ_LAYER_V_CHORD2
  SEQ_LAYER_ControlType_Velocity,  // SEQ_LAYER_V_VEL
  SEQ_LAYER_ControlType_Velocity,  // SEQ_LAYER_V_CHORD1_VEL
  SEQ_LAYER_ControlType_Velocity,  // SEQ_LAYER_V_CHORD2_VEL
  SEQ_LAYER_ControlType_Length,    // SEQ_LAYER_V_LEN
  SEQ_LAYER_ControlType_CC         // SEQ_LAYER_V_CC
};


// constant types (the whole track only uses the specified value)
// Bit 3-0 is a unique number (0x0...0xf)
// Bit 6-4 select the MIDI event status byte, bit 7 must be 1!
//    0x8: not relevant for status byte (e.g. note length)
//    0x9: Note
//    0xb: CC
#define SEQ_LAYER_C_NONE	0x80
#define SEQ_LAYER_C_NOTE_A	0x91
#define SEQ_LAYER_C_NOTE_B	0x92
#define SEQ_LAYER_C_NOTE_C	0x93
#define SEQ_LAYER_C_VEL		0x94
#define SEQ_LAYER_C_LEN		0x05
#define SEQ_LAYER_C_CC_A	0xb6
#define SEQ_LAYER_C_CC_B	0xb7
#define SEQ_LAYER_C_CC_C	0xb8
#define SEQ_LAYER_C_CMEM_T	0x09

static const char seq_layer_c_names[10][5] = {
  "----",	// SEQ_LAYER_C_NONE
  "NteA",	// SEQ_LAYER_C_NOTE_A
  "NteB",	// SEQ_LAYER_C_NOTE_B
  "NteC",	// SEQ_LAYER_C_NOTE_C
  "Vel.",	// SEQ_LAYER_C_VEL
  "Len.",	// SEQ_LAYER_C_LEN
  "CC#A",	// SEQ_LAYER_C_CC_A
  "CC#B",	// SEQ_LAYER_C_CC_B
  "CC#C",	// SEQ_LAYER_C_CC_C
  "CM-T"	// SEQ_LAYER_C_CMEM_T
};

static const seq_layer_ctrl_type_t seq_layer_c_ctrl_type_table[10] = {
  SEQ_LAYER_ControlType_None,      // SEQ_LAYER_C_NONE
  SEQ_LAYER_ControlType_Note,      // SEQ_LAYER_C_NOTE_A
  SEQ_LAYER_ControlType_Note,      // SEQ_LAYER_C_NOTE_B
  SEQ_LAYER_ControlType_Note,      // SEQ_LAYER_C_NOTE_C
  SEQ_LAYER_ControlType_Velocity,  // SEQ_LAYER_C_VEL
  SEQ_LAYER_ControlType_Length,    // SEQ_LAYER_C_LEN
  SEQ_LAYER_ControlType_CC,        // SEQ_LAYER_C_CC_A
  SEQ_LAYER_ControlType_CC,        // SEQ_LAYER_C_CC_B
  SEQ_LAYER_ControlType_CC,        // SEQ_LAYER_C_CC_C
  SEQ_LAYER_ControlType_CMEM_T     // SEQ_LAYER_C_CMEM_T
};


// the assignments depending on the event mode
// note: on changes, adaptions have also to be made in SEQ_LAYER_GetEvnt_*!
static const u8 seq_layer_table[SEQ_LAYER_EVNTMODE_NUM][6] = {
  //      LayerA            LayerB            LayerC            Const1              Const2              Const3
  { SEQ_LAYER_V_NOTE, SEQ_LAYER_V_VEL,  SEQ_LAYER_V_LEN,  SEQ_LAYER_C_NONE,   SEQ_LAYER_C_NONE,   SEQ_LAYER_C_NONE },	 // 1
  { SEQ_LAYER_V_CHORD1,SEQ_LAYER_V_CHORD1_VEL, SEQ_LAYER_V_LEN, SEQ_LAYER_C_CMEM_T, SEQ_LAYER_C_NONE,  SEQ_LAYER_C_NONE }, // 2
  { SEQ_LAYER_V_CHORD2,SEQ_LAYER_V_CHORD2_VEL, SEQ_LAYER_V_LEN, SEQ_LAYER_C_NONE,   SEQ_LAYER_C_NONE,  SEQ_LAYER_C_NONE }, // 3
  { SEQ_LAYER_V_NOTE, SEQ_LAYER_V_NOTE, SEQ_LAYER_V_NOTE, SEQ_LAYER_C_VEL,    SEQ_LAYER_C_LEN,    SEQ_LAYER_C_NONE },	 // 4
  { SEQ_LAYER_V_VEL,  SEQ_LAYER_V_VEL,  SEQ_LAYER_V_VEL,  SEQ_LAYER_C_NOTE_A, SEQ_LAYER_C_NOTE_B, SEQ_LAYER_C_NOTE_C },  // 5
  { SEQ_LAYER_V_NOTE, SEQ_LAYER_V_VEL,  SEQ_LAYER_V_CC,   SEQ_LAYER_C_LEN,    SEQ_LAYER_C_NONE,   SEQ_LAYER_C_CC_C },	 // 6
  { SEQ_LAYER_V_NOTE, SEQ_LAYER_V_CC,   SEQ_LAYER_V_LEN,  SEQ_LAYER_C_VEL,    SEQ_LAYER_C_CC_B,   SEQ_LAYER_C_NONE },    // 7
  { SEQ_LAYER_V_NOTE, SEQ_LAYER_V_CC,   SEQ_LAYER_V_CC,   SEQ_LAYER_C_VEL,    SEQ_LAYER_C_CC_B,   SEQ_LAYER_C_CC_C },	 // 8
  { SEQ_LAYER_V_VEL,  SEQ_LAYER_V_CC,   SEQ_LAYER_V_CC,   SEQ_LAYER_C_NOTE_A, SEQ_LAYER_C_CC_B,   SEQ_LAYER_C_CC_C },	 // 9
  { SEQ_LAYER_V_CC,   SEQ_LAYER_V_CC,   SEQ_LAYER_V_LEN,  SEQ_LAYER_C_CC_A,   SEQ_LAYER_C_CC_B,   SEQ_LAYER_C_NONE },	 // 10
  { SEQ_LAYER_V_CC,   SEQ_LAYER_V_CC,   SEQ_LAYER_V_CC,   SEQ_LAYER_C_CC_A,   SEQ_LAYER_C_CC_B,   SEQ_LAYER_C_CC_C }	 // 11
};


// define which event modes provide a special "drum mode" behaviour
// Note: if more than 2 modes are used, SEQ_LAYER_CheckDrumMode has to be extented as well
#define SEQ_LAYER_DRUM_MODE_EM1	(5-1)
#define SEQ_LAYER_DRUM_MODE_EM2	(9-1)

// define which event modes provide polyphonic recording
// Note: if more than 1 mode is used, SEQ_LAYER_CheckPolyMode has to be extented as well
#define SEQ_LAYER_POLY_MODE_EM1	(4-1)

// maps layer A/B/C to the appr MIDI package number (0/1/2/4 - list entry of midi_packages returned by SEQ_LAYER_GetEvnt_x)
// used by SEQ_LAYER_GetEvntOfParLayer for visualisation purposes
static const u8 seq_layer_eventnum_map[SEQ_LAYER_EVNTMODE_NUM][3] = {
  // map A  B  C  
  {      0, 0, 0 }, // 1 - only one Note Event
  {      0, 0, 0 }, // 2 - Chord Event with 4 notes maximum
  {      0, 0, 0 }, // 3 - Chord Event with 4 notes maximum
  {      0, 1, 2 }, // 4 - three Note Events
  {      0, 1, 2 }, // 5 - three Note Events
  {      0, 0, 1 }, // 6 - one Note, one CC event
  {      0, 1, 0 }, // 7 - one Note, one CC event
  {      0, 1, 0 }, // 8 - one Note, two CC events
  {      0, 1, 2 }, // 9 - one Note, two CC events
  {      0, 1, 0 }, // 10 - two CC events
  {      0, 1, 2 }  // 11 - three CC events
};


// preset table - each entry contains the initial track configuration and the step settings
// EVNT0 and CHN won't be overwritten
// for four steps (they are copied to all other 4step groups)
// structure of track settings matches with the SEQ_TRKRECORD in app_defines.h

// parameters which will be reset to the given values in all event modes
static const u8 seq_layer_preset_table_static[][2] = {
  // parameter             value
  { SEQ_CC_MODE,           SEQ_CORE_TRKMODE_Normal },
  { SEQ_CC_MODE_FLAGS,     0x01 },
  { SEQ_CC_DIRECTION,	   0x00 },
  { SEQ_CC_STEPS_REPLAY,   0x00 },
  { SEQ_CC_STEPS_FORWARD,  0x00 },
  { SEQ_CC_STEPS_JMPBCK,   0x00 },
  { SEQ_CC_CLK_DIVIDER,    0x03 },
  { SEQ_CC_CLKDIV_FLAGS,   0x03 },
  { SEQ_CC_LENGTH,         0x0f },
  { SEQ_CC_LOOP,           0x00 },
  { SEQ_CC_TRANSPOSE_SEMI, 0x00 },
  { SEQ_CC_TRANSPOSE_OCT,  0x00 },
  { SEQ_CC_GROOVE_VALUE,   0x00 },
  { SEQ_CC_GROOVE_STYLE,   0x00 },
  { SEQ_CC_MORPH_MODE,     0x00 },
  { SEQ_CC_MORPH_SPARE,    0x00 },
  { SEQ_CC_HUMANIZE_VALUE, 0x00 },
  { SEQ_CC_HUMANIZE_MODE,  0x00 },
  { 0xff,                  0xff } // end marker
};

// parameters which will be selected depending on the event mode
static const u8 seq_layer_preset_table_mode[][SEQ_LAYER_EVNTMODE_NUM+1] = {
  // parameter                 1     2     3     4     5     6     7     8     9    10    11
  { SEQ_CC_MIDI_EVNT_CONST1, 0x00, 0x00, 0x00, 0x64, 0x24, 0x11, 0x64, 0x64, 0x3c, 0x01, 0x01 },
  { SEQ_CC_MIDI_EVNT_CONST2, 0x00, 0x00, 0x00, 0x11, 0x26, 0x00, 0x01, 0x01, 0x01, 0x10, 0x10 },
  { SEQ_CC_MIDI_EVNT_CONST3, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x01, 0x00, 0x10, 0x10, 0x00, 0x11 },
  { SEQ_CC_ASG_GATE,            1,    1,    1,    1,    0,    1,    1,    1,    0,    1,    1 },
  { SEQ_CC_ASG_SKIP,            0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { SEQ_CC_ASG_ACCENT,          2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2 },
  { SEQ_CC_ASG_GLIDE,           0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { SEQ_CC_ASG_ROLL,            3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3 },
  { SEQ_CC_ASG_RANDOM_GATE,     0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { SEQ_CC_ASG_RANDOM_VALUE,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
  { 0xff,                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }  // end marker
};


// trigger layer value presets
// each byte will be duplicated to 4 times (for 32 steps)
static const u8 seq_layer_preset_table_tlayer[SEQ_TRG_NUM_LAYERS][SEQ_LAYER_EVNTMODE_NUM] = {
  //  1     2     3     4     5     6     7     8     9    10    11
  { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 },
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};


// layer values - 4 entries per layer, they will be duplicated for all 32 steps per layer
// each mode has a package of 3*4 bytes
static const u8 seq_layer_preset_table_player[SEQ_LAYER_EVNTMODE_NUM][4*SEQ_PAR_NUM_LAYERS] = {
  //        Layer A                  Layer B                   Layer C
  { 0x3c, 0x3c, 0x3c, 0x3c,  0x64, 0x64, 0x64, 0x64,   0x11, 0x11, 0x11, 0x11 }, // 1
  { 0x40, 0x40, 0x40, 0x40,  0x64, 0x64, 0x64, 0x64,   0x11, 0x11, 0x11, 0x11 }, // 2
  { 0x40, 0x40, 0x40, 0x40,  0x64, 0x64, 0x64, 0x64,   0x11, 0x11, 0x11, 0x11 }, // 3
  { 0x3c, 0x3c, 0x3c, 0x3c,  0x3e, 0x3e, 0x3e, 0x3e,   0x40, 0x40, 0x40, 0x40 }, // 4
  { 0x64, 0x00, 0x00, 0x00,  0x64, 0x00, 0x00, 0x00,   0x64, 0x00, 0x00, 0x00 }, // 5
  { 0x3c, 0x3c, 0x3c, 0x3c,  0x64, 0x64, 0x64, 0x64,   0x40, 0x40, 0x40, 0x40 }, // 6
  { 0x3c, 0x3c, 0x3c, 0x3c,  0x40, 0x40, 0x40, 0x40,   0x11, 0x11, 0x11, 0x11 }, // 7
  { 0x3c, 0x3c, 0x3c, 0x3c,  0x40, 0x40, 0x40, 0x40,   0x40, 0x40, 0x40, 0x40 }, // 8
  { 0x64, 0x00, 0x00, 0x00,  0x40, 0x40, 0x40, 0x40,   0x40, 0x40, 0x40, 0x40 }, // 9
  { 0x40, 0x40, 0x40, 0x40,  0x40, 0x40, 0x40, 0x40,   0x11, 0x11, 0x11, 0x11 }, // 10
  { 0x40, 0x40, 0x40, 0x40,  0x40, 0x40, 0x40, 0x40,   0x40, 0x40, 0x40, 0x40 }  // 11
};


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local inline function: returns the variable type which is assigned to a layer
// depending on the event mode
/////////////////////////////////////////////////////////////////////////////
__inline static u8 SEQ_LAYER_GetVType(u8 event_mode, u8 par_layer)
{
  return seq_layer_table[event_mode][par_layer];
}


/////////////////////////////////////////////////////////////////////////////
// Local function: returns the constant type depending on the event mode
/////////////////////////////////////////////////////////////////////////////
__inline static u8 SEQ_LAYER_GetCType(u8 event_mode, u8 const_num)
{
  return seq_layer_table[event_mode][3 + const_num];
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the 4 character string of the variable type 
// depending on event mode and assigned layer
/////////////////////////////////////////////////////////////////////////////
char *SEQ_LAYER_VTypeStr(u8 event_mode, u8 par_layer)
{
  return (char *)seq_layer_v_names[SEQ_LAYER_GetVType(event_mode, par_layer) & 0xf];
}

/////////////////////////////////////////////////////////////////////////////
// This function returns the 4 character string of the constant type 
// depending on event mode and constant number
/////////////////////////////////////////////////////////////////////////////
char *SEQ_LAYER_CTypeStr(u8 event_mode, u8 const_num)
{
  return (char *)seq_layer_c_names[SEQ_LAYER_GetCType(event_mode, const_num) & 0xf];
}

/////////////////////////////////////////////////////////////////////////////
// This function returns the layer_evnt_t information based on given
// parameter layer (used for visualisation purposes)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_GetEvntOfParLayer(u8 track, u8 step, u8 par_layer, seq_layer_evnt_t *layer_event)
{
  seq_layer_evnt_t layer_events[4];
  s32 number_of_events = 0;

  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  if( tcc->evnt_mode < SEQ_LAYER_EVNTMODE_NUM ) {
    s32 (*getevnt_func)(u8 track, u8 step, seq_layer_evnt_t layer_events[4]) = seq_layer_getevnt_func[tcc->evnt_mode];
    number_of_events = getevnt_func(track, step, layer_events);
  }

  if( number_of_events <= 0 ) {
    // fill with dummy data
    layer_event->midi_package.ALL = 0;
    layer_event->len = 0;

    return -1; // no valid event
  }

  u8 event_num = seq_layer_eventnum_map[tcc->evnt_mode][par_layer];
  layer_event->midi_package = layer_events[event_num].midi_package;
  layer_event->len = layer_events[event_num].len;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function returns the control type of a variable layer
/////////////////////////////////////////////////////////////////////////////
seq_layer_ctrl_type_t SEQ_LAYER_GetVControlType(u8 track, u8 par_layer)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  if( tcc->evnt_mode >= SEQ_LAYER_EVNTMODE_NUM )
    return SEQ_LAYER_ControlType_None; // invalid event mode

  return seq_layer_v_ctrl_type_table[seq_layer_table[tcc->evnt_mode][par_layer] & 0xf];
}

/////////////////////////////////////////////////////////////////////////////
// This function returns the control type of a constant
/////////////////////////////////////////////////////////////////////////////
seq_layer_ctrl_type_t SEQ_LAYER_GetCControlType(u8 track, u8 const_ix)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  if( tcc->evnt_mode >= SEQ_LAYER_EVNTMODE_NUM )
    return SEQ_LAYER_ControlType_None; // invalid event mode

  return seq_layer_c_ctrl_type_table[seq_layer_table[tcc->evnt_mode][const_ix+3] & 0xf];
}



/////////////////////////////////////////////////////////////////////////////
// Layer Mode #1 - Layer: Note/Vel/Len, Constants: -/-/-
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LAYER_GetEvnt_1(u8 track, u8 step, seq_layer_evnt_t layer_events[4])
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  seq_layer_evnt_t *e = &layer_events[0];
  mios32_midi_package_t *p = &e->midi_package;
  p->type     = NoteOn;
  p->event    = NoteOn;
  p->chn      = tcc->midi_chn;
  p->note     = SEQ_PAR_Get(track, 0, step);
  p->velocity = SEQ_TRG_GateGet(track, step) ? SEQ_PAR_Get(track, 1, step) : 0x00;
  e->len      = SEQ_PAR_Get(track, 2, step) + 1;

  return 1;//events
}

/////////////////////////////////////////////////////////////////////////////
// Layer Mode #2 - Layer: Chord/Vel/Len, Constants: -/-/-
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LAYER_GetEvnt_2(u8 track, u8 step, seq_layer_evnt_t layer_events[4])
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  u8 cmem_track = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST1) % SEQ_CORE_NUM_TRACKS;

  int i;
  for(i=0; i<3; ++i) {
    seq_layer_evnt_t *e = &layer_events[i];
    mios32_midi_package_t *p = &e->midi_package;
    p->type     = NoteOn;
    p->event    = NoteOn;
    p->chn      = tcc->midi_chn;
    p->note     = SEQ_PAR_Get(cmem_track, i, SEQ_PAR_Get(track, 0, step) % SEQ_CORE_NUM_STEPS);
    p->velocity = SEQ_TRG_GateGet(track, step) ? SEQ_PAR_Get(track, 1, step) : 0x00;
    e->len      = SEQ_PAR_Get(track, 2, step) + 1;
  }

  return 3;//events
}

/////////////////////////////////////////////////////////////////////////////
// Layer Mode #3 - Layer: Chord/Vel/Len, Constants: -/-/-
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LAYER_GetEvnt_3(u8 track, u8 step, seq_layer_evnt_t layer_events[4])
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  return -1; // unimplemented layer mode
}

/////////////////////////////////////////////////////////////////////////////
// Layer Mode #4 - Layer: Note/Note/Note, Constants: Vel/Len/-
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LAYER_GetEvnt_4(u8 track, u8 step, seq_layer_evnt_t layer_events[4])
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  u8 gate = SEQ_TRG_GateGet(track, step);
  u8 velocity = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST1) || 1;
  u16 len = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST2) + 1;

  int i;
  for(i=0; i<3; ++i) {
    seq_layer_evnt_t *e = &layer_events[i];
    mios32_midi_package_t *p = &e->midi_package;
    p->type     = NoteOn;
    p->event    = NoteOn;
    p->chn      = tcc->midi_chn;
    p->note     = SEQ_PAR_Get(track, i, step);
    p->velocity = gate ? velocity : 0x00;
    e->len      = len;
  }

  return 3;//events
}

/////////////////////////////////////////////////////////////////////////////
// Layer Mode #5 - Layer: Vel/Vel/Vel, Constants: NoteA/NoteB/NoteC
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LAYER_GetEvnt_5(u8 track, u8 step, seq_layer_evnt_t layer_events[4])
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  u8 gate = SEQ_TRG_GateGet(track, step);

  int i;
  for(i=0; i<3; ++i) {
    seq_layer_evnt_t *e = &layer_events[i];
    mios32_midi_package_t *p = &e->midi_package;
    p->type     = NoteOn;
    p->event    = NoteOn;
    p->chn      = tcc->midi_chn;
    p->note     = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST1+i);
    p->velocity = gate ? SEQ_PAR_Get(track, i, step) : 0x00;
    e->len      = 16; // fixed length
  }

  return 3;//events
}

/////////////////////////////////////////////////////////////////////////////
// Layer Mode #6 - Layer: Note/Vel/CC, Constants: LenA/-/CC_C
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LAYER_GetEvnt_6(u8 track, u8 step, seq_layer_evnt_t layer_events[4])
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  {
    seq_layer_evnt_t *e = &layer_events[0];
    mios32_midi_package_t *p = &e->midi_package;
    p->type     = NoteOn;
    p->event    = NoteOn;
    p->chn      = tcc->midi_chn;
    p->note     = SEQ_PAR_Get(track, 0, step);
    p->velocity = SEQ_TRG_GateGet(track, step) ? SEQ_PAR_Get(track, 1, step) : 0x00;
    e->len      = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST1) + 1;
  }

  {
    seq_layer_evnt_t *e = &layer_events[1];
    mios32_midi_package_t *p = &e->midi_package;
    p->type      = CC;
    p->event     = CC;
    p->chn       = tcc->midi_chn;
    p->cc_number = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST3);
    p->value     = SEQ_PAR_Get(track, 2, step);
    e->len       = 0;
  }

  return 2;//events
}

/////////////////////////////////////////////////////////////////////////////
// Layer Mode #7 - Layer: Note/CC/Len, Constants: VelA/CC_B/-
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LAYER_GetEvnt_7(u8 track, u8 step, seq_layer_evnt_t layer_events[4])
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  {
    seq_layer_evnt_t *e = &layer_events[0];
    mios32_midi_package_t *p = &e->midi_package;
    p->type     = NoteOn;
    p->event    = NoteOn;
    p->chn      = tcc->midi_chn;
    p->note     = SEQ_PAR_Get(track, 0, step);
    p->velocity = SEQ_TRG_GateGet(track, step) ? (SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST1) || 1) : 0x00;
    e->len      = SEQ_PAR_Get(track, 2, step) + 1;
  }

  {
    seq_layer_evnt_t *e = &layer_events[1];
    mios32_midi_package_t *p = &e->midi_package;
    p->type      = CC;
    p->event     = CC;
    p->chn       = tcc->midi_chn;
    p->cc_number = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST2);
    p->value     = SEQ_PAR_Get(track, 1, step);
    e->len       = 0;
  }

  return 2;//events
}

/////////////////////////////////////////////////////////////////////////////
// Layer Mode #8 - Layer: Note/CC/CC, Constants: VelA/CC_B/CC_C
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LAYER_GetEvnt_8(u8 track, u8 step, seq_layer_evnt_t layer_events[4])
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  {
    seq_layer_evnt_t *e = &layer_events[0];
    mios32_midi_package_t *p = &e->midi_package;
    p->type     = NoteOn;
    p->event    = NoteOn;
    p->chn      = tcc->midi_chn;
    p->note     = SEQ_PAR_Get(track, 0, step);
    p->velocity = SEQ_TRG_GateGet(track, step) ? (SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST1) || 1) : 0x00;
    e->len      = 16; // constant
  }

  int i;
  for(i=0; i<2; ++i)
  {
    seq_layer_evnt_t *e = &layer_events[1+i];
    mios32_midi_package_t *p = &e->midi_package;
    p->type      = CC;
    p->event     = CC;
    p->chn       = tcc->midi_chn;
    p->cc_number = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST2+i);
    p->value     = SEQ_PAR_Get(track, 1+i, step);
    e->len       = 0;
  }

  return 3;//events
}

/////////////////////////////////////////////////////////////////////////////
// Layer Mode #9 - Layer: Vel/CC/CC, Constants: NoteA/CC_B/CC_C
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LAYER_GetEvnt_9(u8 track, u8 step, seq_layer_evnt_t layer_events[4])
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  {
    seq_layer_evnt_t *e = &layer_events[0];
    mios32_midi_package_t *p = &e->midi_package;
    p->type     = NoteOn;
    p->event    = NoteOn;
    p->chn      = tcc->midi_chn;
    p->note     = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST1);
    p->velocity = SEQ_TRG_GateGet(track, step) ? SEQ_PAR_Get(track, 0, step) : 0x00;
    e->len      = 16; // constant
  }

  int i;
  for(i=0; i<2; ++i)
  {
    seq_layer_evnt_t *e = &layer_events[1+i];
    mios32_midi_package_t *p = &e->midi_package;
    p->type      = CC;
    p->event     = CC;
    p->chn       = tcc->midi_chn;
    p->cc_number = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST2+i);
    p->value     = SEQ_PAR_Get(track, 1+i, step);
    e->len       = 0;
  }

  return 3;//events
}

/////////////////////////////////////////////////////////////////////////////
// Layer Mode #10 - Layer: CC/CC/Len, Constants: CC_A/CC_B/-
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LAYER_GetEvnt_10(u8 track, u8 step, seq_layer_evnt_t layer_events[4])
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  u8 gate = SEQ_TRG_GateGet(track, step);

  int i;
  for(i=0; i<2; ++i) {
    seq_layer_evnt_t *e = &layer_events[i];
    mios32_midi_package_t *p = &e->midi_package;
    p->type     = CC;
    p->event    = CC;
    p->chn      = tcc->midi_chn;
    p->cc_number = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST1+i);
    p->value    = SEQ_PAR_Get(track, i, step);
    e->len      = SEQ_PAR_Get(track, 2, step) + 1;
  }

  return 2;//events
}

/////////////////////////////////////////////////////////////////////////////
// Layer Mode #11 - Layer: CC/CC/CC, Constants: CC_A/CC_B/CC_C
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LAYER_GetEvnt_11(u8 track, u8 step, seq_layer_evnt_t layer_events[4])
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  u8 gate = SEQ_TRG_GateGet(track, step);

  int i;
  for(i=0; i<3; ++i) {
    seq_layer_evnt_t *e = &layer_events[i];
    mios32_midi_package_t *p = &e->midi_package;
    p->type     = CC;
    p->event    = CC;
    p->chn      = tcc->midi_chn;
    p->cc_number = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST1+i);
    p->value    = SEQ_PAR_Get(track, i, step);
    e->len      = 0;
  }

  return 3;//events
}


// putting pointer to these functions into a global array
// this approach should be faster than a switch statement
// and it's more flexible for future enhancements (e.g. attach layer mode to a certain step)
const void *seq_layer_getevnt_func[SEQ_LAYER_EVNTMODE_NUM] = {
  &SEQ_LAYER_GetEvnt_1,
  &SEQ_LAYER_GetEvnt_2,
  &SEQ_LAYER_GetEvnt_3,
  &SEQ_LAYER_GetEvnt_4,
  &SEQ_LAYER_GetEvnt_5,
  &SEQ_LAYER_GetEvnt_6,
  &SEQ_LAYER_GetEvnt_7,
  &SEQ_LAYER_GetEvnt_8,
  &SEQ_LAYER_GetEvnt_9,
  &SEQ_LAYER_GetEvnt_10,
  &SEQ_LAYER_GetEvnt_11
};
