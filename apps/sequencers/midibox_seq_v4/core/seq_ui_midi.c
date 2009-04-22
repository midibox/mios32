// $Id$
/*
 * MIDI configuration page
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
#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"

#include "seq_file_c.h"

#include "seq_midi_in.h"
#include "seq_midi_router.h"
#include "seq_midi_port.h"
#include "seq_bpm.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       11
#define ITEM_DEF_PORT      0
#define ITEM_IN_PORT       1
#define ITEM_IN_CHN        2
#define ITEM_TA_SPLIT      3
#define ITEM_TA_SPLIT_NOTE 4
#define ITEM_R_NODE        5
#define ITEM_R_SRC_PORT    6
#define ITEM_R_SRC_CHN     7
#define ITEM_R_DST_PORT    8
#define ITEM_R_DST_CHN     9
#define ITEM_MCLK_IN       10
#define ITEM_MCLK_OUT      11


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

u8 selected_router_node = 0;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_DEF_PORT: *gp_leds = 0x0003; break;
    case ITEM_IN_PORT: *gp_leds = 0x0004; break;
    case ITEM_IN_CHN: *gp_leds = 0x0008; break;
    case ITEM_TA_SPLIT: *gp_leds = 0x0030; break;
    case ITEM_TA_SPLIT_NOTE: *gp_leds = 0x00c0; break;
    case ITEM_R_NODE: *gp_leds = 0x0100; break;
    case ITEM_R_SRC_PORT: *gp_leds = 0x0200; break;
    case ITEM_R_SRC_CHN: *gp_leds = 0x0400; break;
    case ITEM_R_DST_PORT: *gp_leds = 0x0800; break;
    case ITEM_R_DST_CHN: *gp_leds = 0x1000; break;
    case ITEM_MCLK_IN: *gp_leds = 0x6000; break;
    case ITEM_MCLK_OUT: *gp_leds = 0x8000; break;
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
  seq_midi_router_node_t *n = &seq_midi_router_node[selected_router_node];

  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
    case SEQ_UI_ENCODER_GP2:
      ui_selected_item = ITEM_DEF_PORT;
      break;

    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_IN_PORT;
      break;

    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_IN_CHN;
      break;

    case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_TA_SPLIT;
      break;

    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_TA_SPLIT_NOTE;
      break;

    case SEQ_UI_ENCODER_GP9:
      ui_selected_item = ITEM_R_NODE;
      break;

    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_R_SRC_PORT;
      break;

    case SEQ_UI_ENCODER_GP11:
      ui_selected_item = ITEM_R_SRC_CHN;
      break;

    case SEQ_UI_ENCODER_GP12:
      ui_selected_item = ITEM_R_DST_PORT;
      break;

    case SEQ_UI_ENCODER_GP13:
      ui_selected_item = ITEM_R_DST_CHN;
      break;

    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
      ui_selected_item = ITEM_MCLK_IN;
      break;

    case SEQ_UI_ENCODER_GP16:
      ui_selected_item = ITEM_MCLK_OUT;
      break;
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_DEF_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_OutIxGet(MIOS32_MIDI_DefaultPortGet());
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	MIOS32_MIDI_DefaultPortSet(SEQ_MIDI_PORT_OutPortGet(port_ix));
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_IN_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_InIxGet(seq_midi_in_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_InNumGet()-1, incrementer) >= 0 ) {
	seq_midi_in_port = SEQ_MIDI_PORT_InPortGet(port_ix);
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_IN_CHN:
      return SEQ_UI_Var8_Inc(&seq_midi_in_channel, 0, 16, incrementer);

    case ITEM_TA_SPLIT:
      if( incrementer > 0 )
	seq_midi_in_ta_split_note |= 0x80;
      else if( incrementer < 0 )
	seq_midi_in_ta_split_note &= ~0x80;
      else
	seq_midi_in_ta_split_note ^= 0x80;
      return 1; // value changed

    case ITEM_TA_SPLIT_NOTE: {
      u8 note = seq_midi_in_ta_split_note & 0x7f;
      if( SEQ_UI_Var8_Inc(&note, 0, 127, incrementer) >= 0 ) {
	seq_midi_in_ta_split_note = (seq_midi_in_ta_split_note & 0x80) | note;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_R_NODE:
      return SEQ_UI_Var8_Inc(&selected_router_node, 0, SEQ_MIDI_ROUTER_NUM_NODES-1, incrementer);

    case ITEM_R_SRC_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_InIxGet(n->src_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_InNumGet()-1, incrementer) >= 0 ) {
	n->src_port = SEQ_MIDI_PORT_InPortGet(port_ix);
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_R_SRC_CHN:
      return SEQ_UI_Var8_Inc(&n->src_chn, 0, 17, incrementer);

    case ITEM_R_DST_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_OutIxGet(n->dst_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	n->dst_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_R_DST_CHN:
      return SEQ_UI_Var8_Inc(&n->dst_chn, 0, 17, incrementer);

    case ITEM_MCLK_IN: {
      u8 port_ix = SEQ_MIDI_PORT_InIxGet(seq_midi_in_mclk_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_InNumGet()-1, incrementer) >= 0 ) {
	seq_midi_in_mclk_port = SEQ_MIDI_PORT_InPortGet(port_ix);
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_MCLK_OUT:
      if( incrementer > 0 )
	seq_midi_router_mclk_out = 0xff;
      else if( incrementer < 0 )
	seq_midi_router_mclk_out = 0;
      else
	seq_midi_router_mclk_out ^= 0xff;
      return 1; // value changed
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

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 ) {
#endif
    // re-use encoder handler - only select UI item, don't increment, flags will be toggled
    return Encoder_Handler((int)button, 0);
  }

  // remaining buttons:
  switch( button ) {
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
  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  //  Def.Port Keyb.Chn. T/A Split Midd.Note Node IN P/Chn  OUT P/Chn     MIDI Clock 
  //    USB0   Def. #16     off       C-3     #1  Def. All  Def. # 1     I:All  O:off


  seq_midi_router_node_t *n = &seq_midi_router_node[selected_router_node];

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString(" Def.Port Keyb.Chn. T/A Split Midd.Note Node IN P/Chn  OUT P/Chn     MIDI Clock ");


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  SEQ_LCD_PrintSpaces(3);
  if( ui_selected_item == ITEM_DEF_PORT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(MIOS32_MIDI_DefaultPortGet())));
  }
  SEQ_LCD_PrintSpaces(3);


  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_IN_PORT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    if( seq_midi_in_port )
      SEQ_LCD_PrintString(SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(seq_midi_in_port)));
    else
      SEQ_LCD_PrintString(" All");
  }
  SEQ_LCD_PrintSpaces(1);


  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_IN_CHN && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    if( seq_midi_in_channel )
      SEQ_LCD_PrintFormattedString("#%2d", seq_midi_in_channel);
    else
      SEQ_LCD_PrintString("---");
  }
  SEQ_LCD_PrintSpaces(5);


  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_TA_SPLIT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString((seq_midi_in_ta_split_note & 0x80) ? "on " : "off");
  }
  SEQ_LCD_PrintSpaces(7);


  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_TA_SPLIT_NOTE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintNote(seq_midi_in_ta_split_note & 0x7f);
  }
  SEQ_LCD_PrintSpaces(4 + 1);


  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_R_NODE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(2);
  } else {
    SEQ_LCD_PrintFormattedString("#%d", selected_router_node+1);
  }
  SEQ_LCD_PrintSpaces(2);


  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_R_SRC_PORT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintString(SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(n->src_port)));
  }
  SEQ_LCD_PrintSpaces(1);


  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_R_SRC_CHN && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    if( !n->src_chn ) {
      SEQ_LCD_PrintString("---");
    } else if( n->src_chn > 16 ) {
      SEQ_LCD_PrintString("All");
    } else {
      SEQ_LCD_PrintFormattedString("#%2d", n->src_chn);
    }
  }
  SEQ_LCD_PrintSpaces(2);


  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_R_DST_PORT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(n->dst_port)));
  }
  SEQ_LCD_PrintSpaces(1);


  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_R_DST_CHN && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    if( !n->dst_chn ) {
      SEQ_LCD_PrintString("---");
    } else if( n->dst_chn > 16 ) {
      SEQ_LCD_PrintString("All");
    } else {
      SEQ_LCD_PrintFormattedString("#%2d", n->dst_chn);
    }
  }
  SEQ_LCD_PrintSpaces(5);


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintString("I:");
  if( ui_selected_item == ITEM_MCLK_IN && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    if( seq_midi_in_mclk_port )
      SEQ_LCD_PrintString(SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(seq_midi_in_mclk_port)));
    else
      SEQ_LCD_PrintString("All ");
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintString("O:");
  if( ui_selected_item == ITEM_MCLK_OUT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString(seq_midi_router_mclk_out ? "All" : "off");
  }


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local exit function
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  s32 status;

  // write config file
  MUTEX_SDCARD_TAKE;
  if( (status=SEQ_FILE_C_Write()) < 0 )
    SEQ_UI_SDCardErrMsg(2000, status);
  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MIDI_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

  return 0; // no error
}
