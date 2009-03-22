// $Id$
/*
 * Song page
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
#include "seq_core.h"
#include "seq_song.h"
#include "seq_pattern.h"
#include "seq_file_b.h"
#include "seq_file_m.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// Note/Chord/CC
#define NUM_OF_ITEMS        8
#define ITEM_SONG           0
#define ITEM_POS            1
#define ITEM_ACTION         2
#define ITEM_G1             3
#define ITEM_G2             4
#define ITEM_G3             5
#define ITEM_G4             6
#define ITEM_SEL_BANK       7


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 edit_pos = 0; // won't be re-initialized by SEQ_UI_SONG_Init()
static u8 sel_bank;


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 CheckChangePattern(u8 group, u8 bank, u8 pattern);


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_SONG: *gp_leds = 0x0001; break;
    case ITEM_POS: *gp_leds = 0x0002; break;
    case ITEM_ACTION: *gp_leds = 0x0004; break;
    case ITEM_G1: *gp_leds = 0x0008; break;
    case ITEM_G2: *gp_leds = 0x0010; break;
    case ITEM_G3: *gp_leds = 0x0020; break;
    case ITEM_G4: *gp_leds = 0x0040; break;
    case ITEM_SEL_BANK: *gp_leds = 0x0080; break;
  }

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
  seq_song_step_t s = SEQ_SONG_StepEntryGet(edit_pos);

  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_SONG;
      break;

    case SEQ_UI_ENCODER_GP2:
      ui_selected_item = ITEM_POS;
      break;

    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_ACTION;
      break;

    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_G1;
      break;

    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_G2;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_G3;
      break;

    case SEQ_UI_ENCODER_GP7:
      ui_selected_item = ITEM_G4;
      break;

    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_SEL_BANK;
      break;

    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      return -1; // not mapped to encoder
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_SONG: {
      u8 song_num = SEQ_SONG_NumGet();
      u8 old_song_num = song_num;
      if( SEQ_UI_Var8_Inc(&song_num, 0, SEQ_SONG_NUM-1, incrementer) >= 0 ) {
	SEQ_SONG_Save(old_song_num);
	SEQ_SONG_Load(song_num);
	return 1; // value has been changed
      }
      return 0; // no change
    } break;

    case ITEM_POS:
      return SEQ_UI_Var8_Inc(&edit_pos, 0, SEQ_SONG_NUM_STEPS-1, incrementer);

    case ITEM_ACTION: {
      u8 action = (u8)s.action;
      if( SEQ_UI_Var8_Inc(&action, 0, SEQ_SONG_NUM_ACTIONS-1, incrementer) >= 0 ) {
	s.action = action;
	SEQ_SONG_StepEntrySet(edit_pos, s);
	return 1; // value has been changed
      }
      return 0; // no change
    } break;

    case ITEM_G1:
    case ITEM_G2:
    case ITEM_G3:
    case ITEM_G4: {
      switch( s.action ) {
        case SEQ_SONG_ACTION_Stop:
	  return 0; // do nothing

        case SEQ_SONG_ACTION_JmpPos: {
	  u8 val = (u8)s.action_value;
	  if( SEQ_UI_Var8_Inc(&val, 0, SEQ_SONG_NUM_STEPS-1, incrementer) >= 0 ) {
	    s.action_value = val;
	    SEQ_SONG_StepEntrySet(edit_pos, s);
	    return 1; // value has been changed
	  }
	  return 0; // no change
	}

        case SEQ_SONG_ACTION_JmpSong: {
	  u8 val = (u8)s.action_value;
	  if( SEQ_UI_Var8_Inc(&val, 0, SEQ_SONG_NUM-1, incrementer) >= 0 ) {
	    s.action_value = val;
	    SEQ_SONG_StepEntrySet(edit_pos, s);
	    return 1; // value has been changed
	  }
	  return 0; // no change
	}

        case SEQ_SONG_ACTION_SelMixerMap: {
	  u8 val = (u8)s.action_value;
	  u8 num_mixer_maps = SEQ_FILE_M_NumMaps();
	  if( !num_mixer_maps )
	    num_mixer_maps = 128;
	  if( SEQ_UI_Var8_Inc(&val, 0, num_mixer_maps-1, incrementer) >= 0 ) {
	    s.action_value = val;
	    SEQ_SONG_StepEntrySet(edit_pos, s);
	    return 1; // value has been changed
	  }
	  return 0; // no change
	}

        default:
	  if( s.action < SEQ_SONG_ACTION_Loop1 || s.action > SEQ_SONG_ACTION_Loop16 )
	    return 0; // no change
	  else {
	    u8 val_bank;
	    switch( ui_selected_item - ITEM_G1 ) {
	      case 0: val_bank = s.bank_g1; break;
	      case 1: val_bank = s.bank_g2; break;
	      case 2: val_bank = s.bank_g3; break;
	      case 3: val_bank = s.bank_g4; break;
	      default: return 0; // invalid bank selection
	    }

	    u8 val_pattern;
	    switch( ui_selected_item - ITEM_G1 ) {
	      case 0: val_pattern = s.pattern_g1; break;
	      case 1: val_pattern = s.pattern_g2; break;
	      case 2: val_pattern = s.pattern_g3; break;
	      case 3: val_pattern = s.pattern_g4; break;
	      default: return 0; // invalid bank selection
	    }

	    if( sel_bank ) {
	      if( SEQ_UI_Var8_Inc(&val_bank, 0, SEQ_FILE_B_NUM_BANKS-1, incrementer) >= 0 ) {
		switch( ui_selected_item - ITEM_G1 ) {
		  case 0: s.bank_g1 = val_bank; break;
		  case 1: s.bank_g2 = val_bank; break;
		  case 2: s.bank_g3 = val_bank; break;
		  case 3: s.bank_g4 = val_bank; break;
   	          default: return 0; // invalid bank selection
		}
		SEQ_SONG_StepEntrySet(edit_pos, s);
		CheckChangePattern(ui_selected_item - ITEM_G1, val_bank, val_pattern);
		return 1; // value has been changed
	      }
	    } else {
	      u8 num_patterns = SEQ_FILE_B_NumPatterns(val_bank);
	      if( !num_patterns )
		num_patterns = 64;
	      // move 0x80 (disable pattern) to the beginning of pattern selection for more comfortable editing
	      u8 val_pattern_converted = (val_pattern >= 0x80) ? 0 : (val_pattern+1);
	      if( SEQ_UI_Var8_Inc(&val_pattern_converted, 0, num_patterns, incrementer) >= 0 ) {
		val_pattern = (val_pattern_converted == 0) ? 0x80 : (val_pattern_converted-1);
		switch( ui_selected_item - ITEM_G1 ) {
		  case 0: s.pattern_g1 = val_pattern; break;
		  case 1: s.pattern_g2 = val_pattern; break;
		  case 2: s.pattern_g3 = val_pattern; break;
		  case 3: s.pattern_g4 = val_pattern; break;
   	          default: return 0; // invalid bank selection
		}
		SEQ_SONG_StepEntrySet(edit_pos, s);
		CheckChangePattern(ui_selected_item - ITEM_G1, val_bank, val_pattern);
		return 1; // value has been changed
	      }
	    }
	    return 0; // no change
	  }
      }
    } break;

    case ITEM_SEL_BANK:
      if( s.action < SEQ_SONG_ACTION_Loop1 || s.action > SEQ_SONG_ACTION_Loop16 )
	return 0; // no change

      if( incrementer > 0 )
	sel_bank = 1;
      else if( incrementer < 0 )
	sel_bank = 0;
      else
	return 0; // no change
      return 1; // value has been changed
  }

  return -1; // invalid or unsupported encoder
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

  switch( button ) {
    case SEQ_UI_BUTTON_GP1:
      ui_selected_item = ITEM_SONG;
      return 1;

    case SEQ_UI_BUTTON_GP2:
      ui_selected_item = ITEM_POS;
      return 1;

    case SEQ_UI_BUTTON_GP3:
      ui_selected_item = ITEM_ACTION;
      return 1;

    case SEQ_UI_BUTTON_GP4:
      ui_selected_item = ITEM_G1;
      return 1;

    case SEQ_UI_BUTTON_GP5:
      ui_selected_item = ITEM_G2;
      return 1;

    case SEQ_UI_BUTTON_GP6:
      ui_selected_item = ITEM_G3;
      return 1;

    case SEQ_UI_BUTTON_GP7:
      ui_selected_item = ITEM_G4;
      return 1;

    case SEQ_UI_BUTTON_GP8:
      ui_selected_item = ITEM_SEL_BANK;
      return 1;

    case SEQ_UI_BUTTON_GP9:
    case SEQ_UI_BUTTON_GP10:
    case SEQ_UI_BUTTON_GP11:
    case SEQ_UI_BUTTON_GP12:
    case SEQ_UI_BUTTON_GP13:
    case SEQ_UI_BUTTON_GP14:
    case SEQ_UI_BUTTON_GP15:
    case SEQ_UI_BUTTON_GP16:
      // not mapped
      return -1;

    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
      if( ++ui_selected_item >= NUM_OF_ITEMS )
	  ui_selected_item = 0;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
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
  if( high_prio ) {
    ///////////////////////////////////////////////////////////////////////////
    // frequently update VU meters
    u8 track;
    seq_core_trk_t *t = &seq_core_trk[0];
    for(track=0; track<16; ++t, ++track) {
      if( !(track % 4) )
	SEQ_LCD_CursorSet(46 + 10*(track>>2), 1);

      SEQ_LCD_PrintVBar(t->vu_meter >> 4);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Song position
    SEQ_LCD_CursorSet(40 + 23, 0);
    u32 song_pos = SEQ_SONG_PosGet();
    SEQ_LCD_PrintFormattedString("%c%d.", 'A' + (song_pos>>3), (song_pos&7)+1);

    if( SEQ_BPM_IsRunning() || ui_seq_pause )
      SEQ_LCD_PrintFormattedString("%3d", seq_core_state.ref_step + 1);
    else
      SEQ_LCD_PrintString("---");

    ///////////////////////////////////////////////////////////////////////////
    // Loop counter
    if( SEQ_SONG_ActiveGet() ) {
      SEQ_LCD_CursorSet(40 + 35, 0);

      if( SEQ_BPM_IsRunning() || ui_seq_pause )
	SEQ_LCD_PrintFormattedString("%2d", SEQ_SONG_LoopCtrGet()+1);
      else
	SEQ_LCD_PrintString("--");

      SEQ_LCD_PrintFormattedString("/%2d", SEQ_SONG_LoopCtrMaxGet()+1);
    }

    return 0;
  }


  seq_song_step_t s = SEQ_SONG_StepEntryGet(edit_pos);
  

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Song  Pos  Actn  G1   G2   G3   G4  Sel. Song Mode   S#xx  Pos xx.xxx Loop xx/xx
  //   1    A1  Stop 1:A1 1:C1 1:E1 1:G1 Pat  1:A1 ____ 1:C1 ____ 1:E1 ____ 1:G1 ____

  // Song Phrase      G1   G2   G3   G4  Sel.Phrase Mode  S#xx  Pos xx.xxx           
  //

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Song  Pos  Actn ");
  switch( s.action ) {
    case SEQ_SONG_ACTION_Stop:
      SEQ_LCD_PrintSpaces(24);
      break;

    case SEQ_SONG_ACTION_JmpPos:
      SEQ_LCD_PrintString(" ->  Pos.");
      SEQ_LCD_PrintSpaces(15);
      break;

    case SEQ_SONG_ACTION_JmpSong:
      SEQ_LCD_PrintString(" ->  Song");
      SEQ_LCD_PrintSpaces(15);
      break;

    case SEQ_SONG_ACTION_SelMixerMap:
      SEQ_LCD_PrintString(" ->  Map");
      SEQ_LCD_PrintSpaces(16);
      break;

    default:
      if( s.action < SEQ_SONG_ACTION_Loop1 || s.action > SEQ_SONG_ACTION_Loop16 )
	SEQ_LCD_PrintSpaces(24);
      else
	SEQ_LCD_PrintString(" G1   G2   G3   G4  Sel.");
  }
  
  SEQ_LCD_PrintString(SEQ_SONG_ActiveGet() ? " Song Mode " : "Phrase Mode");
  SEQ_LCD_PrintFormattedString("  S#%2d  Pos ", SEQ_SONG_NumGet()+1);
  // (song position print in high_prio branch)

  SEQ_LCD_CursorSet(40 + 29, 0);
  if( SEQ_SONG_ActiveGet() ) {
    SEQ_LCD_PrintString(" Loop ");
    // (loop counter print in high_prio branch)
  } else
    SEQ_LCD_PrintSpaces(12);


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_SONG && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("%3d", SEQ_SONG_NumGet() + 1);
  }
  SEQ_LCD_PrintSpaces(4);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_POS && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(2);
  } else {
    SEQ_LCD_PrintFormattedString("%c%d", 'A' + (edit_pos >> 3), (edit_pos&7)+1);
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_ACTION && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    switch( s.action ) {
      case SEQ_SONG_ACTION_Stop:
	SEQ_LCD_PrintString(" Stop");
	break;

      case SEQ_SONG_ACTION_JmpPos:
      case SEQ_SONG_ACTION_JmpSong:
	SEQ_LCD_PrintString(" Jump");
	break;

      case SEQ_SONG_ACTION_SelMixerMap:
	SEQ_LCD_PrintString("Mixer");
	break;

      default:
	if( s.action < SEQ_SONG_ACTION_Loop1 || s.action > SEQ_SONG_ACTION_Loop16 )
	  SEQ_LCD_PrintString(" ????");
	else
	  SEQ_LCD_PrintFormattedString(" x%2d ", (int)(s.action-SEQ_SONG_ACTION_Loop1+1));
    }
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  switch( s.action ) {
    case SEQ_SONG_ACTION_Stop:
      SEQ_LCD_PrintSpaces(20);
      break;

    case SEQ_SONG_ACTION_JmpPos:
      SEQ_LCD_PrintSpaces(6);
      if( ui_selected_item >= ITEM_G1 && ui_selected_item <= ITEM_G4 && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(2);
      } else {
	SEQ_LCD_PrintFormattedString("%c%d", 'A' + (s.action_value >> 3), (s.action_value&7)+1);
      }
      SEQ_LCD_PrintSpaces(12);
      break;

    case SEQ_SONG_ACTION_JmpSong:
    case SEQ_SONG_ACTION_SelMixerMap:
      SEQ_LCD_PrintSpaces(5);
      if( ui_selected_item >= ITEM_G1 && ui_selected_item <= ITEM_G4 && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	SEQ_LCD_PrintFormattedString("%3d", s.action_value + 1);
      }
      SEQ_LCD_PrintSpaces(12);
      break;

    default:
      if( s.action < SEQ_SONG_ACTION_Loop1 || s.action > SEQ_SONG_ACTION_Loop16 )
	SEQ_LCD_PrintSpaces(20);
      else {
	if( ui_selected_item == ITEM_G1 && ui_cursor_flash )
	  SEQ_LCD_PrintSpaces(5);
	else if( s.pattern_g1 >= 0x80 )
	  SEQ_LCD_PrintString("-:-- ");
	else
	  SEQ_LCD_PrintFormattedString("%x:%c%d ", s.bank_g1+1, 'A' + (s.pattern_g1>>3), (s.pattern_g1&7)+1);

	if( ui_selected_item == ITEM_G2 && ui_cursor_flash )
	  SEQ_LCD_PrintSpaces(5);
	else if( s.pattern_g2 >= 0x80 )
	  SEQ_LCD_PrintString("-:-- ");
	else
	  SEQ_LCD_PrintFormattedString("%x:%c%d ", s.bank_g2+1, 'A' + (s.pattern_g2>>3), (s.pattern_g2&7)+1);

	if( ui_selected_item == ITEM_G3 && ui_cursor_flash )
	  SEQ_LCD_PrintSpaces(5);
	else if( s.pattern_g3 >= 0x80 )
	  SEQ_LCD_PrintString("-:-- ");
	else
	  SEQ_LCD_PrintFormattedString("%x:%c%d ", s.bank_g3+1, 'A' + (s.pattern_g3>>3), (s.pattern_g3&7)+1);

	if( ui_selected_item == ITEM_G4 && ui_cursor_flash )
	  SEQ_LCD_PrintSpaces(5);
	else if( s.pattern_g4 >= 0x80 )
	  SEQ_LCD_PrintString("-:-- ");
	else
	  SEQ_LCD_PrintFormattedString("%x:%c%d ", s.bank_g4+1, 'A' + (s.pattern_g4>>3), (s.pattern_g4&7)+1);
      }
  }

  ///////////////////////////////////////////////////////////////////////////
  if( (s.action < SEQ_SONG_ACTION_Loop1 || s.action > SEQ_SONG_ACTION_Loop16) ||
      (ui_selected_item == ITEM_SEL_BANK && ui_cursor_flash) ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintFormattedString(sel_bank ? "Bnk " : "Pat ", 0);
  }

  ///////////////////////////////////////////////////////////////////////////
  int group;
  for(group=0; group<4; ++group) {
    SEQ_LCD_CursorSet(40 + 10*group, 1);

    seq_pattern_t pattern = seq_pattern[group];
    SEQ_LCD_PrintFormattedString(" %d:", pattern.bank + 1);

    if( pattern.pattern < SEQ_FILE_B_NumPatterns(pattern.bank) )
      SEQ_LCD_PrintPattern(pattern);
    else
      SEQ_LCD_PrintString("!!"); // covers the case that bank is not available, or that pattern number too high

    SEQ_LCD_PrintChar(' ');

    // (VU meters print in high_prio branch)

  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_SONG_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // we want to show vertical VU meters
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBars);

  // change pattern numbers by default
  sel_bank = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Change pattern immediately if in phrase mode or song position matches
// with edit position
/////////////////////////////////////////////////////////////////////////////
static s32 CheckChangePattern(u8 group, u8 bank, u8 pattern)
{
  if( pattern < 0x80 && (!SEQ_SONG_ActiveGet() || edit_pos == SEQ_SONG_PosGet()) ) {
    seq_pattern_t p;
    p.ALL = 0;
    p.pattern = pattern;
    p.bank = bank;
    return SEQ_PATTERN_Change(group, p);
  }

  return 0; // no error
}

