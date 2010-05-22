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
#include "seq_blm.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define SUBPAGE_TRANSPOSE  0
#define SUBPAGE_SECTIONS   1
#define SUBPAGE_ROUTER     2
#define SUBPAGE_MISC       3



#define NUM_OF_ITEMS       22

// Transpose
#define ITEM_IN_BUS        0
#define ITEM_IN_PORT       1
#define ITEM_IN_CHN        2
#define ITEM_IN_LOWER      3
#define ITEM_IN_UPPER      4
#define ITEM_IN_MODE       5
#define ITEM_RESET_STACKS  6

// Sections
#define ITEM_S_PORT        7
#define ITEM_S_CHN         8
#define ITEM_S_OCT_G1      9
#define ITEM_S_OCT_G2      10
#define ITEM_S_OCT_G3      11
#define ITEM_S_OCT_G4      12
#define ITEM_S_FWD_PORT    13
#define ITEM_S_RESET_STACKS 14

// MIDI Router
#define ITEM_R_NODE        15
#define ITEM_R_SRC_PORT    16
#define ITEM_R_SRC_CHN     17
#define ITEM_R_DST_PORT    18
#define ITEM_R_DST_CHN     19
#define ITEM_DEF_PORT      20

// Misc
#define ITEM_BLM_SCALAR_PORT 21


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 store_file_required;
static u8 selected_subpage = SUBPAGE_TRANSPOSE;
static u8 selected_router_node = 0;
static u8 selected_bus = 0;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  *gp_leds = (3 << (2*selected_subpage));

  switch( selected_subpage ) {
    case SUBPAGE_TRANSPOSE:
      switch( ui_selected_item ) {
        case ITEM_IN_BUS: *gp_leds |= 0x0100; break;
        case ITEM_IN_PORT: *gp_leds |= 0x0200; break;
        case ITEM_IN_CHN: *gp_leds |= 0x0400; break;
        case ITEM_IN_LOWER: *gp_leds |= 0x0800; break;
        case ITEM_IN_UPPER: *gp_leds |= 0x1000; break;
        case ITEM_IN_MODE: *gp_leds |= 0x2000; break;
        case ITEM_RESET_STACKS: *gp_leds |= 0x8000; break;
      }
      break;

    case SUBPAGE_SECTIONS:
      switch( ui_selected_item ) {
        case ITEM_S_PORT: *gp_leds |= 0x0100; break;
        case ITEM_S_CHN: *gp_leds |= 0x0200; break;
        case ITEM_S_OCT_G1: *gp_leds |= 0x0400; break;
        case ITEM_S_OCT_G2: *gp_leds |= 0x0800; break;
        case ITEM_S_OCT_G3: *gp_leds |= 0x1000; break;
        case ITEM_S_OCT_G4: *gp_leds |= 0x2000; break;
        case ITEM_S_FWD_PORT: *gp_leds |= 0x4000; break;
        case ITEM_S_RESET_STACKS: *gp_leds |= 0x8000; break;
      }
      break;

    case SUBPAGE_ROUTER:
      switch( ui_selected_item ) {
        case ITEM_R_NODE: *gp_leds |= 0x0100; break;
        case ITEM_R_SRC_PORT: *gp_leds |= 0x0200; break;
        case ITEM_R_SRC_CHN: *gp_leds |= 0x0400; break;
        case ITEM_R_DST_PORT: *gp_leds |= 0x0800; break;
        case ITEM_R_DST_CHN: *gp_leds |= 0x1000; break;
        case ITEM_DEF_PORT: *gp_leds |= 0xc000; break;
      }
      break;

    case SUBPAGE_MISC:
      switch( ui_selected_item ) {
        case ITEM_BLM_SCALAR_PORT: *gp_leds |= 0x0300; break;
      }
      break;
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Help function
/////////////////////////////////////////////////////////////////////////////
static s32 SetSubpageBasedOnItem(u8 item)
{
  if( item <= ITEM_RESET_STACKS )
    selected_subpage = SUBPAGE_TRANSPOSE;
  else if( item <= ITEM_S_RESET_STACKS )
    selected_subpage = SUBPAGE_SECTIONS;
  else if( item <= ITEM_DEF_PORT )
    selected_subpage = SUBPAGE_ROUTER;
  else
    selected_subpage = SUBPAGE_MISC;

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

  if( encoder <= SEQ_UI_ENCODER_GP8 ) {
    selected_subpage = encoder/2;
    switch( selected_subpage ) {
      case SUBPAGE_TRANSPOSE:
	ui_selected_item = ITEM_IN_BUS;
	break;

      case SUBPAGE_SECTIONS:
	ui_selected_item = ITEM_S_PORT;
	break;

      case SUBPAGE_ROUTER:
	ui_selected_item = ITEM_R_NODE;
	break;

      case SUBPAGE_MISC:
	ui_selected_item = ITEM_BLM_SCALAR_PORT;
	break;
    }
    return 1;
  }

  switch( selected_subpage ) {
    case SUBPAGE_TRANSPOSE:
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP9:
	  ui_selected_item = ITEM_IN_BUS;
	  break;

        case SEQ_UI_ENCODER_GP10:
	  ui_selected_item = ITEM_IN_PORT;
	  break;

        case SEQ_UI_ENCODER_GP11:
	  ui_selected_item = ITEM_IN_CHN;
	  break;

        case SEQ_UI_ENCODER_GP12:
	  ui_selected_item = ITEM_IN_LOWER;
	  break;

        case SEQ_UI_ENCODER_GP13:
	  ui_selected_item = ITEM_IN_UPPER;
	  break;

        case SEQ_UI_ENCODER_GP14:
	  ui_selected_item = ITEM_IN_MODE;
	  break;

        case SEQ_UI_ENCODER_GP15:
	  return -1; // not mapped

        case SEQ_UI_ENCODER_GP16:
	  ui_selected_item = ITEM_RESET_STACKS;
	  break;
      }
      break;

    case SUBPAGE_SECTIONS:
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP9:
	  ui_selected_item = ITEM_S_PORT;
	  break;

        case SEQ_UI_ENCODER_GP10:
	  ui_selected_item = ITEM_S_CHN;
	  break;

        case SEQ_UI_ENCODER_GP11:
	  ui_selected_item = ITEM_S_OCT_G1;
	  break;

        case SEQ_UI_ENCODER_GP12:
	  ui_selected_item = ITEM_S_OCT_G2;
	  break;

        case SEQ_UI_ENCODER_GP13:
	  ui_selected_item = ITEM_S_OCT_G3;
	  break;

        case SEQ_UI_ENCODER_GP14:
	  ui_selected_item = ITEM_S_OCT_G4;
	  break;

        case SEQ_UI_ENCODER_GP15:
	  ui_selected_item = ITEM_S_FWD_PORT;
	  break;

        case SEQ_UI_ENCODER_GP16:
	  ui_selected_item = ITEM_S_RESET_STACKS;
	  break;
      }
      break;

    case SUBPAGE_ROUTER:
      switch( encoder ) {
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
	  return -1; // not used (yet)

        case SEQ_UI_ENCODER_GP15:
        case SEQ_UI_ENCODER_GP16:
	  ui_selected_item = ITEM_DEF_PORT;
	  break;
      }
      break;

    case SUBPAGE_MISC:
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP9:
        case SEQ_UI_ENCODER_GP10:
	  ui_selected_item = ITEM_BLM_SCALAR_PORT;
	  break;

        case SEQ_UI_ENCODER_GP11:
        case SEQ_UI_ENCODER_GP12:
        case SEQ_UI_ENCODER_GP13:
        case SEQ_UI_ENCODER_GP14:
	  return -1; // not used (yet)

        case SEQ_UI_ENCODER_GP15:
        case SEQ_UI_ENCODER_GP16:
	  // enter midi monitor page
	  SEQ_UI_PageSet(SEQ_UI_PAGE_MIDIMON);
	  return 1;
      }
      break;

    default:
      return -1; // unsupported subpage
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_IN_BUS: {
      if( SEQ_UI_Var8_Inc(&selected_bus, 0, SEQ_MIDI_IN_NUM_BUSSES-1, incrementer) >= 0 ) {
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_IN_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_InIxGet(seq_midi_in_port[selected_bus]);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_InNumGet()-1-4, incrementer) >= 0 ) { // don't allow selection of Bus1..Bus4
	seq_midi_in_port[selected_bus] = SEQ_MIDI_PORT_InPortGet(port_ix);
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_IN_CHN:
      if( SEQ_UI_Var8_Inc(&seq_midi_in_channel[selected_bus], 0, 16, incrementer) >= 0 ) {
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_IN_LOWER:
      if( SEQ_UI_Var8_Inc(&seq_midi_in_lower[selected_bus], 0, 127, incrementer) >= 0 ) {
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_IN_UPPER:
      if( SEQ_UI_Var8_Inc(&seq_midi_in_upper[selected_bus], 0, 127, incrementer) >= 0 ) {
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_IN_MODE: {
      u8 fwd = seq_midi_in_options[selected_bus].MODE_PLAY;
      if( SEQ_UI_Var8_Inc(&fwd, 0, 1, incrementer) >= 0 ) {
	seq_midi_in_options[selected_bus].MODE_PLAY = fwd;
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_RESET_STACKS: {
      SEQ_MIDI_IN_ResetTransArpStacks();
      SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Transposer/Arp.", "Stacks cleared!");
      return 1;
    } break;


    case ITEM_S_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_InIxGet(seq_midi_in_sect_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_InNumGet()-1, incrementer) >= 0 ) {
	seq_midi_in_sect_port = SEQ_MIDI_PORT_InPortGet(port_ix);
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_S_CHN:
      if( SEQ_UI_Var8_Inc(&seq_midi_in_sect_channel, 0, 16, incrementer) >= 0 ) {
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change


    case ITEM_S_OCT_G1: {
      u8 oct = seq_midi_in_sect_note[0] / 12;
      if( SEQ_UI_Var8_Inc(&oct, 0, 11, incrementer) >= 0 ) {
	seq_midi_in_sect_note[0] = 12*oct;
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_S_OCT_G2: {
      u8 oct = seq_midi_in_sect_note[1] / 12;
      if( SEQ_UI_Var8_Inc(&oct, 0, 11, incrementer) >= 0 ) {
	seq_midi_in_sect_note[1] = 12*oct;
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_S_OCT_G3: {
      u8 oct = seq_midi_in_sect_note[2] / 12;
      if( SEQ_UI_Var8_Inc(&oct, 0, 11, incrementer) >= 0 ) {
	seq_midi_in_sect_note[2] = 12*oct;
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_S_OCT_G4: {
      u8 oct = seq_midi_in_sect_note[3] / 12;
      if( SEQ_UI_Var8_Inc(&oct, 0, 11, incrementer) >= 0 ) {
	seq_midi_in_sect_note[3] = 12*oct;
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_S_FWD_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_OutIxGet(seq_midi_in_sect_fwd_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	seq_midi_in_sect_fwd_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_S_RESET_STACKS: {
      SEQ_MIDI_IN_ResetChangerStacks();
      SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Section Changer", "Stacks cleared!");
      return 1;
    } break;



    case ITEM_R_NODE:
      if( SEQ_UI_Var8_Inc(&selected_router_node, 0, SEQ_MIDI_ROUTER_NUM_NODES-1, incrementer) >= 0 ) {
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_R_SRC_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_InIxGet(n->src_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_InNumGet()-1, incrementer) >= 0 ) {
	n->src_port = SEQ_MIDI_PORT_InPortGet(port_ix);
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_R_SRC_CHN:
      if( SEQ_UI_Var8_Inc(&n->src_chn, 0, 17, incrementer) >= 0 ) {
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_R_DST_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_OutIxGet(n->dst_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	n->dst_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_R_DST_CHN:
      if( SEQ_UI_Var8_Inc(&n->dst_chn, 0, 17, incrementer) >= 0 ) {
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change


    case ITEM_DEF_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_OutIxGet(MIOS32_MIDI_DefaultPortGet());
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	MIOS32_MIDI_DefaultPortSet(SEQ_MIDI_PORT_OutPortGet(port_ix));
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;


    case ITEM_BLM_SCALAR_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_InIxGet(seq_blm_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_InNumGet()-1, incrementer) >= 0 ) {
	seq_blm_port = SEQ_MIDI_PORT_InPortGet(port_ix);
	MUTEX_MIDIOUT_TAKE;
	blm_timeout_ctr = 0; // fake timeout (so that "BLM not found" message will be displayed)
	SEQ_BLM_SYSEX_SendRequest(0x00); // request layout from BLM_SCALAR
	MUTEX_MIDIOUT_GIVE;
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;
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
      SetSubpageBasedOnItem(ui_selected_item);
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
      SetSubpageBasedOnItem(ui_selected_item);
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
  // Transposer  Section    MIDI              Bus Port Chn. Lower/Upper Mode   Reset 
  //  and Arp.   Control   Router    Misc.     1  IN1  #16   ---   ---  T&A    Stacks

  // Transposer  Section    MIDI             Port Chn.  G1   G2   G3   G4  Fwd  Reset
  //  and Arp.   Control   Router    Misc.   Def. #16  C-1  C-2  C-3  C-4  USB1 Stcks

  // Transposer  Section    MIDI             Node IN P/Chn  OUT P/Chn     DefaultPort
  //  and Arp.   Control   Router    Misc.    #1  Def. All  Def. # 1         USB1    

  // Transposer  Section    MIDI             BLM_SCALAR connected            MIDI   
  //  and Arp.   Control   Router    Misc.   Port: OUT2                     Monitor 


  seq_midi_router_node_t *n = &seq_midi_router_node[selected_router_node];

  ///////////////////////////////////////////////////////////////////////////
  const char leftpage[2][41] = {
    "Transposer  Section    MIDI             ",
    " and Arp.   Control   Router    Misc.   "
  };

  int line;
  for(line=0; line<2; ++line) {
    int i;

    SEQ_LCD_CursorSet(0, line);

    for(i=0; i<selected_subpage*10; ++i)
      SEQ_LCD_PrintChar(leftpage[line][i]);

    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(10);
      i+=10;
    } else {
      for(;i<(10 + selected_subpage*10); ++i)
	SEQ_LCD_PrintChar(leftpage[line][i]);
    }

    for(; i<40; ++i)
      SEQ_LCD_PrintChar(leftpage[line][i]);
  }

  switch( selected_subpage ) {
  ///////////////////////////////////////////////////////////////////////////
    case SUBPAGE_TRANSPOSE: {
      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintString(" Bus Port Chn. Lower/Upper Mode   Reset ");
      SEQ_LCD_CursorSet(40, 1);

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_IN_BUS && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	SEQ_LCD_PrintFormattedString("  %d  ", selected_bus+1);
      }

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_IN_PORT && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	if( seq_midi_in_port[selected_bus] )
	  SEQ_LCD_PrintString(SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(seq_midi_in_port[selected_bus])));
	else
	  SEQ_LCD_PrintString(" All");
      }
      SEQ_LCD_PrintSpaces(1);


      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_IN_CHN && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	if( seq_midi_in_channel[selected_bus] )
	  SEQ_LCD_PrintFormattedString("#%2d", seq_midi_in_channel[selected_bus]);
	else
	  SEQ_LCD_PrintString("---");
      }
      SEQ_LCD_PrintSpaces(3);


      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_IN_LOWER && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	SEQ_LCD_PrintNote(seq_midi_in_lower[selected_bus]);
      }
      SEQ_LCD_PrintSpaces(3);


      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_IN_UPPER && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	SEQ_LCD_PrintNote(seq_midi_in_upper[selected_bus]);
      }
      SEQ_LCD_PrintSpaces(2);


      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_IN_MODE && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	SEQ_LCD_PrintString(seq_midi_in_options[selected_bus].MODE_PLAY ? "Play" : "T&A ");
      }
      SEQ_LCD_PrintSpaces(3);


      SEQ_LCD_PrintString("Stacks");
    } break;


  ///////////////////////////////////////////////////////////////////////////
    case SUBPAGE_SECTIONS: {
      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintString("Port Chn.  G1   G2   G3   G4  Fwd  Reset");
      SEQ_LCD_CursorSet(40, 1);

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_S_PORT && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	if( seq_midi_in_sect_port )
	  SEQ_LCD_PrintString(SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(seq_midi_in_sect_port)));
	else
	  SEQ_LCD_PrintString(" All");
      }
      SEQ_LCD_PrintSpaces(1);

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_S_CHN && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	if( seq_midi_in_sect_channel )
	  SEQ_LCD_PrintFormattedString("#%2d", seq_midi_in_sect_channel);
	else
	  SEQ_LCD_PrintString("---");
      }
      SEQ_LCD_PrintSpaces(2);

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_S_OCT_G1 && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	SEQ_LCD_PrintNote(seq_midi_in_sect_note[0]);
      }
      SEQ_LCD_PrintSpaces(2);

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_S_OCT_G2 && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	SEQ_LCD_PrintNote(seq_midi_in_sect_note[1]);
      }
      SEQ_LCD_PrintSpaces(2);

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_S_OCT_G3 && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	SEQ_LCD_PrintNote(seq_midi_in_sect_note[2]);
      }
      SEQ_LCD_PrintSpaces(2);

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_S_OCT_G4 && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	SEQ_LCD_PrintNote(seq_midi_in_sect_note[3]);
      }

      SEQ_LCD_PrintSpaces(2);

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_S_FWD_PORT && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	if( seq_midi_in_sect_fwd_port == 0 )
	  SEQ_LCD_PrintString("----");
	else
	  SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(seq_midi_in_sect_fwd_port)));
      }

      SEQ_LCD_PrintSpaces(1);
      SEQ_LCD_PrintString("Stcks");
    } break;


  ///////////////////////////////////////////////////////////////////////////
    case SUBPAGE_ROUTER: {
      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintString("Node IN P/Chn  OUT P/Chn     DefaultPort");
      SEQ_LCD_CursorSet(40, 1);

      ///////////////////////////////////////////////////////////////////////
      SEQ_LCD_PrintSpaces(1);
      if( ui_selected_item == ITEM_R_NODE && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(2);
      } else {
	SEQ_LCD_PrintFormattedString("#%d", selected_router_node+1);
      }
      SEQ_LCD_PrintSpaces(2);

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_R_SRC_PORT && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	SEQ_LCD_PrintString(SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(n->src_port)));
      }
      SEQ_LCD_PrintSpaces(1);

      ///////////////////////////////////////////////////////////////////////
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

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_R_DST_PORT && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(n->dst_port)));
      }
      SEQ_LCD_PrintSpaces(1);

      ///////////////////////////////////////////////////////////////////////
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
      SEQ_LCD_PrintSpaces(9);

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_DEF_PORT && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(MIOS32_MIDI_DefaultPortGet())));
      }
      SEQ_LCD_PrintSpaces(4);
    } break;

    case SUBPAGE_MISC: {
      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintString("BLM_SCALAR ");
      SEQ_LCD_PrintString(blm_timeout_ctr ? "connected    " : "not found    ");
      SEQ_LCD_PrintString("         MIDI   ");
      SEQ_LCD_CursorSet(40, 1);

      ///////////////////////////////////////////////////////////////////////
      SEQ_LCD_PrintString("Port: ");
      if( ui_selected_item == ITEM_BLM_SCALAR_PORT && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	if( !seq_blm_port )
	  SEQ_LCD_PrintString(" off");
	else
	  SEQ_LCD_PrintString(SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_OutIxGet(seq_blm_port)));
      }
      SEQ_LCD_PrintSpaces(22);

      ///////////////////////////////////////////////////////////////////////
      SEQ_LCD_PrintString("Monitor ");

    } break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local exit function
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  s32 status = 0;

  if( store_file_required ) {
    // write config file
    MUTEX_SDCARD_TAKE;
    if( (status=SEQ_FILE_C_Write()) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    MUTEX_SDCARD_GIVE;
  }

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

  store_file_required = 0;

  return 0; // no error
}
