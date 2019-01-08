// $Id: seq_ui_opt.c 2102 2014-12-15 17:40:04Z tk $
/*
 * Options page
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

#include "seq_file.h"
#include "seq_file_c.h"
#include "seq_file_gc.h"

#include "seq_core.h"
#include "seq_cc.h"
#include "seq_midi_port.h"
#include "seq_midi_sysex.h"
#include "seq_hwcfg.h"
#include "seq_mixer.h"
#include "seq_tpd.h"
#include "seq_blm.h"

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define ITEM_STEPS_MEASURE   0
#define ITEM_STEPS_PATTERN   1
#define ITEM_SYNC_CHANGE     2
#define ITEM_RATOPC          3
#define ITEM_PATTERN_MIXER_MAP_COUPLING 4
#define ITEM_SYNC_MUTE       5
#define ITEM_SYNC_UNMUTE     6
#define ITEM_PASTE_CLR_ALL   7
#define ITEM_INIT_CC         8
#define ITEM_LIVE_LAYER_MUTE 9
#define ITEM_909_MULTIMACHINE 10
#define ITEM_909_NON909       11
#define ITEM_EXPERT_MODE      12
#define ITEM_TPD_MODE        13
#define ITEM_BLM_ALWAYS_USE_FTS 14
#define ITEM_MIXER_CC1234    15

#define NUM_OF_ITEMS         16

const u8 expert_items[NUM_OF_ITEMS] = { 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1};

static const char *item_text[NUM_OF_ITEMS][2] = {
  
  {//<-------------------------------------->
    "Track Synchronisation",
    "Steps per Measure:   " // %3d
  },

  {//<-------------------------------------->
    "Pattern Change Synchr",
    "Change considered    " // %3d steps
  },

  {//<-------------------------------------->
    "Pattern Change       ",
    "Synchronisation      ", // enabled/disabled
  },

  {//<-------------------------------------->
    "Restart all Tracks   ",
    "on Pattern Change    ", // enabled/disabled
  },

  {//<-------------------------------------->
    "Dump predef. Mixer   ",
    "Map on patrn Changes:", // enabled/disabled
  },

  {//<-------------------------------------->
    "Synchronize MUTE     ",
    "to Measure           ", // enabled/disabled
  },

  {//<-------------------------------------->
    "Synchronize UNMUTE   ",
    "to Measure           ", // enabled/disabled
  },

  {//<-------------------------------------->
    "Paste & Clear button ",
    "will modify          ", // Only Steps/Complete Track
  },

  {//<-------------------------------------->
    "Initial CC value for ",
    "Clear and Init is:   ",
  },

  {//<-------------------------------------->
    "If Live function,    ",
    "matching received    ",
  },

{//<-------------------------------------->
    "Use multimachine to  ",
    "play 4 tracks in song",
  },

{//<-------------------------------------->
    "Use non909 to play   ",
    "16 tracks in song    ",
  },  
{//<-------------------------------------->
    "expert mode unlocks  ",
    "all the possibilities",
  },  
  {//<-------------------------------------->
    "Track Position Displ ",
    "(TPD) Mode           "
  },

  {//<-------------------------------------->
    "BLM16x16+X always use",
    "Forc-To-Scal in Grid " // yes/no
  },

  {//<-------------------------------------->
    "Mixr CCs which should",
    "be sent after PC     "
  },
};

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 local_selected_item = 0; // stays stable when menu is exit


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  *gp_leds = 0x0000;

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
//<<<<<<< .mine
  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
	//if( SEQ_UI_Var8_Inc(&ui_selected_item, 0, NUM_OF_ITEMS, incrementer) >= 1 )
	if( SEQ_UI_Var8_Inc(&local_selected_item, 0, NUM_OF_ITEMS-1, incrementer) >= 0 ){
		while(expert_items[local_selected_item]==1 && seq_core_glb_mb909_expert==1 && (local_selected_item <=(NUM_OF_ITEMS-1))){
			//DEBUG_MSG("dit is een expert optie");
			SEQ_UI_Var8_Inc(&local_selected_item, 0, NUM_OF_ITEMS-1, incrementer);
			if(local_selected_item ==(NUM_OF_ITEMS-1))
				local_selected_item = 0;
		}
		return 1;
      }else
	return 0;
      break;


    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
	case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6:	
	 return -1; // not mapped
	break;

	}
  // change page
  /*
  if( encoder == SEQ_UI_ENCODER_GP7 || encoder == SEQ_UI_ENCODER_GP8 ) {
    if( SEQ_UI_Var8_Inc(&local_selected_item, 0, NUM_OF_ITEMS-1, incrementer) >= 0 ) {
      return 1; // changed
    }
    return 0; // not changed
  }
  */
  
  // for GP encoders and Datawheel
  switch( local_selected_item ) {
    case ITEM_STEPS_MEASURE:
      //if( encoder == SEQ_UI_ENCODER_GP1 ) {
	  if( encoder == SEQ_UI_ENCODER_Datawheel && (seq_hwcfg_mb909.enabled)    ) {
	// increment in +/- 16 steps
	//DEBUG_MSG("encoder 1 gewijzigd maar ook steps measure aangepast, shit"); this is ok, never happens
	u8 value = seq_core_steps_per_measure >> 4;
	if( SEQ_UI_Var8_Inc(&value, 0, 15, incrementer) >= 0 ) {
	  seq_core_steps_per_measure = (value << 4) + 15;
	  ui_store_file_required = 1;
	  return 1;
	}
      } else {
	if( SEQ_UI_Var8_Inc(&seq_core_steps_per_measure, 0, 255, incrementer) >= 0 ) {
	  ui_store_file_required = 1;
	  return 1;
	}
      }
      return 0;

    case ITEM_STEPS_PATTERN:

      if( encoder == SEQ_UI_ENCODER_GP3 || ( encoder == SEQ_UI_ENCODER_Datawheel && (seq_hwcfg_mb909.enabled)    )) {

	u8 value = seq_core_steps_per_pattern >> 4;
	if( SEQ_UI_Var8_Inc(&value, 0, 15, incrementer) >= 0 ) {
	  seq_core_steps_per_pattern = (value << 4) + 15;
	  ui_store_file_required = 1;
	  return 1;
	}
      } else {
	if( SEQ_UI_Var8_Inc(&seq_core_steps_per_pattern, 0, 255, incrementer) >= 0 ) {
	  ui_store_file_required = 1;
	  return 1;
	}
      }
      return 0;

    case ITEM_SYNC_CHANGE:
      if( incrementer )
	seq_core_options.SYNCHED_PATTERN_CHANGE = incrementer > 0 ? 1 : 0;
      else
	seq_core_options.SYNCHED_PATTERN_CHANGE ^= 1;
      ui_store_file_required = 1;
      return 1;

    case ITEM_PATTERN_MIXER_MAP_COUPLING:
      if( incrementer )
	seq_core_options.PATTERN_MIXER_MAP_COUPLING = incrementer > 0 ? 1 : 0;
      else
	seq_core_options.PATTERN_MIXER_MAP_COUPLING ^= 1;
      ui_store_file_required = 1;
      return 1;

    case ITEM_PASTE_CLR_ALL:
      if( incrementer )
	seq_core_options.PASTE_CLR_ALL = incrementer > 0 ? 1 : 0;
      else
	seq_core_options.PASTE_CLR_ALL ^= 1;
      ui_store_file_required = 1;
      return 1;

    case ITEM_RATOPC:
      if( incrementer )
	seq_core_options.RATOPC = incrementer > 0 ? 1 : 0;
      else
	seq_core_options.RATOPC ^= 1;
      ui_store_file_required = 1;
      return 1;

    case ITEM_SYNC_MUTE: {
      if( incrementer )
	seq_core_options.SYNCHED_MUTE = incrementer > 0 ? 1 : 0;
      else
	seq_core_options.SYNCHED_MUTE ^= 1;
      ui_store_file_required = 1;
      return 1;
    } break;

    case ITEM_SYNC_UNMUTE: {
      if( incrementer )
	seq_core_options.SYNCHED_UNMUTE = incrementer > 0 ? 1 : 0;
      else
	seq_core_options.SYNCHED_UNMUTE ^= 1;
      ui_store_file_required = 1;
      return 1;
    } break;

    case ITEM_INIT_CC: {
      u8 value = seq_core_options.INIT_CC;
      if( SEQ_UI_Var8_Inc(&value, 0, 127, incrementer) >= 0 ) {
	seq_core_options.INIT_CC = value;
	ui_store_file_required = 1;
	return 1;
      }
      return 0;
    } break;

    case ITEM_LIVE_LAYER_MUTE: {
      u8 value = seq_core_options.LIVE_LAYER_MUTE_STEPS;
      if( SEQ_UI_Var8_Inc(&value, 0, 7, incrementer) >= 0 ) {
	seq_core_options.LIVE_LAYER_MUTE_STEPS = value;
	ui_store_file_required = 1;
	return 1;
      }
      return 0;
    } break;


    case ITEM_909_MULTIMACHINE: {
      if( incrementer )
	seq_core_glb_mb909_multimachine = incrementer > 0 ? 1 : 0;
      //else
	//seq_core_options.SYNCHED_MUTE ^= 1;
      ui_store_file_required = 1;
      return 1;
    } break;
	  
    case ITEM_909_NON909: {
      if( incrementer )
	seq_core_glb_mb909_non909 = incrementer > 0 ? 1 : 0;
      //else
	//seq_core_options.SYNCHED_MUTE ^= 1;
      ui_store_file_required = 1;
      return 1;
    } break;
	
	
    case ITEM_EXPERT_MODE: {
      if( incrementer )
	seq_core_glb_mb909_expert = incrementer > 0 ? 1 : 0;
      //else
	//seq_core_options.SYNCHED_MUTE ^= 1;
      ui_store_file_required = 1;
      return 1;
    } break;	
	
    case ITEM_TPD_MODE: {
      u8 value = SEQ_TPD_ModeGet();
      if( SEQ_UI_Var8_Inc(&value, 0, SEQ_TPD_NUM_MODES-1, incrementer) >= 0 ) {
	SEQ_TPD_ModeSet(value);
	ui_store_file_required = 1;
	return 1;
      }
      return 0;
    } break;

    case ITEM_BLM_ALWAYS_USE_FTS: {
      if( incrementer )
	seq_blm_options.ALWAYS_USE_FTS = incrementer > 0 ? 1 : 0;
      else
	seq_blm_options.ALWAYS_USE_FTS ^= 1;
      ui_store_file_required = 1;
      return 1;
    } break;

    case ITEM_MIXER_CC1234: {
      u8 pos = (encoder-8) / 2;
      u8 value = (seq_mixer_cc1234_before_pc & (1 << pos)) ? 0 : 1; // note: we've to invert yes/no!
      if( SEQ_UI_Var8_Inc(&value, 0, 1, incrementer) >= 0 ) {
	if( !value ) // note: we've to invert yes/no
	  seq_mixer_cc1234_before_pc |= (1 << pos);
	else
	  seq_mixer_cc1234_before_pc &= ~(1 << pos);

	ui_store_file_required = 1;
	return 1;
      }
      return 0;
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
      if( depressed ) return -1;
      if( ++local_selected_item >= NUM_OF_ITEMS )
	local_selected_item = 0;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( depressed ) return -1;
      if( local_selected_item == 0 )
	local_selected_item = NUM_OF_ITEMS-1;
      else
	--local_selected_item;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Up:
      if( depressed ) return -1;
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Down:
      if( depressed ) return -1;
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
  //                                 Option  Track Synchronisation
  //                                   1/10  Steps per Measure:  16

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);

  SEQ_LCD_PrintString("    OPTIONS: ");
  SEQ_LCD_PrintFormattedString("  %2d/%-2d", local_selected_item+1, NUM_OF_ITEMS);
  SEQ_LCD_CursorSet(0, 3);
  SEQ_LCD_PrintStringPadded((char *)item_text[local_selected_item][0], 40);
  
  ///////////////////////////////////////////////////////////////////////////

  //SEQ_LCD_PrintSpaces(33);
  

  ///////////////////////////////////////////////////////////////////////////
  char *str = (char *)item_text[local_selected_item][1];
  //u32 len = strlen(str);
  int enabled_value = -1; // cheap: will print enabled/disabled in second line if >= 0

  
  SEQ_LCD_CursorSet(0, 4);
  switch( local_selected_item ) {

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_STEPS_MEASURE: {
    SEQ_LCD_PrintString(str);
	SEQ_LCD_CursorSet(0, 5);
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      SEQ_LCD_PrintFormattedString("%3d", (int)seq_core_steps_per_measure + 1);
    }
    SEQ_LCD_PrintSpaces(21-3);
	SEQ_LCD_CursorSet(0, 6);
	SEQ_LCD_PrintSpaces(21);
  } break;
  
  //SEQ_LCD_CursorSet(0, 3);
  ///////////////////////////////////////////////////////////////////////////
  case ITEM_STEPS_PATTERN: {
    SEQ_LCD_PrintString(str);
	SEQ_LCD_CursorSet(0, 5);
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(9);
    } else {
      SEQ_LCD_PrintFormattedString("each %3d steps", (int)seq_core_steps_per_pattern + 1);
    }
    SEQ_LCD_PrintSpaces(21-9);
	SEQ_LCD_CursorSet(0, 6);
	SEQ_LCD_PrintSpaces(21);	
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_SYNC_CHANGE: {
	SEQ_LCD_PrintString(str);
	SEQ_LCD_CursorSet(0, 5);
    enabled_value = seq_core_options.SYNCHED_PATTERN_CHANGE;
	SEQ_LCD_CursorSet(0, 6);
	SEQ_LCD_PrintSpaces(21);
	SEQ_LCD_CursorSet(0, 5);	
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_RATOPC: {
	SEQ_LCD_PrintString(str);
	SEQ_LCD_CursorSet(0, 5);  
    enabled_value = seq_core_options.RATOPC;
	SEQ_LCD_CursorSet(0, 6);
	SEQ_LCD_PrintSpaces(21);
	SEQ_LCD_CursorSet(0, 5);	
  } break;

//>>>>>>> .r1826
  ///////////////////////////////////////////////////////////////////////////
  case ITEM_PATTERN_MIXER_MAP_COUPLING: {
    SEQ_LCD_PrintString(str);
	SEQ_LCD_CursorSet(0, 5);
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(21);
    } else {
      SEQ_LCD_PrintStringPadded(seq_core_options.PATTERN_MIXER_MAP_COUPLING ? "enabled" : "disabled", 21);
    }
	SEQ_LCD_CursorSet(0, 6);
	SEQ_LCD_PrintSpaces(21);	
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_SYNC_MUTE: {
    SEQ_LCD_PrintString(str);
	
	SEQ_LCD_CursorSet(0, 5);  
    enabled_value = seq_core_options.SYNCHED_MUTE;
	SEQ_LCD_CursorSet(0, 6);
	SEQ_LCD_PrintSpaces(21);
	SEQ_LCD_CursorSet(0, 5);	
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_SYNC_UNMUTE: {
    SEQ_LCD_PrintString(str);
	
	SEQ_LCD_CursorSet(0, 5);  
    enabled_value = seq_core_options.SYNCHED_UNMUTE;
	SEQ_LCD_CursorSet(0, 6);
	SEQ_LCD_PrintSpaces(21);
	SEQ_LCD_CursorSet(0, 5);	
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_PASTE_CLR_ALL: {
    SEQ_LCD_PrintString(str);
	
	SEQ_LCD_CursorSet(0, 5);
	if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(14);
    } else {
      SEQ_LCD_PrintFormattedString("%-8s", seq_core_options.PASTE_CLR_ALL ? "Complete Track" : "Only Steps    ");
    }
    SEQ_LCD_PrintSpaces(21-14);
	SEQ_LCD_CursorSet(0, 6);
	SEQ_LCD_PrintSpaces(21);	
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_INIT_CC: {
    SEQ_LCD_PrintString(str);
	SEQ_LCD_CursorSet(0, 5);	
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      SEQ_LCD_PrintFormattedString("%3d", seq_core_options.INIT_CC);
    }
    SEQ_LCD_PrintSpaces(21-3);
	SEQ_LCD_CursorSet(0, 6);
	SEQ_LCD_PrintSpaces(21);	
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_LIVE_LAYER_MUTE: {
    SEQ_LCD_PrintString(str);
	SEQ_LCD_CursorSet(0, 5);
	SEQ_LCD_PrintString("MIDI Evnts will:     ");
	SEQ_LCD_CursorSet(0, 6);
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(21);
    } else {
      if( seq_core_options.LIVE_LAYER_MUTE_STEPS == 0 ) {
	SEQ_LCD_PrintStringPadded("do nothing", 21);
      } else if( seq_core_options.LIVE_LAYER_MUTE_STEPS == 1 ) {
	SEQ_LCD_PrintStringPadded("mute the appr. layer", 21);
      } else if( seq_core_options.LIVE_LAYER_MUTE_STEPS == 2 ) {
	SEQ_LCD_PrintStringPadded("mute layer for 1 step ", 21);
      } else {
	SEQ_LCD_PrintFormattedString("mute layer for %d step", seq_core_options.LIVE_LAYER_MUTE_STEPS-1);
      }
    }
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_909_MULTIMACHINE: {
	SEQ_LCD_PrintString(str);
	SEQ_LCD_CursorSet(0, 5);
    enabled_value = seq_core_glb_mb909_multimachine;
	SEQ_LCD_CursorSet(0, 6);
	SEQ_LCD_PrintSpaces(21);
	SEQ_LCD_CursorSet(0, 5);	
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_909_NON909: {
	SEQ_LCD_PrintString(str);
	SEQ_LCD_CursorSet(0, 5);
    enabled_value = seq_core_glb_mb909_non909;
	SEQ_LCD_CursorSet(0, 6);
	SEQ_LCD_PrintSpaces(21);
	SEQ_LCD_CursorSet(0, 5);	
  } break;  
  
  ///////////////////////////////////////////////////////////////////////////
  case ITEM_EXPERT_MODE: {
	SEQ_LCD_PrintString(str);
	SEQ_LCD_CursorSet(0, 5);
    enabled_value = seq_core_glb_mb909_expert;
	SEQ_LCD_CursorSet(0, 6);
	SEQ_LCD_PrintSpaces(21);
	SEQ_LCD_CursorSet(0, 5);	
  } break; 
  
  ///////////////////////////////////////////////////////////////////////////
  case ITEM_TPD_MODE: {
    SEQ_LCD_PrintString(str);

    //if( ui_cursor_flash ) {
    //  SEQ_LCD_PrintSpaces(20);
    //} else {
      const char *tpd_mode_str[SEQ_TPD_NUM_MODES] = {
       //<-------------------------------------->
	"Green LED: Pos      ",
	"Green LED: Pos      ",
	"Green LED: Meter    ",
	"Green LED: Meter    ",
	"Green LED: DotMeter ",
	"Green LED: DotMeter ",
      };
      const char *tpd_mode_str2[SEQ_TPD_NUM_MODES] = {
       //<-------------------------------------->
	"Red LED: Track      ",
	"Red LED: Track (Rot)",
	"Red LED: Pos        ",
	"Red LED: Pos   (Rot)",
	"Red LED: Pos        ",
	"Red LED: Pos   (Rot)",
      };
      u8 mode = SEQ_TPD_ModeGet();
      if( mode >= SEQ_TPD_NUM_MODES )
	mode = 0;
	SEQ_LCD_CursorSet(0, 5);
      SEQ_LCD_PrintStringPadded((char *)tpd_mode_str[mode], 20);
	SEQ_LCD_CursorSet(0, 6);  
      SEQ_LCD_PrintStringPadded((char *)tpd_mode_str2[mode], 20);	
    //}
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_BLM_ALWAYS_USE_FTS: {
    SEQ_LCD_PrintString(str);
	SEQ_LCD_CursorSet(0, 5);
    //if( ui_cursor_flash ) {
    //  SEQ_LCD_PrintSpaces(20);
    //} else {
      SEQ_LCD_PrintStringPadded(seq_blm_options.ALWAYS_USE_FTS ? "yes" : "no", 20);
    //}
	SEQ_LCD_CursorSet(0, 6);
	SEQ_LCD_PrintSpaces(21);
	SEQ_LCD_CursorSet(0, 5);	
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_MIXER_CC1234: {
    SEQ_LCD_PrintString(str);
	SEQ_LCD_CursorSet(0, 5);
    //if( ui_cursor_flash ) {
    //  SEQ_LCD_PrintSpaces(20);

    //} else {
      SEQ_LCD_PrintFormattedString(" CC1:%s   CC2:%s    ",
				   (seq_mixer_cc1234_before_pc & 0x1) ? " no" : "yes",
				   (seq_mixer_cc1234_before_pc & 0x2) ? " no" : "yes");
	SEQ_LCD_CursorSet(0, 6);
	SEQ_LCD_PrintFormattedString(" CC3:%s   CC4:%s     ",
				   (seq_mixer_cc1234_before_pc & 0x4) ? " no" : "yes",
				   (seq_mixer_cc1234_before_pc & 0x8) ? " no" : "yes");
	//}
  } break;
  ///////////////////////////////////////////////////////////////////////////
  default:
    SEQ_LCD_PrintSpaces(21);
  }
  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////


  // for cheap enabled/disabled
  if( enabled_value >= 0 ) {
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(21);
    } else {
      SEQ_LCD_PrintString(enabled_value ? "enabled" : "disabled");
    }
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
s32 SEQ_UI_OPT_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

  return 0; // no error
}
