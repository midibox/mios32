// $Id: seq_ui_trkinst.c 2092 2014-11-16 00:21:24Z tk $
/*
 * Track instrument page
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

#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_cc.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_layer.h"
#include "seq_label.h"
#include "seq_midi_port.h"
#include "seq_midi_in.h"

#include "file.h"
#include "seq_file.h"
#include "seq_file_t.h"
#include <glcd_font.h>

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// Note/Chord/CC
#define NUM_OF_ITEMS        9
#define ITEM_GXTY           0
#define ITEM_MIDI_PORT      1
#define ITEM_MIDI_CHANNEL   2
#define ITEM_MIDI_PC        3
#define ITEM_MIDI_BANK_H    4
#define ITEM_MIDI_BANK_L    5
#define ITEM_TRKEVNT        6
#define ITEM_EDIT_NAME      7
#define ITEM_LAYER_SELECT   8


// Preset dialog screens
#define PR_DIALOG_NONE          0
#define PR_DIALOG_EDIT_LABEL    1


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 pr_dialog;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  u8 event_mode = SEQ_CC_Get(SEQ_UI_VisibleTrackGet(), SEQ_CC_MIDI_EVENT_MODE);

  switch( pr_dialog ) {
    case PR_DIALOG_EDIT_LABEL:
      // no LED functions yet
      break;

    default:
      switch( ui_selected_item ) {
      case ITEM_GXTY: *gp_leds = 0x0001; break;
      case ITEM_MIDI_PORT: *gp_leds = 0x0002; break;
      case ITEM_MIDI_CHANNEL: *gp_leds = 0x0004; break;
      case ITEM_MIDI_PC: *gp_leds = 0x0008; break;
      case ITEM_MIDI_BANK_H: *gp_leds = 0x0010; break;
      case ITEM_MIDI_BANK_L: *gp_leds = 0x0020; break;
      case ITEM_EDIT_NAME: *gp_leds = (event_mode == SEQ_EVENT_MODE_Drum) ? 0x0900 : 0xf900; break;
      case ITEM_LAYER_SELECT: *gp_leds = (event_mode == SEQ_EVENT_MODE_Drum) ? 0x0a00 : 0xfa00; break;
      }
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
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

  switch( pr_dialog ) {
    ///////////////////////////////////////////////////////////////////////////
    case PR_DIALOG_EDIT_LABEL:
      if( encoder <= SEQ_UI_ENCODER_GP16 ) {
	switch( encoder ) {
	  case SEQ_UI_ENCODER_GP15: { // select preset
	    int pos;

	    if( event_mode == SEQ_EVENT_MODE_Drum ) {
	      if( SEQ_UI_Var8_Inc(&ui_edit_preset_num_drum, 0, SEQ_LABEL_NumPresetsDrum()-1, incrementer) ) {
		SEQ_LABEL_CopyPresetDrum(ui_edit_preset_num_drum, (char *)&seq_core_trk[visible_track].name[5*ui_selected_instrument]);
		for(pos=4, ui_edit_name_cursor=pos; pos>=0; --pos)
		  if( seq_core_trk[visible_track].name[5*ui_selected_instrument + pos] == ' ' )
		    ui_edit_name_cursor = pos;
		  else
		    break;
		return 1;
	      }
	      return 0;
	    } else {
	      if( ui_edit_name_cursor < 5 ) { // select category preset
		if( SEQ_UI_Var8_Inc(&ui_edit_preset_num_category, 0, SEQ_LABEL_NumPresetsCategory()-1, incrementer) ) {
		  SEQ_LABEL_CopyPresetCategory(ui_edit_preset_num_category, (char *)&seq_core_trk[visible_track].name[0]);
		  for(pos=4, ui_edit_name_cursor=pos; pos>=0; --pos)
		    if( seq_core_trk[visible_track].name[pos] == ' ' )
		      ui_edit_name_cursor = pos;
		    else
		      break;
		  return 1;
		}
		return 0;
	      } else { // select label preset
		if( SEQ_UI_Var8_Inc(&ui_edit_preset_num_label, 0, SEQ_LABEL_NumPresets()-1, incrementer) ) {
		  SEQ_LABEL_CopyPreset(ui_edit_preset_num_label, (char *)&seq_core_trk[visible_track].name[5]);
		  for(pos=19, ui_edit_name_cursor=pos; pos>=5; --pos)
		    if( seq_core_trk[visible_track].name[pos] == ' ' )
		      ui_edit_name_cursor = pos;
		    else
		      break;
		  return 1;
		}
		return 0;
	      }
	    }
	  } break;

	  case SEQ_UI_ENCODER_GP16: // exit keypad editor
	    // EXIT only via button
	    if( incrementer == 0 ) {
	      ui_selected_item = 0;
	      pr_dialog = PR_DIALOG_NONE;
	    }
	    return 1;
	}

	if( event_mode == SEQ_EVENT_MODE_Drum )
	  return SEQ_UI_KeyPad_Handler(encoder, incrementer, (char *)&seq_core_trk[visible_track].name[ui_selected_instrument*5], 5);
	else
	  return SEQ_UI_KeyPad_Handler(encoder, incrementer, (char *)&seq_core_trk[visible_track].name, 20);
      }

      return -1; // encoder not mapped

    ///////////////////////////////////////////////////////////////////////////
    default:

      switch( encoder ) {
      case SEQ_UI_ENCODER_GP1:
			if( SEQ_UI_Var8_Inc(&ui_selected_item, 0, NUM_OF_ITEMS-1, incrementer) >= 0 )
				return 1;
			else
				return 0;	
	
	//ui_selected_item = ITEM_GXTY;
	break;

      case SEQ_UI_ENCODER_GP2:
	ui_selected_item = ITEM_MIDI_PORT;
	break;

      case SEQ_UI_ENCODER_GP3:
	ui_selected_item = ITEM_MIDI_CHANNEL;
	break;

      case SEQ_UI_ENCODER_GP4:
	ui_selected_item = ITEM_MIDI_PC;
	break;

      case SEQ_UI_ENCODER_GP5:
	ui_selected_item = ITEM_MIDI_BANK_H;
	break;

      case SEQ_UI_ENCODER_GP6:
	ui_selected_item = ITEM_MIDI_BANK_L;
	break;

      case SEQ_UI_ENCODER_GP8:
	// enter track event page if button has been pressed
	if( incrementer == 0 )
	  SEQ_UI_PageSet(SEQ_UI_PAGE_TRKEVNT);
	else
	  ui_selected_item = ITEM_LAYER_SELECT;
	return 1;

      case SEQ_UI_ENCODER_GP9:
	ui_selected_item = ITEM_EDIT_NAME;
	break;

      case SEQ_UI_ENCODER_GP10:
	ui_selected_item = (event_mode == SEQ_EVENT_MODE_Drum) ? ITEM_LAYER_SELECT : ITEM_EDIT_NAME;
	break;

      case SEQ_UI_ENCODER_GP11:
      case SEQ_UI_ENCODER_GP12:
      case SEQ_UI_ENCODER_GP13:
      case SEQ_UI_ENCODER_GP14:
      case SEQ_UI_ENCODER_GP15:
      case SEQ_UI_ENCODER_GP16:
	ui_selected_item = ITEM_EDIT_NAME;
	break;
      }

      // for GP encoders and Datawheel
      switch( ui_selected_item ) {
      case ITEM_GXTY: return SEQ_UI_GxTyInc(incrementer);

      case ITEM_MIDI_PORT: {
	mios32_midi_port_t port = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT);
	u8 port_ix = SEQ_MIDI_PORT_OutIxGet(port);
	if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) ) {
	  mios32_midi_port_t new_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	  SEQ_UI_CC_Set(SEQ_CC_MIDI_PORT, new_port);
	  return 1; // value changed
	}
	return 0; // value not changed
      } break;

      case ITEM_MIDI_CHANNEL: return SEQ_UI_CC_Inc(SEQ_CC_MIDI_CHANNEL, 0, 15, incrementer);

      case ITEM_MIDI_PC: {
	if( SEQ_UI_CC_Inc(SEQ_CC_MIDI_PC, 0, 128, incrementer) >= 0 ) {
	  MUTEX_MIDIOUT_TAKE;
	  SEQ_LAYER_SendPCBankValues(visible_track, 1, 1);
	  MUTEX_MIDIOUT_GIVE;
	  return 1; // value changed
	}
	return 0; // value not changed
      }

      case ITEM_MIDI_BANK_H: {
	if( SEQ_UI_CC_Inc(SEQ_CC_MIDI_BANK_H, 0, 128, incrementer) >= 0 ) {
	  MUTEX_MIDIOUT_TAKE;
	  SEQ_LAYER_SendPCBankValues(visible_track, 1, 1);
	  MUTEX_MIDIOUT_GIVE;
	  return 1; // value changed
	}
	return 0; // value not changed
      }

      case ITEM_MIDI_BANK_L: {
	if( SEQ_UI_CC_Inc(SEQ_CC_MIDI_BANK_L, 0, 128, incrementer) >= 0 ) {
	  MUTEX_MIDIOUT_TAKE;
	  SEQ_LAYER_SendPCBankValues(visible_track, 1, 1);
	  MUTEX_MIDIOUT_GIVE;
	  return 1; // value changed
	}
	return 0; // value not changed
      }

      case ITEM_EDIT_NAME:
	// switch to keypad editor if button has been pressed
	if( incrementer == 0 ) {
	  ui_selected_item = 0;
	  pr_dialog = PR_DIALOG_EDIT_LABEL;
	  SEQ_UI_KeyPad_Init();
	  ui_edit_name_cursor = (event_mode != SEQ_EVENT_MODE_Drum && encoder >= SEQ_UI_ENCODER_GP13) ? 5 : 0;
	}
	return 1;

      case ITEM_LAYER_SELECT: {
	if( event_mode == SEQ_EVENT_MODE_Drum ) {
	  return SEQ_UI_Var8_Inc(&ui_selected_instrument, 0, SEQ_TRG_NumInstrumentsGet(visible_track)-1, incrementer);
	} else {
	  return 1;
	}
      } break;
      }
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

  if( button <= SEQ_UI_BUTTON_GP16 )
    return Encoder_Handler(button, 0); // re-use encoder handler

  switch( pr_dialog ) {
    ///////////////////////////////////////////////////////////////////////////
    case PR_DIALOG_EDIT_LABEL:
      break;

    ///////////////////////////////////////////////////////////////////////////
    default:
      switch( button ) {
        case SEQ_UI_BUTTON_Select:
			if  ((6) == ui_selected_item) 
				SEQ_UI_PageSet(SEQ_UI_PAGE_TRKEVNT);
		return 1;		
		
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
  }

  if( depressed )
    return 0; // ignore when button depressed

  switch( button ) {
  case SEQ_UI_BUTTON_Exit:
    if( pr_dialog != PR_DIALOG_NONE ) {
      // switch to main dialog
      pr_dialog = PR_DIALOG_NONE;
      ui_selected_item = 0;
      return 1;
    }
    break;
  }

  return -1; // invalid or unsupported button
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. Port Chn.  PC  BnkH BnkL      Trk. Edit                                    
  // G1T1 IIC2  12  ---  ---  ---       Evnt Name           xxxxx-xxxxxxxxxxxxxxx    

  // "Edit Name" layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Please enter Track Category for G1T1    <xxxxx-xxxxxxxxxxxxxxx>                 
  // .,!1 ABC2 DEF3 GHI4 JKL5 MNO6 PQRS7 TUV8WXYZ9 -_ 0  Char <>  Del Ins Preset DONE

  // "Edit Drum Name" layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Please enter Drum Label for G1T1- 1:C-1 <xxxxx>                                 
  // .,!1 ABC2 DEF3 GHI4 JKL5 MNO6 PQRS7 TUV8WXYZ9 -_ 0  Char <>  Del Ins Preset DONE


  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);


  switch( pr_dialog ) {
    ///////////////////////////////////////////////////////////////////////////
    case PR_DIALOG_EDIT_LABEL: {
      int i;

      SEQ_LCD_CursorSet(0, 0);
      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	SEQ_LCD_PrintString("Enter Drum Label for ");
    
	///////////////////////////////////////////////////////////////////////////
	SEQ_LCD_CursorSet(0, 1);	
	SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
	SEQ_LCD_PrintFormattedString("-%2d:", ui_selected_instrument + 1);
	SEQ_LCD_PrintNote(SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_A1 + ui_selected_instrument));
	SEQ_LCD_PrintSpaces(1);

	SEQ_LCD_PrintChar('<');
	for(i=0; i<5; ++i)
	  SEQ_LCD_PrintChar(seq_core_trk[visible_track].name[5*ui_selected_instrument + i]);
	SEQ_LCD_PrintChar('>');
	SEQ_LCD_PrintSpaces(1);

	// insert flashing cursor
	if( ui_cursor_flash ) {
	  if( ui_edit_name_cursor >= 5 ) // correct cursor position if it is outside the 5 char label space
	    ui_edit_name_cursor = 0;
	  SEQ_LCD_CursorSet(13 + ui_edit_name_cursor, 1);
	  SEQ_LCD_PrintChar('*');
	}
      } else {
	if( ui_edit_name_cursor < 5 ) {
	  SEQ_LCD_PrintString("Enter Track Cat for  ");
	///////////////////////////////////////////////////////////////////////////
	SEQ_LCD_CursorSet(0, 1);	  
	  SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
	  SEQ_LCD_PrintSpaces(1);
	} else {
	  SEQ_LCD_PrintString("Enter Trck Label for ");
	///////////////////////////////////////////////////////////////////////////
	SEQ_LCD_CursorSet(0, 1);	  
	  SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
	  SEQ_LCD_PrintSpaces(1);
	}
	SEQ_LCD_PrintChar('<');
	for(i=0; i<5; ++i)
	  SEQ_LCD_PrintChar(seq_core_trk[visible_track].name[i]);
	SEQ_LCD_PrintChar('-');
	for(i=5; i<20; ++i)
	  SEQ_LCD_PrintChar(seq_core_trk[visible_track].name[i]);
	SEQ_LCD_PrintChar('>');
	SEQ_LCD_PrintSpaces(17);
      }


      // insert flashing cursor
      if( ui_cursor_flash ) {
	SEQ_LCD_CursorSet(13 + ((ui_edit_name_cursor < 5) ? 1 : 2) + ui_edit_name_cursor, 1);
	SEQ_LCD_PrintChar('*');
      }

	///////////////////////////////////////////////////////////////////////////
	SEQ_LCD_CursorSet(0, 2);
	///////////////////////////////////////////////////////////////////////////
	SEQ_LCD_PrintSpaces(21);	

	///////////////////////////////////////////////////////////////////////////
	SEQ_LCD_CursorSet(0, 3);
	///////////////////////////////////////////////////////////////////////////
	SEQ_LCD_PrintSpaces(21);	
	  
      SEQ_UI_KeyPad_LCD_Msg();
      SEQ_LCD_PrintString("Preset DONE");
    } break;


    ///////////////////////////////////////////////////////////////////////////
    default:
    ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 0);
    ///////////////////////////////////////////////////////////////////////////	  
	  	SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
		SEQ_LCD_PrintSpaces(1);
		
	  if  ((0) == ui_selected_item) {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
		SEQ_LCD_PrintString("TRACK ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	   } else {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
		SEQ_LCD_PrintString("TRACK ");
	   }
	SEQ_LCD_PrintString("INSTRUMENT");	
	
      //SEQ_LCD_PrintString("Trk. Port Chn.  PC  BnkH BnkL      Trk. ");
	  
      //SEQ_LCD_PrintSpaces(30);

	  
      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 1);
      ///////////////////////////////////////////////////////////////////////////		  
		SEQ_LCD_PrintSpaces(21);
	  
      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 2);
      ///////////////////////////////////////////////////////////////////////////	 
		//SEQ_LCD_PrintSpaces(21);	
	

      ///////////////////////////////////////////////////////////////////////////
	  
	  if  ((1) == ui_selected_item) {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
		SEQ_LCD_PrintString("Port ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	   } else {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
		SEQ_LCD_PrintString("Port ");
	   }
      mios32_midi_port_t port = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT);
      if( ui_selected_item == ITEM_MIDI_PORT && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	SEQ_LCD_PrintMIDIOutPort(port);
	SEQ_LCD_PrintChar(SEQ_MIDI_PORT_OutCheckAvailable(port) ? ' ' : '*');
      }

      ///////////////////////////////////////////////////////////////////////////
	  
	  if  ((2) == ui_selected_item) {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
		SEQ_LCD_PrintString("Chn. ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	   } else {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
		SEQ_LCD_PrintString("Chn. ");
	   }	  
	  
      if( ui_selected_item == ITEM_MIDI_CHANNEL && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	SEQ_LCD_PrintFormattedString("%3d  ", SEQ_CC_Get(visible_track, SEQ_CC_MIDI_CHANNEL)+1);
      }

	  
      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 3);
      ///////////////////////////////////////////////////////////////////////////		  
	  
      ///////////////////////////////////////////////////////////////////////////
	  if  ((3) == ui_selected_item) {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
		SEQ_LCD_PrintString("PC ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	   } else {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
		SEQ_LCD_PrintString("PC ");
	   }	  
	SEQ_LCD_PrintSpaces(2);	  
      if( ui_selected_item == ITEM_MIDI_PC && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	u8 value = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PC);
	if( value )
	  SEQ_LCD_PrintFormattedString("%3d  ", value-1);
	else
	  SEQ_LCD_PrintString("---  ");
      }

      ///////////////////////////////////////////////////////////////////////////
	  if  ((4) == ui_selected_item) {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
		SEQ_LCD_PrintString("BnkH ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	   } else {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
		SEQ_LCD_PrintString("BnkH ");
	   }	  
	  
      if( ui_selected_item == ITEM_MIDI_BANK_H && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	u8 value = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_BANK_H);
	if( value )
	  SEQ_LCD_PrintFormattedString("%3d  ", value-1);
	else
	  SEQ_LCD_PrintString("---  ");
      }

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 4);
      ///////////////////////////////////////////////////////////////////////////		  
	  
      ///////////////////////////////////////////////////////////////////////////
	  if  ((5) == ui_selected_item) {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
		SEQ_LCD_PrintString("BnkL ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	   } else {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
		SEQ_LCD_PrintString("BnkL ");
	   }	  
	  
      if( ui_selected_item == ITEM_MIDI_BANK_L && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	u8 value = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_BANK_L);
	if( value )
	  SEQ_LCD_PrintFormattedString("%3d  ", value-1);
	else
	  SEQ_LCD_PrintString("---  ");
      }

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_PrintSpaces(5);

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 5);
      ///////////////////////////////////////////////////////////////////////////	      
	  
	  ///////////////////////////////////////////////////////////////////////////
	  if  ((6) == ui_selected_item) {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
		SEQ_LCD_PrintString("Trk Event ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	   } else {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
		SEQ_LCD_PrintString("Trk Event ");
	   }
	SEQ_LCD_PrintString("         ");

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 6);
      ///////////////////////////////////////////////////////////////////////////	  
	  
      ///////////////////////////////////////////////////////////////////////////
	  if  ((7) == ui_selected_item) {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
		SEQ_LCD_PrintFormattedString("Edit ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	   } else {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
		SEQ_LCD_PrintFormattedString("Edit ");
	   }

	   if( event_mode != SEQ_EVENT_MODE_Drum  ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	  if  ((8) == ui_selected_item) {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
		SEQ_LCD_PrintFormattedString("Drum ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	   } else {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
		SEQ_LCD_PrintFormattedString("Drum ");
	   }	  
	  
	
      }

      if( ui_selected_item == ITEM_EDIT_NAME && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	SEQ_LCD_PrintString("Name ");
      }

	  
      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 7);
      ///////////////////////////////////////////////////////////////////////////		  
	  
      ///////////////////////////////////////////////////////////////////////////
      if( event_mode != SEQ_EVENT_MODE_Drum || (ui_selected_item == ITEM_LAYER_SELECT && ui_cursor_flash) ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	//SEQ_LCD_PrintSpaces(2);
	SEQ_LCD_PrintChar('A' + ui_selected_instrument);
	SEQ_LCD_PrintSpaces(2);
      }

      //SEQ_LCD_PrintSpaces(5);

      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	{
	  char *pattern_name = (char *)&seq_core_trk[visible_track].name[5*ui_selected_instrument];

	  int i;
	  u8 found_char = 0;
	  for(i=0; i<5; ++i) {
	    if( pattern_name[i] != ' ' ) {
	      found_char = 1;
	      break;
	    }
	  }

	  if( found_char )
	    SEQ_LCD_PrintStringPadded(pattern_name, 5);
	  else
	    SEQ_LCD_PrintString("_____");
	}

	SEQ_LCD_PrintSpaces(16);
      } else {
	{
	  char *pattern_name = (char *)&seq_core_trk[visible_track].name[0];

	  int i;
	  u8 found_char = 0;
	  for(i=0; i<5; ++i) {
	    if( pattern_name[i] != ' ' ) {
	      found_char = 1;
	      break;
	    }
	  }

	  if( found_char )
	    SEQ_LCD_PrintStringPadded(pattern_name, 5);
	  else
	    SEQ_LCD_PrintString("_____");
	}

	SEQ_LCD_PrintChar('-');

	{
	  char *pattern_name = (char *)&seq_core_trk[visible_track].name[5];

	  int i;
	  u8 found_char = 0;
	  for(i=0; i<15; ++i) {
	    if( pattern_name[i] != ' ' ) {
	      found_char = 1;
	      break;
	    }
	  }

	  if( found_char )
	    SEQ_LCD_PrintStringPadded(pattern_name, 15);
	  else
	    SEQ_LCD_PrintString("_______________");
	}
      }

      SEQ_LCD_PrintSpaces(4);
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKINST_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // load charset (if this hasn't been done yet)
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_Menu);

  pr_dialog = 0;

  // initialize edit label vars (name is modified directly, not via ui_edit_name!)
  ui_edit_name_cursor = 0;
  ui_edit_preset_num_category = 0;
  ui_edit_preset_num_label = 0;

  return 0; // no error
}
