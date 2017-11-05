// $Id$
/*
 * Pattern page
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
#include "seq_lcd.h"
#include "seq_ui.h"
#include "tasks.h"
#include "file.h"

#include "seq_file.h"
#include "seq_file_b.h"
#include "seq_file_t.h"
#include "seq_bpm.h"
#include "seq_core.h"
#include "seq_midi_in.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       4
#define ITEM_PATTERN_G1    0
#define ITEM_PATTERN_G2    1
#define ITEM_PATTERN_G3    2
#define ITEM_PATTERN_G4    3


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_pattern_t selected_pattern[SEQ_CORE_NUM_GROUPS];


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( SEQ_FILE_FormattingRequired() )
    return 0; // no LED action as long as files not available

  seq_pattern_t pattern = (ui_selected_item == (ITEM_PATTERN_G1 + ui_selected_group) && ui_cursor_flash) 
    ? seq_pattern[ui_selected_group] : selected_pattern[ui_selected_group];

  *gp_leds = (1 << pattern.group) | (1 << (pattern.num+8));

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local encoder callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported encoder
/////////////////////////////////////////////////////////////////////////////
static s32 Encoder_Handler(seq_ui_encoder_t encoder, s32 incrementer)
{
  if( SEQ_FILE_FormattingRequired() )
    return 0; // no encoder action as long as files not available

  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
    case SEQ_UI_ENCODER_GP2: {
      ui_selected_group = 0; // select group
      return 1;
    } break;

    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_PATTERN_G1;
      break;

    case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6: {
      ui_selected_group = 1; // select group
      return 1;
    } break;

    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_PATTERN_G2;
      break;

    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10: {
      ui_selected_group = 2; // select group
      return 1;
    } break;

    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
      ui_selected_item = ITEM_PATTERN_G3;
      break;

    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14: {
      ui_selected_group = 3; // select group
      return 1;
    } break;

    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      ui_selected_item = ITEM_PATTERN_G4;
      break;

    case SEQ_UI_ENCODER_Datawheel:
      return -1; // not mapped yet

    default:
      return -1; // invalid or unsupported encoder
  }

  // take over new group
  ui_selected_group = ui_selected_item;

  // change bank/pattern number(s)
  seq_pattern_t *pattern = &selected_pattern[ui_selected_group];
  if( !seq_ui_options.MODIFY_PATTERN_BANKS || (encoder & 1) ) {
    u8 tmp = pattern->pattern;
    u8 max_patterns = SEQ_FILE_B_NumPatterns(pattern->bank);
    // TODO: print error message if bank not valid (max_patterns = 0)
    if( !max_patterns || !SEQ_UI_Var8_Inc(&tmp, 0, max_patterns-1, incrementer) ) {
      return 0; // no change
    }
    pattern->pattern = tmp;
  } else {
    u8 tmp = pattern->bank;
    if( !SEQ_UI_Var8_Inc(&tmp, 0, SEQ_FILE_B_NUM_BANKS-1, incrementer) ) {
      return 0; // no change
    }
    pattern->bank = tmp;
  }

  u8 is_critical = SEQ_BPM_IsRunning();
  if( is_critical )
    portENTER_CRITICAL();
  int group;
  for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
    if( seq_ui_button_state.CHANGE_ALL_STEPS || group == ui_selected_group ) {
      if( encoder & 1 ) {
	selected_pattern[group].pattern = pattern->pattern;

      } else {
	// in order to avoid accidents the bank won't be changed
	// because normally each group has it's dedicated bank
#if 0
	selected_pattern[group].bank = pattern->bank;
#endif
      }

      SEQ_PATTERN_Change(group, selected_pattern[group], 0);
    }
  }
  if( is_critical )
    portEXIT_CRITICAL();

  // send to external
  {
    int group;
    for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
      if( seq_ui_button_state.CHANGE_ALL_STEPS || group == ui_selected_group ) {
	SEQ_MIDI_IN_ExtCtrlSend(SEQ_MIDI_IN_EXT_CTRL_PATTERN_G1 + group, selected_pattern[group].pattern, 0);
	SEQ_MIDI_IN_ExtCtrlSend(SEQ_MIDI_IN_EXT_CTRL_BANK_G1 + group, selected_pattern[group].bank, 0);
      }
    }
  }


  return 1; // value as been changed
}


/////////////////////////////////////////////////////////////////////////////
// Local button callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported button
/////////////////////////////////////////////////////////////////////////////
static s32 Button_Handler(seq_ui_button_t button, s32 depressed)
{
  if( depressed ) return 0; // ignore when button depressed

  if( SEQ_FILE_FormattingRequired() )
    return 0; // no button action as long as files not available

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP8 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP8 ) {
#endif
    if( selected_pattern[ui_selected_group].group == button ) {
      if( SEQ_FILE_B_NumPatterns(selected_pattern[ui_selected_group].bank) > 64 ) {
	selected_pattern[ui_selected_group].lower ^= 1;
      } else {
	selected_pattern[ui_selected_group].lower = 0; // toggling not required
      }
    } else {
      selected_pattern[ui_selected_group].group = button;
    }

    if( seq_ui_button_state.CHANGE_ALL_STEPS ) {
      int group;
      for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group)
	selected_pattern[group].pattern = selected_pattern[ui_selected_group].pattern;
    }

    return 1; // value always changed
  }

  if( button >= SEQ_UI_BUTTON_GP9 && button <= SEQ_UI_BUTTON_GP16 ) {
    int group;
    for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
      if( seq_ui_button_state.CHANGE_ALL_STEPS || group == ui_selected_group ) {
	selected_pattern[group].num = button-8;

	// send to external
	SEQ_MIDI_IN_ExtCtrlSend(SEQ_MIDI_IN_EXT_CTRL_PATTERN_G1 + group, selected_pattern[group].pattern, 0);
	SEQ_MIDI_IN_ExtCtrlSend(SEQ_MIDI_IN_EXT_CTRL_BANK_G1 + group, selected_pattern[group].bank, 0);

	SEQ_PATTERN_Change(group, selected_pattern[group], 0);
      }
    }

    return 1; // value always changed
  }


  switch( button ) {
    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
      if( ++ui_selected_item >= NUM_OF_ITEMS )
	ui_selected_item = 0;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
      else
	--ui_selected_item;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Up:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Down:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);
  }

  return -1; // invalid or unsupported button
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  // take over selected group -> item
  ui_selected_item = ui_selected_group;

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // G1: xxxxxxxxxxxxxxx  G2: xxxxxxxxxxxxxxxG3: xxxxxxxxxxxxxxx  G4: xxxxxxxxxxxxxxx
  // >>> xxxxx 1:A1 ____      xxxxx 2:A1 ____    xxxxx 3:A1 ____      xxxxx 4:A1 ____                                         


  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // G1: Breakbeats 2     G2: Lovely Arps 5  G3: My fav.Bassline  G4: Transposer Emaj 
  // >>> Drums 1:B2 ____      Synth 2:A5 ____    Bass  3:B1 ____      Ctrl 4:C1 ____                                         


  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  //     No Patterns available as long as theSession hasn't been created!            
  //                   Please press EXIT and create a new Session!                   

  if( SEQ_FILE_FormattingRequired() ) {
    if( high_prio )
      return 0;

    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintString("    No Patterns available as long as theSession hasn't been created!            ");
    SEQ_LCD_CursorSet(0, 1);
    SEQ_LCD_PrintString("                  Please press EXIT and create a new Session!                   ");
    return 0;
  }

  if( high_prio ) {
    ///////////////////////////////////////////////////////////////////////////
    // frequently update VU meters
    u8 track;
    seq_core_trk_t *t = &seq_core_trk[0];
    for(track=0; track<16; ++t, ++track) {
      if( !(track % 4) )
	SEQ_LCD_CursorSet(15 + 20*(track>>2), 1);

      if( seq_core_trk_muted & (1 << track) )
	SEQ_LCD_PrintVBar('M');
      else
	SEQ_LCD_PrintVBar(t->vu_meter >> 4);
    }
  } else {
    ///////////////////////////////////////////////////////////////////////////
    u8 group;
    for(group=0; group<4; ++group) {
      SEQ_LCD_CursorSet(20*group, 0);
      if( group % 1 )
	SEQ_LCD_PrintSpaces(1);

      SEQ_LCD_PrintFormattedString("G%d: ", group+1);
      SEQ_LCD_PrintPatternLabel(seq_pattern[group], seq_pattern_name[group]);

      if( !(group % 1) )
	SEQ_LCD_PrintSpaces(1);

      SEQ_LCD_CursorSet(20*group, 1);

      if( group % 1 )
	SEQ_LCD_PrintSpaces(1);

      if( seq_ui_button_state.CHANGE_ALL_STEPS || group == ui_selected_group )
	SEQ_LCD_PrintString(">>>");
      else
	SEQ_LCD_PrintSpaces(3);

      SEQ_LCD_PrintSpaces(1);
      SEQ_LCD_PrintPatternCategory(seq_pattern[group], seq_pattern_name[group]);
      SEQ_LCD_PrintSpaces(1);


      // shortly show current pattern
      seq_pattern_t pattern = ((seq_ui_button_state.CHANGE_ALL_STEPS || (ui_selected_item == (ITEM_PATTERN_G1 + group))) && ui_cursor_flash) ? seq_pattern[group] : selected_pattern[group];
      SEQ_LCD_PrintFormattedString("%d:", pattern.bank + 1);

      if( pattern.pattern < SEQ_FILE_B_NumPatterns(pattern.bank) )
	SEQ_LCD_PrintPattern(pattern);
      else
	SEQ_LCD_PrintString("!!"); // covers the case that bank is not available, or that pattern number too high

      if( selected_pattern[group].pattern != seq_pattern[group].pattern ||
	  selected_pattern[group].bank != seq_pattern[group].bank )
	SEQ_LCD_PrintChar('*');
      else
	SEQ_LCD_PrintChar(' ');

      if( !(group % 1) ) {
	SEQ_LCD_CursorSet(20*group+19, 1);
	SEQ_LCD_PrintSpaces(1);
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_PATTERN_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // we want to show vertical VU meters
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBars);

  // init selection patterns
  u8 group;
  for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group)
    selected_pattern[group] = seq_pattern[group];

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Copy Patterns
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_PATTERN_MultiCopy(u8 only_selected)
{
  s32 status = 0;
  char path[30];

  // create directory if it doesn't exist
  strcpy(path, "/PRESETS");
  MUTEX_SDCARD_TAKE;
  status = FILE_MakeDir(path);
  status = FILE_DirExists(path);
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
    return -3;
  }

  if( status == 0 ) {
    SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "/PRESETS directory", "cannot be created!");
    return -4;
  }

  // copy all selected patterns to preset directory
  u8 track, track_id;
  for(track=0, track_id=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    if( !only_selected || (ui_selected_tracks & (1 << track)) ) {
      ++track_id;
      sprintf(path, "/PRESETS/COPY%d.v4t", track_id);

      char str[30];
      sprintf(str, "Exporting G%dT%d to:", (track/4)+1, (track%4)+1);
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, str, path);

      MUTEX_SDCARD_TAKE;
      status=SEQ_FILE_T_Write(path, track);
      MUTEX_SDCARD_GIVE;

      if( status < 0 ) {
	SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "Error during Export!", "see MIOS Terminal!");
	return -6;
      }
    }
  }

  if( !track_id ) {
    SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "No Track selected", "for Multi-Copy!");
    return -1;
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Paste Pattern
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_PATTERN_MultiPaste(u8 only_selected)
{
  s32 status = 0;
  char path[30];
	
  // paste multi copy presets into selected tracks
  u8 track, track_id;
  for(track=0, track_id=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    if( !only_selected || (ui_selected_tracks & (1 << track)) ) {
      ++track_id;
      sprintf(path, "/PRESETS/COPY%d.v4t", track_id);

      char str[30];
      sprintf(str, "Importing to G%dT%d:", (track/4)+1, (track%4)+1);
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, str, path);

      // mute track to avoid random effects while loading the file
      MIOS32_IRQ_Disable(); // this operation should be atomic!
      u8 muted = seq_core_trk_muted & (1 << track);
      if( !muted )
	seq_core_trk_muted |= (1 << track);
      MIOS32_IRQ_Enable();

      static seq_file_t_import_flags_t import_flags;
      import_flags.ALL = 0xff;

      // read file
      MUTEX_SDCARD_TAKE;
      status = SEQ_FILE_T_Read(path, track, import_flags, 1);
      MUTEX_SDCARD_GIVE;

      // unmute track if it wasn't muted before
      MIOS32_IRQ_Disable(); // this operation should be atomic!
      if( !muted )
	seq_core_trk_muted &= ~(1 << track);
      MIOS32_IRQ_Enable();

      if( status == FILE_ERR_OPEN_READ ) {
	char str[30];
	sprintf(str, "File for G%dT%d missing:", (track/4)+1, (track%4)+1);
	SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, str, path);
      } else if( status < 0 ) {
	SEQ_UI_SDCardErrMsg(2000, status);
      } else {
	sprintf(str, "Imported to G%dT%d:", (track/4)+1, (track%4)+1);
	SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, str, path);
      }
    }

#if 0
    // save pattern disabled
    if( status >= 0 && !only_selected && (track % 4) == 0 ) {
      u8 group = track/4;
      status = SEQ_FILE_B_PatternWrite(seq_file_session_name, seq_pattern[group].bank, seq_pattern[group].pattern, group, 0);
    }
#endif
  }

  if( !track_id ) {
    SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "No Track selected", "for Multi-Paste!");
    return -1;
  }

  // update selected patterns
  {
    int group;
    for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
      selected_pattern[group] = seq_pattern[group];
    }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Paste Pattern
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_PATTERN_MultiClear(u8 only_selected)
{
  // clear (selected) tracks
  u8 track, track_id;
  for(track=0, track_id=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    if( !only_selected || (ui_selected_tracks & (1 << track)) ) {
      ++track_id;

      // copy preset
      u8 only_layers = seq_core_options.PASTE_CLR_ALL ? 0 : 1;
      u8 all_triggers_cleared = 0;
      u8 init_assignments = 0;
      SEQ_LAYER_CopyPreset(track, only_layers, all_triggers_cleared, init_assignments);

      // clear all triggers
      memset((u8 *)&seq_trg_layer_value[track], 0, SEQ_TRG_MAX_BYTES);

      // cancel sustain if there are no steps played by the track anymore.
      SEQ_CORE_CancelSustainedNotes(track);      
    }
  }

  if( !track_id ) {
    SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "No Track selected", "for Multi-Clear!");
    return -1;
  }

  return 0; // no error
}
