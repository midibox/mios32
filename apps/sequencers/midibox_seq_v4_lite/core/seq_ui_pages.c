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


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////
seq_ui_page_t ui_page;


/////////////////////////////////////////////////////////////////////////////
// presets (stored on SD Card)
/////////////////////////////////////////////////////////////////////////////

seq_ui_pages_echo_presets_t seq_ui_pages_echo_presets[16] = {
  // repeats  delay  velocity  fb_velocity  fb_note  fb_gatelength  fb_ticks
  {     0,      7,      15,        15,        24,          20,        20     }, // GP#1 (off)
  {     3,      7,      15,        15,        24,          20,        20     }, // GP#2
  {     3,      7,      15,        15,        27,          20,        20     }, // GP#3
  {     3,      7,      15,        15,        36,          20,        20     }, // GP#4
  {     3,      5,      15,        15,        24,          20,        20     }, // GP#5
  {     3,      5,      15,        15,        27,          20,        20     }, // GP#6
  {     3,      5,      15,        15,        29,          20,        20     }, // GP#7
  {     3,      3,      15,        15,        36,          20,        20     }, // GP#8
  {     3,     19,      15,        15,        24,          20,        20     }, // GP#9
  {     3,     19,      15,        15,        27,          20,        20     }, // GP#10
  {     3,     19,      15,        15,        29,          20,        20     }, // GP#11
  {     3,     19,      15,        15,        36,          20,        10     }, // GP#12
  {     3,     19,      15,        15,        36,          20,        30     }, // GP#13
  {     3,     19,      15,        15,        27,          20,        10     }, // GP#14
  {     5,     19,      15,        15,        28,          20,        10     }, // GP#15
  {     5,     19,      15,        15,        29,          20,        10     }, // GP#16
};


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u8 ui_selected_echo_preset;


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
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  // for periodic checks (e.g. of selections derived from patches)
  if( ++check_100mS_ctr >= 100 )
    check_100mS_ctr = 0;

  switch( ui_page ) {

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_LOAD: {
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_SAVE: {
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_TRIGGER:
  case SEQ_UI_PAGE_REC_STEP:
  case SEQ_UI_PAGE_REC_LIVE: {
    u8 *trg_ptr = (u8 *)&seq_trg_layer_value[visible_track][2*ui_selected_step_view];
    u16 leds = *trg_ptr;
    ++trg_ptr;
    leds |= (*trg_ptr << 8);

    // recording page: flash record position
    if( !ui_cursor_flash &&
	(ui_page == SEQ_UI_PAGE_REC_STEP || ui_page == SEQ_UI_PAGE_REC_LIVE) &&
	((seq_record_step >> 4) == ui_selected_step_view) ) {
      leds ^= (1 << (seq_record_step % 16));
    }

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
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_GROOVE: {
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
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_LFO: {
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_SCALE: {
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_MUTE: {
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_MIDICHN: {
    u8 chn = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_CHANNEL);
    return (1 << chn);
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_REC_ARM: {
    return seq_record_armed_tracks;
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
  case SEQ_UI_PAGE_LOAD: {
    // should be atomic
    portENTER_CRITICAL();

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_SAVE: {
    // should be atomic
    portENTER_CRITICAL();

    portEXIT_CRITICAL();
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

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_GROOVE: {
    // should be atomic
    portENTER_CRITICAL();

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

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_LFO: {
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_SCALE: {
    // should be atomic
    portENTER_CRITICAL();

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_MUTE: {
    // should be atomic
    portENTER_CRITICAL();

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
    if( button >= 8 && (seq_record_armed_tracks & 0x00ff) )
      seq_record_armed_tracks = 0xff00;
    else if( button < 8 && (seq_record_armed_tracks & 0xff00) )
      seq_record_armed_tracks = 0x00ff;
    else {
      seq_record_armed_tracks ^= (1 << button);
    }

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_REC_STEP: {
    // should be atomic
    portENTER_CRITICAL();

    seq_record_step = 16*ui_selected_step_view + button;

    portEXIT_CRITICAL();
    return 0;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SEQ_UI_PAGE_REC_LIVE: {
    // should be atomic
    portENTER_CRITICAL();

    portEXIT_CRITICAL();
    return 0;
  } break;
  }

  return -1; // unsupported page
}

