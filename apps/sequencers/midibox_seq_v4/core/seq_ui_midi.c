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
#include <blm_scalar_master.h>
#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"

#include "seq_file.h"
#include "seq_file_c.h"
#include "seq_file_gc.h"

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
#define SUBPAGE_EXT_CTRL   3
#define SUBPAGE_MISC       4



#define NUM_OF_ITEMS       27

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

// Ext. Ctrl
#define ITEM_EXT_PORT      21
#define ITEM_EXT_PORT_OUT  22
#define ITEM_EXT_CHN       23
#define ITEM_EXT_CTRL      24
#define ITEM_EXT_VALUE     25

// Misc
#define ITEM_BLM_SCALAR_PORT 26


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 selected_subpage = SUBPAGE_TRANSPOSE;
static u8 selected_router_node = 0;
static u8 selected_bus = 0;
static u8 selected_ext_ctrl = 0;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  if( selected_subpage <= 5 ) {
    const u8 select_led_pattern[5] = { 0x03, 0x0c, 0x30, 0x40, 0x80 };
    *gp_leds = (u16)select_led_pattern[selected_subpage];
  }

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

    case SUBPAGE_EXT_CTRL:
      switch( ui_selected_item ) {
        case ITEM_EXT_PORT:     *gp_leds |= 0x0100; break;
        case ITEM_EXT_PORT_OUT: *gp_leds |= 0x0200; break;
        case ITEM_EXT_CHN:      *gp_leds |= 0x0400; break;
        case ITEM_EXT_CTRL:     *gp_leds |= 0x3800; break;
        case ITEM_EXT_VALUE:    *gp_leds |= 0x4000; break;
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
  else if( item <= ITEM_EXT_VALUE )
    selected_subpage = SUBPAGE_EXT_CTRL;
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
    switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
    case SEQ_UI_ENCODER_GP2:
      selected_subpage = SUBPAGE_TRANSPOSE;
      ui_selected_item = ITEM_IN_BUS;
      break;

    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
      selected_subpage = SUBPAGE_SECTIONS;
      ui_selected_item = ITEM_S_PORT;
      break;

    case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6:
      selected_subpage = SUBPAGE_ROUTER;
      ui_selected_item = ITEM_R_NODE;
      break;

    case SEQ_UI_ENCODER_GP7:
      selected_subpage = SUBPAGE_EXT_CTRL;
      ui_selected_item = ITEM_EXT_PORT;
      break;

    default:
      selected_subpage = SUBPAGE_MISC;
      ui_selected_item = ITEM_BLM_SCALAR_PORT;
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

    case SUBPAGE_EXT_CTRL:
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP9:
	  ui_selected_item = ITEM_EXT_PORT;
	  break;

        case SEQ_UI_ENCODER_GP10:
	  ui_selected_item = ITEM_EXT_PORT_OUT;
	  break;

        case SEQ_UI_ENCODER_GP11:
	  ui_selected_item = ITEM_EXT_CHN;
	  break;

        case SEQ_UI_ENCODER_GP12:
        case SEQ_UI_ENCODER_GP13:
        case SEQ_UI_ENCODER_GP14:
	  ui_selected_item = ITEM_EXT_CTRL;
	  break;

        case SEQ_UI_ENCODER_GP15:
        case SEQ_UI_ENCODER_GP16:
	  ui_selected_item = ITEM_EXT_VALUE;
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
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_InNumGet()-1, incrementer) >= 0 ) {
	seq_midi_in_port[selected_bus] = SEQ_MIDI_PORT_InPortGet(port_ix);
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_IN_CHN:
      if( SEQ_UI_Var8_Inc(&seq_midi_in_channel[selected_bus], 0, 17, incrementer) >= 0 ) {
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_IN_LOWER:
      if( SEQ_UI_Var8_Inc(&seq_midi_in_lower[selected_bus], 0, 127, incrementer) >= 0 ) {
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_IN_UPPER:
      if( SEQ_UI_Var8_Inc(&seq_midi_in_upper[selected_bus], 0, 127, incrementer) >= 0 ) {
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_IN_MODE: {
      u8 fwd = seq_midi_in_options[selected_bus].MODE_PLAY;
      if( incrementer == 0 )
	incrementer = fwd ? -1 : 1;
      if( SEQ_UI_Var8_Inc(&fwd, 0, 1, incrementer) >= 0 ) {
	seq_midi_in_options[selected_bus].MODE_PLAY = fwd;
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_RESET_STACKS: {
      SEQ_MIDI_IN_ResetTransArpStacks();
      SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Transposer/Arp.", "Stacks cleared!");

      // send to external
      SEQ_MIDI_IN_ExtCtrlSend(SEQ_MIDI_IN_EXT_CTRL_ALL_NOTES_OFF, 127, 0);

      return 1;
    } break;


    case ITEM_S_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_InIxGet(seq_midi_in_sect_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_InNumGet()-1, incrementer) >= 0 ) {
	seq_midi_in_sect_port = SEQ_MIDI_PORT_InPortGet(port_ix);
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_S_CHN:
      if( SEQ_UI_Var8_Inc(&seq_midi_in_sect_channel, 0, 16, incrementer) >= 0 ) {
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change


    case ITEM_S_OCT_G1: {
      u8 oct = seq_midi_in_sect_note[0] / 12;
      if( SEQ_UI_Var8_Inc(&oct, 0, 11, incrementer) >= 0 ) {
	seq_midi_in_sect_note[0] = 12*oct;
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_S_OCT_G2: {
      u8 oct = seq_midi_in_sect_note[1] / 12;
      if( SEQ_UI_Var8_Inc(&oct, 0, 11, incrementer) >= 0 ) {
	seq_midi_in_sect_note[1] = 12*oct;
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_S_OCT_G3: {
      u8 oct = seq_midi_in_sect_note[2] / 12;
      if( SEQ_UI_Var8_Inc(&oct, 0, 11, incrementer) >= 0 ) {
	seq_midi_in_sect_note[2] = 12*oct;
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_S_OCT_G4: {
      u8 oct = seq_midi_in_sect_note[3] / 12;
      if( SEQ_UI_Var8_Inc(&oct, 0, 11, incrementer) >= 0 ) {
	seq_midi_in_sect_note[3] = 12*oct;
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_S_FWD_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_OutIxGet(seq_midi_in_sect_fwd_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	seq_midi_in_sect_fwd_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_S_RESET_STACKS: {
      SEQ_MIDI_IN_ResetChangerStacks();
      SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Section Changer", "Stacks cleared!");

      // send to external
      SEQ_MIDI_IN_ExtCtrlSend(SEQ_MIDI_IN_EXT_CTRL_ALL_NOTES_OFF, 127, 0);

      return 1;
    } break;



    case ITEM_R_NODE:
      if( SEQ_UI_Var8_Inc(&selected_router_node, 0, SEQ_MIDI_ROUTER_NUM_NODES-1, incrementer) >= 0 ) {
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_R_SRC_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_InIxGet(n->src_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_InNumGet()-1, incrementer) >= 0 ) {
	n->src_port = SEQ_MIDI_PORT_InPortGet(port_ix);
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_R_SRC_CHN:
      if( SEQ_UI_Var8_Inc(&n->src_chn, 0, 17, incrementer) >= 0 ) {
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_R_DST_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_OutIxGet(n->dst_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	n->dst_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_R_DST_CHN:
      if( SEQ_UI_Var8_Inc(&n->dst_chn, 0, 19, incrementer) >= 0 ) {
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change


    case ITEM_DEF_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_OutIxGet(MIOS32_MIDI_DefaultPortGet());
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	MIOS32_MIDI_DefaultPortSet(SEQ_MIDI_PORT_OutPortGet(port_ix));
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_EXT_PORT: {
      u8 numPorts = SEQ_MIDI_PORT_InNumGet();
      u8 port_ix = (seq_midi_in_ext_ctrl_port == 0xff) ? numPorts : SEQ_MIDI_PORT_InIxGet(seq_midi_in_ext_ctrl_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, numPorts, incrementer) >= 0 ) {
	seq_midi_in_ext_ctrl_port = (port_ix == numPorts) ? 0xff : SEQ_MIDI_PORT_InPortGet(port_ix);
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_EXT_PORT_OUT: {
      u8 port_ix = SEQ_MIDI_PORT_OutIxGet(seq_midi_in_ext_ctrl_out_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	seq_midi_in_ext_ctrl_out_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_EXT_CHN:
      if( SEQ_UI_Var8_Inc(&seq_midi_in_ext_ctrl_channel, 0, 16, incrementer) >= 0 ) {
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_EXT_CTRL:
      if( SEQ_UI_Var8_Inc(&selected_ext_ctrl, 0, SEQ_MIDI_IN_EXT_CTRL_NUM-1, incrementer) >= 0 ) {
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_EXT_VALUE:
      if( selected_ext_ctrl == SEQ_MIDI_IN_EXT_CTRL_NRPN_ENABLED ) {
	if( SEQ_UI_Var8_Inc(&seq_midi_in_ext_ctrl_asg[selected_ext_ctrl], 0, 1, incrementer) >= 0 ) {
	  ui_store_file_required = 1;
	  return 1; // value changed
	}
      } else if( selected_ext_ctrl == SEQ_MIDI_IN_EXT_CTRL_PC_MODE ) {
	if( SEQ_UI_Var8_Inc(&seq_midi_in_ext_ctrl_asg[selected_ext_ctrl], 0, SEQ_MIDI_IN_EXT_CTRL_PC_MODE_NUM-1, incrementer) >= 0 ) {
	  ui_store_file_required = 1;
	  return 1; // value changed
	}
      } else if( selected_ext_ctrl <= SEQ_MIDI_IN_EXT_CTRL_NUM ) {
	if( SEQ_UI_Var8_Inc(&seq_midi_in_ext_ctrl_asg[selected_ext_ctrl], 0, 128, incrementer) >= 0 ) {
	  ui_store_file_required = 1;
	  return 1; // value changed
	}
      }
      return 0; // no change

    case ITEM_BLM_SCALAR_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_InIxGet(BLM_SCALAR_MASTER_MIDI_PortGet(0));
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_InNumGet()-1, incrementer) >= 0 ) {
	BLM_SCALAR_MASTER_MIDI_PortSet(0, SEQ_MIDI_PORT_InPortGet(port_ix));
	MUTEX_MIDIOUT_TAKE;
	BLM_SCALAR_MASTER_TimeoutCtrSet(0, 0); // fake timeout (so that "BLM not found" message will be displayed)
	BLM_SCALAR_MASTER_SendRequest(0, 0x00); // request layout from BLM_SCALAR
	MUTEX_MIDIOUT_GIVE;
	ui_store_file_required = 1;
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
      else
	--ui_selected_item;
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
  // Transposer  Section    MIDI   Ext.       Bus Port Chn. Lower/Upper Mode   Reset 
  //  and Arp.   Control   Router  Ctrl Misc.  1  IN1  #16   ---   ---  T&A    Stacks

  // Transposer  Section    MIDI   Ext.      Port Chn.  G1   G2   G3   G4  Fwd  Reset
  //  and Arp.   Control   Router  Ctrl Misc. All #16  C-1  C-2  C-3  C-4  USB1 Stcks

  // Transposer  Section    MIDI   Ext.      Node IN P/Chn  OUT P/Chn   | DefaultPort
  //  and Arp.   Control   Router  Ctrl Misc. #1  Def. All  Def. # 1    |    USB1    

  // Transposer  Section    MIDI   Ext.      Port Chn.      Function        CC#      
  // Transposer  Section    MIDI   Ext.       IN  OUT Chn.| Function        CC#      
  //  and Arp.   Control   Router  Ctrl Misc. All off --- | Morph Value       1      

  // Transposer  Section    MIDI   Ext.      BLM_SCALAR                       MIDI   
  //  and Arp.   Control   Router  Ctrl Misc.Port: OUT2                     Monitor 


  seq_midi_router_node_t *n = &seq_midi_router_node[selected_router_node];

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Transposer  Section    MIDI   Ext.      ");
  SEQ_LCD_CursorSet(0, 1);
  SEQ_LCD_PrintString(" and Arp.   Control   Router  Ctrl Misc.");

  if( ui_cursor_flash && selected_subpage <= 5 ) {
    const u8 select_pos1[5]  = {  0, 10, 20, 30, 35 };
    const u8 select_width[5] = { 10, 10, 10,  5,  5 };

    int line;
    for(line=0; line<2; ++line) {
      SEQ_LCD_CursorSet(select_pos1[selected_subpage], line);
      SEQ_LCD_PrintSpaces(select_width[selected_subpage]);
    }
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
	if( seq_midi_in_channel[selected_bus] == 17 )
	  SEQ_LCD_PrintString("All");
	else if( seq_midi_in_channel[selected_bus] )
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
	SEQ_LCD_PrintString(seq_midi_in_options[selected_bus].MODE_PLAY ? "Jam " : "T&A ");
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
      SEQ_LCD_PrintString("Node IN P/Chn  OUT P/Chn   | DefaultPort");
      SEQ_LCD_CursorSet(40, 1);

      ///////////////////////////////////////////////////////////////////////
      SEQ_LCD_PrintSpaces(1);
      if( ui_selected_item == ITEM_R_NODE && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	SEQ_LCD_PrintFormattedString("#%2d", selected_router_node+1);
      }
      SEQ_LCD_PrintSpaces(1);

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
	if( n->dst_chn >= 18 ) {
	  SEQ_LCD_PrintSpaces(2);
	} else {
	  SEQ_LCD_PrintSpaces(4);
	}
      } else {
	if( n->dst_chn >= 18 ) {
	  SEQ_LCD_PrintSpaces(2);
	} else {
	  SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(n->dst_port)));
	}
      }

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_R_DST_CHN && ui_cursor_flash ) {
	if( n->dst_chn >= 18 ) {
	  SEQ_LCD_PrintSpaces(7);
	} else {
	  SEQ_LCD_PrintSpaces(5);
	}
      } else {
	if( !n->dst_chn ) {
	  SEQ_LCD_PrintString(" --- ");
	} else if( n->dst_chn == 17 ) {
	  SEQ_LCD_PrintString(" All ");
	} else if( n->dst_chn == 18 ) {
	  SEQ_LCD_PrintString("  Track");
	} else if( n->dst_chn >= 19 ) {
	  SEQ_LCD_PrintString("Sel.Trk");
	} else {
	  SEQ_LCD_PrintFormattedString(" #%2d ", n->dst_chn);
	}
      }
      SEQ_LCD_PrintSpaces(3);

      ///////////////////////////////////////////////////////////////////////
      SEQ_LCD_PrintChar('|');
      SEQ_LCD_PrintSpaces(4);

      if( ui_selected_item == ITEM_DEF_PORT && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(MIOS32_MIDI_DefaultPortGet())));
      }
      SEQ_LCD_PrintSpaces(4);
    } break;


  ///////////////////////////////////////////////////////////////////////////
    case SUBPAGE_EXT_CTRL: {
      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintString(" IN  OUT  Chn.|Function        ");
      if( selected_ext_ctrl < SEQ_MIDI_IN_EXT_CTRL_NUM_IX_CC ) {
	SEQ_LCD_PrintString("CC#      ");
      } else {
	SEQ_LCD_PrintSpaces(10);
      }
      SEQ_LCD_CursorSet(40, 1);

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_EXT_PORT && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	if( seq_midi_in_ext_ctrl_port == 0xff )
	  SEQ_LCD_PrintString(" All");
	else if( !seq_midi_in_ext_ctrl_port )
	  SEQ_LCD_PrintString(" off");
	else
	  SEQ_LCD_PrintString(SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(seq_midi_in_ext_ctrl_port)));
      }
      SEQ_LCD_PrintSpaces(1);

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_EXT_PORT_OUT && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	if( seq_midi_in_ext_ctrl_out_port )
	  SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(seq_midi_in_ext_ctrl_out_port)));
	else
	  SEQ_LCD_PrintString("off ");
      }
      SEQ_LCD_PrintSpaces(1);

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_EXT_CHN && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	if( seq_midi_in_ext_ctrl_channel )
	  SEQ_LCD_PrintFormattedString("#%2d", seq_midi_in_ext_ctrl_channel);
	else
	  SEQ_LCD_PrintString("---");
      }
      SEQ_LCD_PrintString(" |");

      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_EXT_CTRL && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(15);
      } else {
	SEQ_LCD_PrintStringPadded((char *)SEQ_MIDI_IN_ExtCtrlStr(selected_ext_ctrl), 15);
      }


      ///////////////////////////////////////////////////////////////////////
      if( ui_selected_item == ITEM_EXT_VALUE && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(10);
      } else {
	SEQ_LCD_PrintSpaces(1);
	if( selected_ext_ctrl == SEQ_MIDI_IN_EXT_CTRL_NRPN_ENABLED ) {
	  SEQ_LCD_PrintStringPadded(seq_midi_in_ext_ctrl_asg[selected_ext_ctrl] ? "enabled" : "disabled", 9);
	} else if( selected_ext_ctrl == SEQ_MIDI_IN_EXT_CTRL_PC_MODE ) {
	  SEQ_LCD_PrintStringPadded((char *)SEQ_MIDI_IN_ExtCtrlPcModeStr(seq_midi_in_ext_ctrl_asg[selected_ext_ctrl]), 9);
	} else {
	  u8 cc = seq_midi_in_ext_ctrl_asg[selected_ext_ctrl];
	  if( cc >= 0x80 )
	    SEQ_LCD_PrintString("off");
	  else
	    SEQ_LCD_PrintFormattedString("%3d", cc);
	  SEQ_LCD_PrintSpaces(7);
	}
      }

    } break;


  ///////////////////////////////////////////////////////////////////////////
    case SUBPAGE_MISC: {
      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintString("BLM_SCALAR                       MIDI   ");
      SEQ_LCD_CursorSet(40, 1);

      ///////////////////////////////////////////////////////////////////////
      SEQ_LCD_PrintString("Port: ");
      if( ui_selected_item == ITEM_BLM_SCALAR_PORT && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	mios32_midi_port_t blm_port = BLM_SCALAR_MASTER_MIDI_PortGet(0);
	if( !blm_port )
	  SEQ_LCD_PrintString(" off");
	else
	  SEQ_LCD_PrintString(SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(blm_port)));
      }

      SEQ_LCD_PrintString(BLM_SCALAR_MASTER_TimeoutCtrGet(0) ? " (found)  " : "          ");

      // free for new parameters
      SEQ_LCD_PrintSpaces(12);

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

  if( ui_store_file_required ) {
    // write config files
    MUTEX_SDCARD_TAKE;
    if( (status=SEQ_FILE_C_Write(seq_file_session_name)) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    MUTEX_SDCARD_GIVE;

    MUTEX_SDCARD_TAKE;
    if( (status=SEQ_FILE_GC_Write()) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    MUTEX_SDCARD_GIVE;

    ui_store_file_required = 0;
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

  return 0; // no error
}
