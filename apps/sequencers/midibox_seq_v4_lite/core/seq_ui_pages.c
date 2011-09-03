// $Id$
/*
 * UI Menu Pages
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
#include <string.h>
#include "tasks.h"
#include "seq_ui.h"
#include "seq_ui_pages.h"

#include "seq_core.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_record.h"
#include "seq_file.h"
#include "seq_pattern.h"


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////
seq_ui_page_t ui_page;


/////////////////////////////////////////////////////////////////////////////
// presets (stored on SD Card)
/////////////////////////////////////////////////////////////////////////////

seq_ui_pages_progression_presets_t seq_ui_pages_progression_presets[16] = {
  // +CC  forward  jmpbck  replay  repeat  skip  rs_interval
  {    0,    1,      0,       0,     0,      0,       4     }, // GP#1 (off) -- also syncs to measure
  {    0,    4,      2,       0,     0,      0,       4     }, // GP#2
  {    0,    8,      4,       0,     0,      0,       4     }, // GP#3
  {    0,    8,      2,       1,     0,      0,       4     }, // GP#4
  {    0,    5,      2,       0,     0,      0,       4     }, // GP#5
  {    0,    5,      3,       0,     0,      0,       4     }, // GP#6
  {    0,    2,      3,       0,     0,      0,       4     }, // GP#7
  {    0,    2,      5,       0,     0,      0,       4     }, // GP#8
  {    0,    1,      0,       0,     1,      0,       4     }, // GP#9
  {    0,    1,      0,       0,     2,      0,       4     }, // GP#10
  {    0,    1,      0,       0,     1,      0,       8     }, // GP#11
  {    0,    1,      0,       0,     2,      0,       8     }, // GP#12
  {    0,    1,      0,       0,     0,      1,       4     }, // GP#13
  {    0,    1,      0,       0,     0,      2,       4     }, // GP#14
  {    0,    1,      0,       0,     0,      1,       8     }, // GP#15
  {    0,    1,      0,       0,     0,      2,       8     }, // GP#16
};

seq_ui_pages_echo_presets_t seq_ui_pages_echo_presets[16] = {
  // repeats  delay  velocity  fb_velocity  fb_note  fb_gatelength  fb_ticks
  {     0,      7,      15,        15,        24,          20,        20     }, // GP#1 (off)
  {     3,      5,      15,        15,        24,          20,        20     }, // GP#2
  {     3,      7,      15,        15,        24,          20,        20     }, // GP#3
  {     3,     19,      15,        15,        24,          20,        20     }, // GP#4
  {     3,      5,       5,        35,        24,          20,        20     }, // GP#5
  {     3,      7,       5,        35,        24,          20,        20     }, // GP#6
  {     3,     19,       5,        35,        24,          20,        20     }, // GP#7
  {     3,     19,      20,        15,        24,          20,        20     }, // GP#8
  {     3,      7,      15,        15,        27,          20,        20     }, // GP#9
  {     3,      7,      15,        15,        29,          20,        20     }, // GP#10
  {     3,     19,      15,        15,        27,          20,        20     }, // GP#11
  {     3,     19,      15,        15,        29,          20,        20     }, // GP#12
  {     3,      7,       5,        35,        27,          20,        20     }, // GP#13
  {     3,      7,       5,        35,        29,          20,        20     }, // GP#14
  {     3,     19,       5,        35,        27,          20,        20     }, // GP#15
  {     3,     19,       5,        35,        29,          20,        20     }, // GP#16
};

seq_ui_pages_lfo_presets_t seq_ui_pages_lfo_presets[16] = {
  // waveform amplitude phase steps steps_rst enable_flags cc cc_offset cc_ppqn
  {     0,      128+64,   0,   15,    15,        0x00,     0,    64,       6    }, // GP#1 (off)
  {     1,      128+12,   0,   15,    15,        0x0e,     0,    64,       6    }, // GP#2
  {     3,      128+24,   0,   15,    15,        0x0e,     0,    64,       6    }, // GP#3
  {     3,      128-12,   0,   15,    15,        0x06,     0,    64,       6    }, // GP#4
  {     3,      128+24,   0,   15,     3,        0x06,     0,    64,       6    }, // GP#5
  {     3,      128-12,   0,   31,    15,        0x06,     0,    64,       6    }, // GP#6
  {     3,      128+12,   0,   31,    15,        0x06,     0,    64,       6    }, // GP#7
  {     3,      128+12,   0,   63,    31,        0x06,     0,    64,       6    }, // GP#8

  {     1,      128+32,   0,   15,    15,        0x08,     1,    64,       6    }, // GP#9
  {     3,      128+64,   0,   15,    15,        0x08,     1,    64,       6    }, // GP#10
  {     3,      128+48,   0,   15,    15,        0x08,     1,    64,       6    }, // GP#11
  {     3,      128+48,   0,   15,     3,        0x08,     1,    64,       6    }, // GP#12
  {     3,      128+48,   0,   31,     3,        0x08,     1,    64,       6    }, // GP#13
  {     3,      128+48,   0,   31,    15,        0x08,     1,    64,       6    }, // GP#14
  {     3,      128+48,   0,   63,    31,        0x08,     1,    64,       6    }, // GP#15
  {     1,      128+48,   0,   63,    31,        0x08,     1,    64,       6    }, // GP#16
};


seq_ui_pages_humanizer_presets_t seq_ui_pages_humanizer_presets[16] = {
  // mode  value
  {  0x00,    0   }, // GP#1 (off)
  {  0x06,    8   }, // GP#2
  {  0x06,   16   }, // GP#3
  {  0x06,   24   }, // GP#4
  {  0x06,   32   }, // GP#5
  {  0x06,   40   }, // GP#6
  {  0x06,   48   }, // GP#7
  {  0x06,   64   }, // GP#8
  {  0x01,   12   }, // GP#9
  {  0x01,   16   }, // GP#10
  {  0x01,   24   }, // GP#11
  {  0x01,   36   }, // GP#12
  {  0x07,   12   }, // GP#13
  {  0x07,   16   }, // GP#14
  {  0x07,   24   }, // GP#15
  {  0x07,   36   }, // GP#16
};


u8 seq_ui_pages_scale_presets[16] = {
   0, // reserved, first position not used as it disabled force-to-scale
   0, // Major
   1, // Harmonic Minor
   2, // Melodic Minor
   3, // Natural Minor
   4, // Chromatic
   5, // Whole Tone
   6, // Pentatonic Major
   7, // Pentatonic Minor
   8, // Pentatonic Blues
   9, // Pentatonic Neutral
  10, // Octatonic (H-W)
  11, // Octatonic (W-H)
  12, // Ionian
  13, // Dorian
  14, // Phrygian
};


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u8 ui_selected_progression_preset;
static u8 ui_selected_echo_preset;
static u8 ui_selected_lfo_preset;
static u8 ui_selected_humanizer_preset;
static u8 ui_selected_scale;
static seq_pattern_t ui_selected_pattern[SEQ_CORE_NUM_GROUPS];
static u8 ui_selected_pattern_changing;
static u16 load_save_notifier_ctr;


/////////////////////////////////////////////////////////////////////////////
// Selects a new page
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_PAGES_Set(seq_ui_page_t page)
{
  // set page
  ui_page = page;

  // restart cursor counters
  ui_cursor_flash_ctr = ui_cursor_flash_overrun_ctr = 0;

  // special init depending on page (if required)
  switch( ui_page ) {

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_REC_STEP: {
    seq_record_state.ENABLED = 1;
    seq_record_options.STEP_RECORD = 1;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_REC_LIVE: {
    seq_record_state.ENABLED = 1;
    seq_record_options.STEP_RECORD = 0;
  } break;

  default:
    seq_record_state.ENABLED = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Frequently called to return the status of GP LEDs
/////////////////////////////////////////////////////////////////////////////
u16 SEQ_UI_PAGES_GP_LED_Handler(void)
{
  static u16 check_100mS_ctr = 0;

  if( load_save_notifier_ctr )
    --load_save_notifier_ctr;

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  // for periodic checks (e.g. of selections derived from patches)
  if( ++check_100mS_ctr >= 100 )
    check_100mS_ctr = 0;

  switch( ui_page ) {

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_LOAD:
  case SEQ_UI_PAGE_SAVE: {
    u8 group = 8;
    if( ui_selected_tracks & (1 << 0) )
      group = 0;

    u16 leds = (1 << ui_selected_pattern[group].group) | (1 << (ui_selected_pattern[group].num+8));
    if( ui_selected_pattern_changing && ui_cursor_flash )
      leds &= 0x00ff;

    // invert LEDs each 50 mS if load/save notifier active
    if( load_save_notifier_ctr && (load_save_notifier_ctr % 100) >= 50 )
      leds ^= 0xffff;

    return leds;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_TRIGGER: {
    u8 *trg_ptr = (u8 *)&seq_trg_layer_value[visible_track][2*ui_selected_step_view];
    u16 leds = *trg_ptr;
    ++trg_ptr;
    leds |= (*trg_ptr << 8);

    return leds;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_REC_STEP:
  case SEQ_UI_PAGE_REC_LIVE: {
    u8 record_track = 8;
    if( seq_record_state.ARMED_TRACKS & (1 << 0) )
      record_track = 0;

    u8 *trg_ptr = (u8 *)&seq_trg_layer_value[record_track][2*ui_selected_step_view];
    u16 leds = *trg_ptr;
    ++trg_ptr;
    leds |= (*trg_ptr << 8);

    u16 record_step_mask = (1 << (seq_record_step % 16));
    if( ui_cursor_flash &&
	((seq_record_step >> 4) == ui_selected_step_view) ) {
      leds ^= record_step_mask;
    }

    // button of selected step always active as long as GP button is pressed
    if( ui_selected_gp_buttons & record_step_mask )
      leds |= record_step_mask;

    return leds;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_LENGTH: {
    u8 length = SEQ_CC_Get(visible_track, SEQ_CC_LENGTH);
    if( length >= 16*(ui_selected_step_view+1) )
      return 0xffff;
    else if( (length >> 4) == ui_selected_step_view )
      return (1 << ((length % 16)+1))-1;
    else
      return 0x0000;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_PROGRESSION: {
    // check if selection still valid
    if( check_100mS_ctr == 0 ) {
      seq_cc_trk_t *tcc = &seq_cc_trk[visible_track];
      seq_ui_pages_progression_presets_t *preset = (seq_ui_pages_progression_presets_t *)&seq_ui_pages_progression_presets[0];
      int i;
      for(i=0; i<16; ++i, ++preset) {
	if( ((u8)tcc->steps_forward+1) == preset->steps_forward &&
	    tcc->steps_jump_back == preset->steps_jump_back &&
	    tcc->steps_replay == preset->steps_replay &&
	    tcc->steps_repeat == preset->steps_repeat &&
	    tcc->steps_skip == preset->steps_skip &&
	    tcc->steps_rs_interval == preset->steps_rs_interval ) {
	  ui_selected_progression_preset = i;
	  break;
	}
      }
    }

    return (1 << ui_selected_progression_preset);
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_GROOVE: {
    u8 groove_style = SEQ_CC_Get(visible_track, SEQ_CC_GROOVE_STYLE);
    if( groove_style < 8 || groove_style > 22 )
      return 0x0001; // off resp. non-selectable value
    else // note: starting at second custom groove, the first groove is always "off"
      return (1 << (groove_style-8+1));
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_ECHO: {
    // check if selection still valid
    if( check_100mS_ctr == 0 ) {
      seq_cc_trk_t *tcc = &seq_cc_trk[visible_track];
      seq_ui_pages_echo_presets_t *preset = (seq_ui_pages_echo_presets_t *)&seq_ui_pages_echo_presets[0];
      int i;
      for(i=0; i<16; ++i, ++preset) {
	if( tcc->echo_repeats == preset->repeats &&
	    tcc->echo_delay == preset->delay &&
	    tcc->echo_velocity == preset->velocity &&
	    tcc->echo_fb_velocity == preset->fb_velocity &&
	    tcc->echo_fb_note == preset->fb_note &&
	    tcc->echo_fb_gatelength == preset->fb_gatelength &&
	    tcc->echo_fb_ticks == preset->fb_ticks ) {
	  ui_selected_echo_preset = i;
	  break;
	}
      }
    }

    return (1 << ui_selected_echo_preset);
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_HUMANIZER: {
    // check if selection still valid
    if( check_100mS_ctr == 0 ) {
      seq_cc_trk_t *tcc = &seq_cc_trk[visible_track];
      seq_ui_pages_humanizer_presets_t *preset = (seq_ui_pages_humanizer_presets_t *)&seq_ui_pages_humanizer_presets[0];
      int i;
      for(i=0; i<16; ++i, ++preset) {
	if( tcc->humanize_mode == preset->mode &&
	    tcc->humanize_value == preset->value ) {
	  ui_selected_humanizer_preset = i;
	  break;
	}
      }
    }

    return (1 << ui_selected_humanizer_preset);
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_LFO: {
    // check if selection still valid
    if( check_100mS_ctr == 0 ) {
      seq_cc_trk_t *tcc = &seq_cc_trk[visible_track];
      seq_ui_pages_lfo_presets_t *preset = (seq_ui_pages_lfo_presets_t *)&seq_ui_pages_lfo_presets[0];
      int i;
      for(i=0; i<16; ++i, ++preset) {
	if( tcc->lfo_waveform == preset->waveform &&
	    tcc->lfo_amplitude == preset->amplitude &&
	    tcc->lfo_phase == preset->phase &&
	    tcc->lfo_steps == preset->steps &&
	    tcc->lfo_steps_rst == preset->steps_rst &&
	    tcc->lfo_enable_flags.ALL == preset->enable_flags &&
	    tcc->lfo_cc == preset->cc &&
	    tcc->lfo_cc_offset == preset->cc_offset &&
	    tcc->lfo_cc_ppqn == preset->cc_ppqn ) {
	  ui_selected_lfo_preset = i;
	  break;
	}
      }
    }

    return (1 << ui_selected_lfo_preset);
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_SCALE: {
    // check if selection still valid
    if( check_100mS_ctr == 0 ) {
      u8 note_track = 8;
      if( ui_selected_tracks & (1 << 0) )
	note_track = 0;

      if( !seq_cc_trk[note_track].mode.FORCE_SCALE )
	ui_selected_scale = 0;
      else {
	u8 *preset = (u8 *)&seq_ui_pages_scale_presets[1];
	int i;
	for(i=1; i<16; ++i, ++preset)
	  if( seq_core_global_scale == *preset )
	    break;

	ui_selected_scale = (i > 15) ? 15 : i; // if no preset scale, show last LED
      }
    }

    return (1 << ui_selected_scale);
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_MUTE: {
    u16 muted = seq_core_trk_muted;
    if( ui_cursor_flash && seq_ui_button_state.SOLO ) {
      //muted |= !ui_selected_tracks; // doesn't work with gcc version 4.2.1 ?!?
      muted |= ui_selected_tracks ^ 0xffff;
    }
    return muted;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_MIDICHN: {
    u8 chn = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_CHANNEL);
    return (1 << chn);
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_REC_ARM: {
    return seq_record_state.ARMED_TRACKS;
  } break;

  }

  return 0x0000;
}


/////////////////////////////////////////////////////////////////////////////
// Called when a GP button has been toggled
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_PAGES_GP_Button_Handler(u8 button, u8 depressed)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  // no page reacts on depressed buttons
  if( depressed )
    return 0;

  switch( ui_page ) {

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_LOAD:
  case SEQ_UI_PAGE_SAVE: {
    // not atomic so that files can be stored in background
    //portENTER_CRITICAL();

    u8 group;
    for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
      seq_pattern_t *pattern = &ui_selected_pattern[group];

      pattern->bank = group; // always same as group
      if( button < 8 ) {
	pattern->group = button;
	ui_selected_pattern_changing = 1;
      } else {
	pattern->num = button-8;
	ui_selected_pattern_changing = 0;

	if( ui_page == SEQ_UI_PAGE_SAVE ) {
	  //DEBUG_MSG("BEGIN Save %d:%c%d\n", pattern->bank+1, 'A'+pattern->group, pattern->num+1);
	  s32 status = 0;
	  if( (status=SEQ_PATTERN_Save(group, *pattern)) < 0 )
	    SEQ_UI_SDCardErrMsg(2000, status);
	  else
	    load_save_notifier_ctr = 300; // notify about save operation for 300 mS
	  //DEBUG_MSG("END   Save %d:%c%d\n", pattern->bank+1, 'A'+pattern->group, pattern->num+1);
	} else {
	  //DEBUG_MSG("BEGIN Load %d:%c%d\n", pattern->bank+1, 'A'+pattern->group, pattern->num+1);
	  SEQ_PATTERN_Change(group, *pattern, 0);
	  load_save_notifier_ctr = 300; // notify about load operation for 300 mS
	  //DEBUG_MSG("END   Load %d:%c%d\n", pattern->bank+1, 'A'+pattern->group, pattern->num+1);
	}
      }
    }

    //portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_TRIGGER: {
    // should be atomic
    portENTER_CRITICAL();

    ui_selected_step = button;

    u8 track; // only change triggers if track 0 and 8
    for(track=0; track<SEQ_CORE_NUM_TRACKS; track+=8)
      if( ui_selected_tracks & (1 << track) ) {
	u8 *trg_ptr = (u8 *)&seq_trg_layer_value[track][2*ui_selected_step_view + (button>>3)];
	*trg_ptr ^= (1 << (button&7));
      }

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_LENGTH: {
    // should be atomic
    portENTER_CRITICAL();

    u8 track;
    for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
      if( ui_selected_tracks & (1 << track) ) {
	if( (track >= 0 && track <= 2) || (track >= 8 && track <= 10) )
	  SEQ_CC_Set(track, SEQ_CC_LENGTH, 16*ui_selected_step_view + button);
	else
	  SEQ_CC_Set(track, SEQ_CC_LENGTH, 64*ui_selected_step_view + 4*button + 3); // 4x resolution
      }
    }

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_PROGRESSION: {
    // should be atomic
    portENTER_CRITICAL();

    ui_selected_progression_preset = button;
    seq_ui_pages_progression_presets_t *preset = (seq_ui_pages_progression_presets_t *)&seq_ui_pages_progression_presets[ui_selected_progression_preset];
    SEQ_CC_Set(visible_track, SEQ_CC_STEPS_FORWARD, (preset->steps_forward > 0) ? (preset->steps_forward-1) : 0);
    SEQ_CC_Set(visible_track, SEQ_CC_STEPS_JMPBCK, preset->steps_jump_back);
    SEQ_CC_Set(visible_track, SEQ_CC_STEPS_REPLAY, preset->steps_replay);
    SEQ_CC_Set(visible_track, SEQ_CC_STEPS_REPEAT, preset->steps_repeat);
    SEQ_CC_Set(visible_track, SEQ_CC_STEPS_SKIP, preset->steps_skip);
    SEQ_CC_Set(visible_track, SEQ_CC_STEPS_RS_INTERVAL, preset->steps_rs_interval);

    // GP1 also requests synch to measure
    // it's a good idea to do this for all tracks so that both sequences are in synch again
    if( button == 0 )
      SEQ_CORE_ManualSynchToMeasure(0xffff);

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_GROOVE: {
    // should be atomic
    portENTER_CRITICAL();

    // first button turns off groove function
    // remaining buttons select custom groove #1..15
    u8 groove_style = 0;
    if( button > 0 ) // note: starting at second custom groove, the first groove is always "off"
      groove_style = button+8-1;
    SEQ_UI_CC_Set(SEQ_CC_GROOVE_STYLE, groove_style);
    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_ECHO: {
    // should be atomic
    portENTER_CRITICAL();

    ui_selected_echo_preset = button;
    seq_ui_pages_echo_presets_t *preset = (seq_ui_pages_echo_presets_t *)&seq_ui_pages_echo_presets[ui_selected_echo_preset];
    SEQ_CC_Set(visible_track, SEQ_CC_ECHO_REPEATS, preset->repeats);
    SEQ_CC_Set(visible_track, SEQ_CC_ECHO_DELAY, preset->delay);
    SEQ_CC_Set(visible_track, SEQ_CC_ECHO_VELOCITY, preset->velocity);
    SEQ_CC_Set(visible_track, SEQ_CC_ECHO_FB_VELOCITY, preset->fb_velocity);
    SEQ_CC_Set(visible_track, SEQ_CC_ECHO_FB_NOTE, preset->fb_note);
    SEQ_CC_Set(visible_track, SEQ_CC_ECHO_FB_GATELENGTH, preset->fb_gatelength);
    SEQ_CC_Set(visible_track, SEQ_CC_ECHO_FB_TICKS, preset->fb_ticks);

    portEXIT_CRITICAL();
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_HUMANIZER: {
    // should be atomic
    portENTER_CRITICAL();

    ui_selected_humanizer_preset = button;
    seq_ui_pages_humanizer_presets_t *preset = (seq_ui_pages_humanizer_presets_t *)&seq_ui_pages_humanizer_presets[ui_selected_humanizer_preset];
    SEQ_CC_Set(visible_track, SEQ_CC_HUMANIZE_MODE, preset->mode);
    SEQ_CC_Set(visible_track, SEQ_CC_HUMANIZE_VALUE, preset->value);

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_LFO: {
    // should be atomic
    portENTER_CRITICAL();

    ui_selected_lfo_preset = button;
    seq_ui_pages_lfo_presets_t *preset = (seq_ui_pages_lfo_presets_t *)&seq_ui_pages_lfo_presets[ui_selected_lfo_preset];
    SEQ_CC_Set(visible_track, SEQ_CC_LFO_WAVEFORM, preset->waveform);
    SEQ_CC_Set(visible_track, SEQ_CC_LFO_AMPLITUDE, preset->amplitude);
    SEQ_CC_Set(visible_track, SEQ_CC_LFO_PHASE, preset->phase);
    SEQ_CC_Set(visible_track, SEQ_CC_LFO_STEPS, preset->steps);
    SEQ_CC_Set(visible_track, SEQ_CC_LFO_STEPS_RST, preset->steps_rst);
    SEQ_CC_Set(visible_track, SEQ_CC_LFO_ENABLE_FLAGS, preset->enable_flags);
    SEQ_CC_Set(visible_track, SEQ_CC_LFO_CC, preset->cc);
    SEQ_CC_Set(visible_track, SEQ_CC_LFO_CC_OFFSET, preset->cc_offset);
    SEQ_CC_Set(visible_track, SEQ_CC_LFO_CC_PPQN, preset->cc_ppqn);

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_SCALE: {
    // should be atomic
    portENTER_CRITICAL();

    ui_selected_scale = button;

    if( ui_selected_scale == 0 ) {
      // disable force-to-scale for both note tracks (makes sense, since the scale itself is global as well)
      u8 track;
      for(track=0; track<SEQ_CORE_NUM_TRACKS; track+=8)
	seq_cc_trk[track].mode.FORCE_SCALE = 0;
    } else {
      // select scale
      seq_core_global_scale = seq_ui_pages_scale_presets[ui_selected_scale];

      // enable force-to-scale for both note tracks (makes sense, since the scale itself is global as well)
      u8 track;
      for(track=0; track<SEQ_CORE_NUM_TRACKS; track+=8)
	seq_cc_trk[track].mode.FORCE_SCALE = 1;
    }

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_MUTE: {
    // should be atomic
    portENTER_CRITICAL();

    seq_core_trk_muted ^= (1 << button);

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_MIDICHN: {
    // should be atomic
    portENTER_CRITICAL();

    u8 track;
    for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
      SEQ_CC_Set(track, SEQ_CC_MIDI_CHANNEL, button);

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_REC_ARM: {
    // should be atomic
    portENTER_CRITICAL();

    // allow to select an unplayed track
    if( button >= 8 && (seq_record_state.ARMED_TRACKS & 0x00ff) )
      seq_record_state.ARMED_TRACKS = 0xff00;
    else if( button < 8 && (seq_record_state.ARMED_TRACKS & 0xff00) )
      seq_record_state.ARMED_TRACKS = 0x00ff;
    else {
      seq_record_state.ARMED_TRACKS ^= (1 << button);
    }

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_REC_STEP: {
  case SEQ_UI_PAGE_REC_LIVE:
    // should be atomic
    portENTER_CRITICAL();

    seq_record_step = 16*ui_selected_step_view + button;

    // we will always clear the current step for more comfortable handling
    // (no need to select the Trigger page for doing this)
    u8 track; // only change triggers if track 0 and 8
    for(track=0; track<SEQ_CORE_NUM_TRACKS; track+=8)
      if( seq_record_state.ARMED_TRACKS & (1 << track) ) {
	u8 *trg_ptr = (u8 *)&seq_trg_layer_value[track][2*ui_selected_step_view + (button>>3)];
	*trg_ptr &= ~(1 << (button&7));

	SEQ_RECORD_Reset(track);
      }

    portEXIT_CRITICAL();
    return 0;
  } break;
  }

  return -1; // unsupported page
}

