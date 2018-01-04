// $Id$
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
#include "seq_midi_in.h"
#include "seq_midi_port.h"
#include "seq_midi_sysex.h"
#include "seq_mixer.h"
#include "seq_tpd.h"
#include "seq_blm.h"
#include "seq_lcd_logo.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define ITEM_STEPS_MEASURE                 0
#define ITEM_STEPS_PATTERN                 1
#define ITEM_SYNC_CHANGE                   2
#define ITEM_PATTERN_RESEND_PC             3
#define ITEM_RATOPC                        4
#define ITEM_PATTERN_MIXER_MAP_COUPLING    5
#define ITEM_SYNC_MUTE                     6
#define ITEM_SYNC_UNMUTE                   7
#define ITEM_PASTE_CLR_ALL                 8
#define ITEM_RESTORE_TRACK_SELECTIONS      9
#define ITEM_MODIFY_PATTERN_BANKS         10
#define ITEM_METRONOME                    11
#define ITEM_SHADOW_OUT                   12
#define ITEM_MIDI_REMOTE                  13
#define ITEM_TRACK_CC                     14
#define ITEM_RUNTIME_STATUS_OPTIMIZATION  15
#define ITEM_LIVE_LAYER_MUTE              16
#define ITEM_INIT_WITH_TRIGGERS           17
#define ITEM_INIT_CC                      18
#define ITEM_DRUM_CC                      19
#define ITEM_TPD_MODE                     20
#define ITEM_BLM_ALWAYS_USE_FTS           21
#define ITEM_BLM_FADERS                   22
#define ITEM_MIXER_CC1234                 23
#define ITEM_MENU_SHORTCUTS               24
#define ITEM_SCREEN_SAVER                 25

#define NUM_OF_ITEMS                      26


static const char *item_text[NUM_OF_ITEMS][2] = {
  
  {//<-------------------------------------->
    "Track Synchronisation",
    "Steps per Measure:" // %3d
  },

  {//<-------------------------------------->
    "Pattern Change Synchronisation",
    "Change considered each " // %3d steps
  },

  {//<-------------------------------------->
    "Pattern Change Synchronisation",
    NULL, // enabled/disabled
  },

  {//<-------------------------------------->
    "Pattern Change re-sends Program Change",
    NULL, // enabled/disabled
  },

  {//<-------------------------------------->
    "Restart all Tracks on Pattern Change",
    NULL, // enabled/disabled
  },

  {//<-------------------------------------->
    "Dump a predefined Mixer Map on",
    "Pattern Changes: ", // enabled/disabled
  },

  {//<-------------------------------------->
    "Synchronize MUTE to Measure",
    NULL, // enabled/disabled
  },

  {//<-------------------------------------->
    "Synchronize UNMUTE to Measure",
    NULL, // enabled/disabled
  },

  {//<-------------------------------------->
    "Paste and Clear button will modify",
    NULL, // Only Steps/Complete Track
  },

  {//<-------------------------------------->
    "Selections restored on Track Change",
    NULL, // enabled/disabled
  },

  {//<-------------------------------------->
    "Allow to change pattern banks",
    NULL, // enabled/disabled
  },

  {//<-------------------------------------->
    "Metronome Port Chn. Meas.Note  BeatNote",
    NULL,
  },

  {//<-------------------------------------->
    "ShadowOut Port Chn.",
    NULL,
  },

  {//<-------------------------------------->
    "Frontpanel MIDI Remote Control",
    "via: ",
  },

  {//<-------------------------------------->
    "Sending selected Track as CC to a DAW",
    NULL,
  },

  {//<-------------------------------------->
    "MIDI OUT Runtime Status Optimization",
    NULL,
  },

  {//<-------------------------------------->
    "If Live function, matching received",
    "MIDI Events will: ",
  },

  {//<-------------------------------------->
    "Initial Gate Trigger Layer Pattern:",
    NULL,
  },

  {//<-------------------------------------->
    "Initial CC value for Clear and Init",
    "is: ",
  },

  {//<-------------------------------------->
    "Session specific Drum CC Assignments",
    "",
  },

  {//<-------------------------------------->
    "Track Position Display (TPD) Mode",
    NULL,
  },

  {//<-------------------------------------->
    "The BLM16x16+X should always use",
    "Force-To-Scale in Grid Edit Mode: " // yes/no
  },

  {//<-------------------------------------->
    "BLM16x16+X Fader Assignments",
    NULL,
  },

  {//<-------------------------------------->
    "Mixer CCs which should be sent after PC",
    NULL,
  },

  {//<-------------------------------------->
    "MENU button page assignments",
    "GP Button #",
  },

  {//<-------------------------------------->
    "Screen Saver:",
    ""
  },
};

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 local_selected_item = 0; // stays stable when menu is exit

static u8 selected_blm_fader = 0;
static u8 selected_menu_assignment_button = 0;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  *gp_leds = 0xffc0;

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
  if( encoder <= SEQ_UI_ENCODER_GP6 )
    return -1; // not mapped

  // change page
  if( encoder == SEQ_UI_ENCODER_GP7 || encoder == SEQ_UI_ENCODER_GP8 ) {
    if( SEQ_UI_Var8_Inc(&local_selected_item, 0, NUM_OF_ITEMS-1, incrementer) >= 0 ) {
      return 1; // changed
    }
    return 0; // not changed
  }

  // for GP encoders and Datawheel
  switch( local_selected_item ) {
    case ITEM_STEPS_MEASURE:
      if( encoder == SEQ_UI_ENCODER_GP16 ) {
	// increment in +/- 16 steps
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
      if( encoder == SEQ_UI_ENCODER_GP16 ) {
	// increment in +/- 16 steps
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
	seq_core_options.SYNCHED_PATTERN_CHANGE = (incrementer > 0) ? 1 : 0;
      else
	seq_core_options.SYNCHED_PATTERN_CHANGE ^= 1;
      ui_store_file_required = 1;
      return 1;

    case ITEM_PATTERN_RESEND_PC:
      if( incrementer )
	seq_core_options.PATTERN_CHANGE_DONT_RESET_LATCHED_PC = (incrementer > 0) ? 1 : 0;
      else
	seq_core_options.PATTERN_CHANGE_DONT_RESET_LATCHED_PC ^= 1;
      ui_store_file_required = 1;
      return 1;

    case ITEM_PATTERN_MIXER_MAP_COUPLING:
      if( incrementer )
	seq_core_options.PATTERN_MIXER_MAP_COUPLING = (incrementer > 0) ? 1 : 0;
      else
	seq_core_options.PATTERN_MIXER_MAP_COUPLING ^= 1;
      ui_store_file_required = 1;
      return 1;

    case ITEM_PASTE_CLR_ALL:
      if( incrementer )
	seq_core_options.PASTE_CLR_ALL = (incrementer > 0) ? 1 : 0;
      else
	seq_core_options.PASTE_CLR_ALL ^= 1;
      ui_store_file_required = 1;
      return 1;

    case ITEM_RATOPC:
      if( incrementer )
	seq_core_options.RATOPC = (incrementer > 0) ? 1 : 0;
      else
	seq_core_options.RATOPC ^= 1;
      ui_store_file_required = 1;
      return 1;

    case ITEM_SYNC_MUTE: {
      if( incrementer )
	seq_core_options.SYNCHED_MUTE = (incrementer > 0) ? 1 : 0;
      else
	seq_core_options.SYNCHED_MUTE ^= 1;
      ui_store_file_required = 1;
      return 1;
    } break;

    case ITEM_SYNC_UNMUTE: {
      if( incrementer )
	seq_core_options.SYNCHED_UNMUTE = (incrementer > 0) ? 1 : 0;
      else
	seq_core_options.SYNCHED_UNMUTE ^= 1;
      ui_store_file_required = 1;
      return 1;
    } break;

    case ITEM_RESTORE_TRACK_SELECTIONS: {
      if( incrementer )
	seq_ui_options.RESTORE_TRACK_SELECTIONS = (incrementer > 0) ? 1 : 0;
      else
	seq_ui_options.RESTORE_TRACK_SELECTIONS ^= 1;
      ui_store_file_required = 1;
      return 1;
    } break;

    case ITEM_MODIFY_PATTERN_BANKS: {
      if( incrementer )
	seq_ui_options.MODIFY_PATTERN_BANKS = (incrementer > 0) ? 1 : 0;
      else
	seq_ui_options.MODIFY_PATTERN_BANKS ^= 1;
      ui_store_file_required = 1;
      return 1;
    } break;

    case ITEM_INIT_WITH_TRIGGERS: {
      if( incrementer )
	seq_core_options.INIT_WITH_TRIGGERS = (incrementer > 0) ? 1 : 0;
      else
	seq_core_options.INIT_WITH_TRIGGERS ^= 1;
      ui_store_file_required = 1;
      return 1;
    } break;

    case ITEM_INIT_CC: {
      u8 value = seq_core_options.INIT_CC;
      if( SEQ_UI_Var8_Inc(&value, 0, 128, incrementer) >= 0 ) {
	seq_core_options.INIT_CC = value;
	ui_store_file_required = 1;
	return 1;
      }
      return 0;
    } break;

    case ITEM_DRUM_CC: {
#ifdef MBSEQV4P
      switch( encoder ) {
      case SEQ_UI_ENCODER_GP9:
      case SEQ_UI_ENCODER_GP10:
      case SEQ_UI_ENCODER_GP11:
      case SEQ_UI_ENCODER_GP12: {
	if( SEQ_UI_Var8_Inc(&ui_selected_instrument, 0, 15, incrementer) >= 0 ) {
	  return 1; // value changed
	}
	return 0; // no change
      } break;

      case SEQ_UI_ENCODER_GP13:
      case SEQ_UI_ENCODER_GP14: {
	if( ui_selected_par_layer >= 4 ) {
	  ui_selected_par_layer = 3; // max. 4 layers supported
	  return 1;
	}

	if( SEQ_UI_Var8_Inc(&ui_selected_par_layer, 0, 3, incrementer) >= 0 ) {
	  return 1; // value changed
	}
	return 0; // no change
      } break;

      case SEQ_UI_ENCODER_GP15:
      case SEQ_UI_ENCODER_GP16: {
	if( SEQ_UI_Var8_Inc(&seq_layer_drum_cc[ui_selected_instrument][ui_selected_par_layer], 0, 128, incrementer) >= 0 ) {
	  ui_store_file_required = 1;
	  return 1; // value changed
	}
	return 0; // no change
      } break;
      }
#endif

      return -1;
    } break;

    case ITEM_METRONOME: {
      switch( encoder ) {
      case SEQ_UI_ENCODER_GP9:
      case SEQ_UI_ENCODER_GP10: {
	if( incrementer )
	  seq_core_state.METRONOME = ((incrementer > 0)) ? 1 : 0;
	else
	  seq_core_state.METRONOME ^= 1;
	ui_store_file_required = 1;
	return 1;
      } break;

      case SEQ_UI_ENCODER_GP11: {
	u8 port_ix = SEQ_MIDI_PORT_OutIxGet(seq_core_metronome_port);
	if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	  seq_core_metronome_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	  ui_store_file_required = 1;
	  return 1; // value changed
	}
	return 0; // no change
      } break;

      case SEQ_UI_ENCODER_GP12: {
	if( SEQ_UI_Var8_Inc(&seq_core_metronome_chn, 0, 16, incrementer) >= 0 ) {
	  ui_store_file_required = 1;
	  return 1; // value changed
	}
	return 0; // no change
      } break;

      case SEQ_UI_ENCODER_GP13:
      case SEQ_UI_ENCODER_GP14: {
	if( SEQ_UI_Var8_Inc(&seq_core_metronome_note_m, 0, 127, incrementer) >= 0 ) {
	  ui_store_file_required = 1;
	  return 1; // value changed
	}
	return 0; // no change
      } break;

      case SEQ_UI_ENCODER_GP15:
      case SEQ_UI_ENCODER_GP16: {
	if( SEQ_UI_Var8_Inc(&seq_core_metronome_note_b, 0, 127, incrementer) >= 0 ) {
	  ui_store_file_required = 1;
	  return 1; // value changed
	}
	return 0; // no change
      } break;
      }

      return -1;
    } break;

    case ITEM_SHADOW_OUT: {
      switch( encoder ) {
      case SEQ_UI_ENCODER_GP11: {
	u8 port_ix = SEQ_MIDI_PORT_OutIxGet(seq_core_shadow_out_port);
	if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	  seq_core_shadow_out_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	  ui_store_file_required = 1;
	  return 1; // value changed
	}
	return 0; // no change
      } break;

      case SEQ_UI_ENCODER_GP12: {
	if( SEQ_UI_Var8_Inc(&seq_core_shadow_out_chn, 0, 16, incrementer) >= 0 ) {
	  ui_store_file_required = 1;
	  return 1; // value changed
	}
	return 0; // no change
      } break;
      }

      return -1;
    } break;

    case ITEM_MIDI_REMOTE: {
      if( encoder == SEQ_UI_ENCODER_GP10 ) {
	if( incrementer )
	  seq_midi_in_remote.cc_or_key = ((incrementer > 0)) ? 1 : 0;
	else
	  seq_midi_in_remote.cc_or_key ^= 1;
	ui_store_file_required = 1;
	return 1;
      } else if( encoder == SEQ_UI_ENCODER_GP11 ) {
	u8 value = seq_midi_in_remote.value;
	if( SEQ_UI_Var8_Inc(&value, 0, 127, incrementer) >= 0 ) {
	  seq_midi_in_remote.value = value;
	  ui_store_file_required = 1;
	  return 1;
	}
      }

      return 0;
    } break;

    case ITEM_TRACK_CC: {
      switch( encoder ) {
      case SEQ_UI_ENCODER_GP9:
      case SEQ_UI_ENCODER_GP10:
      case SEQ_UI_ENCODER_GP11:
      case SEQ_UI_ENCODER_GP12: {
	u8 value = seq_ui_track_cc.mode;
	if( SEQ_UI_Var8_Inc(&value, 0, 2, incrementer) >= 0 ) {
	  seq_ui_track_cc.mode = value;
	  ui_store_file_required = 1;
	  return 1;
	}
	return 0;
      } break;
      case SEQ_UI_ENCODER_GP13:
      case SEQ_UI_ENCODER_GP14: {
	u8 port_ix = SEQ_MIDI_PORT_OutIxGet(seq_ui_track_cc.port);
	if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	  seq_ui_track_cc.port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	  ui_store_file_required = 1;
	  return 1;
	}
	return 0;
      } break;
      case SEQ_UI_ENCODER_GP15: {
	u8 value = seq_ui_track_cc.chn;
	if( SEQ_UI_Var8_Inc(&value, 0, 15, incrementer) >= 0 ) {
	  seq_ui_track_cc.chn = value;
	  ui_store_file_required = 1;
	  return 1;
	}
	return 0;
      } break;
      case SEQ_UI_ENCODER_GP16: {
	u8 value = seq_ui_track_cc.cc;
	if( SEQ_UI_Var8_Inc(&value, 0, 127, incrementer) >= 0 ) {
	  seq_ui_track_cc.cc = value;
	  ui_store_file_required = 1;
	  return 1;
	}
	return 0;
      } break;
      }

      return -1;
    } break;

    case ITEM_RUNTIME_STATUS_OPTIMIZATION: {
      mios32_midi_port_t port = DEFAULT;

      if( encoder == SEQ_UI_ENCODER_GP10 ) {
	port = UART0;
      } else if( encoder == SEQ_UI_ENCODER_GP12 ) {
	port = UART1;
      } else if( encoder == SEQ_UI_ENCODER_GP14 ) {
	port = UART2;
      } else if( encoder == SEQ_UI_ENCODER_GP16 ) {
	port = UART3;
      }

      if( port != DEFAULT ) {
	if( incrementer )
	  MIOS32_MIDI_RS_OptimisationSet(port, (incrementer > 0) ? 1 : 0);
	else
	  MIOS32_MIDI_RS_OptimisationSet(port, (MIOS32_MIDI_RS_OptimisationGet(port) > 0) ? 0 : 1);
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
	seq_blm_options.ALWAYS_USE_FTS = (incrementer > 0) ? 1 : 0;
      else
	seq_blm_options.ALWAYS_USE_FTS ^= 1;
      ui_store_file_required = 1;
      return 1;
    } break;

    case ITEM_BLM_FADERS: {
      seq_blm_fader_t *fader = &seq_blm_fader[selected_blm_fader];

      switch( encoder ) {
      case SEQ_UI_ENCODER_GP9: {
	if( SEQ_UI_Var8_Inc(&selected_blm_fader, 0, SEQ_BLM_NUM_FADERS-1, incrementer) >= 0 ) {
	  return 1;
	}
	return 0;
      } break;

      case SEQ_UI_ENCODER_GP10:
      case SEQ_UI_ENCODER_GP11: {
	u8 port_ix = SEQ_MIDI_PORT_OutIxGet(fader->port);
	if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	  fader->port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	  ui_store_file_required = 1;
	  return 1;
	}
	return 0;
      } break;

      case SEQ_UI_ENCODER_GP12:
      case SEQ_UI_ENCODER_GP13: {
	u8 value = fader->chn;
	if( SEQ_UI_Var8_Inc(&value, 0, 16, incrementer) >= 0 ) {
	  fader->chn = value;
	  ui_store_file_required = 1;
	  return 1;
	}
	return 0;
      } break;

      case SEQ_UI_ENCODER_GP14:
      case SEQ_UI_ENCODER_GP16:
      case SEQ_UI_ENCODER_GP15: {
	u16 value = fader->send_function;
	if( SEQ_UI_Var16_Inc(&value, 0, 256, incrementer) >= 0 ) {
	  fader->send_function = value;
	  ui_store_file_required = 1;
	  return 1;
	}
	return 0;
      } break;
      }
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

    case ITEM_MENU_SHORTCUTS: {
      switch( encoder ) {
      case SEQ_UI_ENCODER_GP9:
      case SEQ_UI_ENCODER_GP10:
      case SEQ_UI_ENCODER_GP11: {
	if( SEQ_UI_Var8_Inc(&selected_menu_assignment_button, 0, 15, incrementer) >= 0 ) {
	  return 1;
	}
	return 0;
      } break;
      case SEQ_UI_ENCODER_GP12:
      case SEQ_UI_ENCODER_GP13:
      case SEQ_UI_ENCODER_GP14:
      case SEQ_UI_ENCODER_GP15:
      case SEQ_UI_ENCODER_GP16: {
	u8 value = (u8)SEQ_UI_PAGES_MenuShortcutPageGet(selected_menu_assignment_button);
	if( SEQ_UI_Var8_Inc(&value, 0, SEQ_UI_PAGES-1, incrementer) >= 0 ) {
	  SEQ_UI_PAGES_MenuShortcutPageSet(selected_menu_assignment_button, value);
	  ui_store_file_required = 1;
	  return 1;
	}
	return 0;
      } break;
      }
      return -1;
    } break;

    case ITEM_SCREEN_SAVER: {
      if( incrementer ) {
	int delay = (int)seq_lcd_logo_screensaver_delay + incrementer;
	if( delay > 255 )
	  delay = 255;
	else if( delay < 0 )
	  delay = 0;

	seq_lcd_logo_screensaver_delay = delay;
	ui_store_file_required = 1;
      }
      return 1;
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
  SEQ_LCD_PrintSpaces(32);
  SEQ_LCD_PrintString("Option  ");
  SEQ_LCD_PrintStringPadded((char *)item_text[local_selected_item][0], 40);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);
  SEQ_LCD_PrintSpaces(33);
  SEQ_LCD_PrintFormattedString("%2d/%-2d  ", local_selected_item+1, NUM_OF_ITEMS);

  ///////////////////////////////////////////////////////////////////////////
  char *str = (char *)item_text[local_selected_item][1];
  u32 len = strlen(str);
  int enabled_value = -1; // cheap: will print enabled/disabled in second line if >= 0

  switch( local_selected_item ) {

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_STEPS_MEASURE: {
    SEQ_LCD_PrintString(str);
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      SEQ_LCD_PrintFormattedString("%3d", (int)seq_core_steps_per_measure + 1);
    }
    SEQ_LCD_PrintSpaces(40-3-len);
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_STEPS_PATTERN: {
    SEQ_LCD_PrintString(str);
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(9);
    } else {
      SEQ_LCD_PrintFormattedString("%3d steps", (int)seq_core_steps_per_pattern + 1);
    }
    SEQ_LCD_PrintSpaces(40-9-len);
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_SYNC_CHANGE: {
    enabled_value = seq_core_options.SYNCHED_PATTERN_CHANGE;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_PATTERN_RESEND_PC: {
    enabled_value = seq_core_options.PATTERN_CHANGE_DONT_RESET_LATCHED_PC ? 0 : 1;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_RATOPC: {
    enabled_value = seq_core_options.RATOPC;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_PATTERN_MIXER_MAP_COUPLING: {
    SEQ_LCD_PrintString(str);

    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(40-len);
    } else {
      SEQ_LCD_PrintStringPadded(seq_core_options.PATTERN_MIXER_MAP_COUPLING ? "enabled" : "disabled", 40-len);
    }
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_SYNC_MUTE: {
    enabled_value = seq_core_options.SYNCHED_MUTE;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_SYNC_UNMUTE: {
    enabled_value = seq_core_options.SYNCHED_UNMUTE;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_PASTE_CLR_ALL: {
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(14);
    } else {
      SEQ_LCD_PrintString(seq_core_options.PASTE_CLR_ALL ? "Complete Track" : "Only Steps    ");
    }
    SEQ_LCD_PrintSpaces(40-14);
  } break;


  ///////////////////////////////////////////////////////////////////////////
  case ITEM_METRONOME: {
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(40);
    } else {
      SEQ_LCD_PrintSpaces(3);
      SEQ_LCD_PrintString(seq_core_state.METRONOME ? "on " : "off");
      SEQ_LCD_PrintSpaces(4);

      SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(seq_core_metronome_port)));
      SEQ_LCD_PrintSpaces(1);

      if( !seq_core_metronome_chn )
	SEQ_LCD_PrintString("---");
      else
	SEQ_LCD_PrintFormattedString("#%2d", seq_core_metronome_chn);
      SEQ_LCD_PrintSpaces(5);

      SEQ_LCD_PrintNote(seq_core_metronome_note_m);
      SEQ_LCD_PrintSpaces(7);

      SEQ_LCD_PrintNote(seq_core_metronome_note_b);
      SEQ_LCD_PrintSpaces(4);
    }
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_SHADOW_OUT: {
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(40);
    } else {
      SEQ_LCD_PrintSpaces(10);

      SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(seq_core_shadow_out_port)));
      SEQ_LCD_PrintSpaces(1);

      if( !seq_core_shadow_out_chn )
	SEQ_LCD_PrintString("---");
      else
	SEQ_LCD_PrintFormattedString("#%2d", seq_core_shadow_out_chn);
      SEQ_LCD_PrintSpaces(25);
    }
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_MIDI_REMOTE: {
    SEQ_LCD_PrintString(str);

    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(8);
    } else {
      if( seq_midi_in_remote.cc_or_key ) {
	SEQ_LCD_PrintFormattedString("CC   #%3d", seq_midi_in_remote.value);
      } else {
	SEQ_LCD_PrintString("Key   ");
	SEQ_LCD_PrintNote(seq_midi_in_remote.value);
      }

    }
    SEQ_LCD_PrintSpaces(40-8-5);
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_TRACK_CC: {
    u8 valid = 0;
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(40);
    } else {
      if( seq_ui_track_cc.mode == 1 ) {
	SEQ_LCD_PrintString("Single CC with Track: ");
	valid = 1;
      } else if( seq_ui_track_cc.mode == 2 ) {
	SEQ_LCD_PrintString("CC..CC+15 per Track:  ");
	valid = 1;
      } else {
	SEQ_LCD_PrintString("Function disabled     ");
      }

      if( !valid ) {
	SEQ_LCD_PrintSpaces(40-22);
      } else {
	SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(seq_ui_track_cc.port)));
	SEQ_LCD_PrintFormattedString(" Chn#%2d CC#%3d", seq_ui_track_cc.chn+1, seq_ui_track_cc.cc);
      }
    }
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_RUNTIME_STATUS_OPTIMIZATION: {
    int i;

    for(i=0; i<4; ++i) {
      SEQ_LCD_PrintFormattedString("OUT%d: ", i+1);
      if( ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	s32 enabled = MIOS32_MIDI_RS_OptimisationGet(UART0 + i);
	if( enabled < 0 ) {
	  SEQ_LCD_PrintString("--- ");
	} else {
	  SEQ_LCD_PrintString(enabled >= 1 ? "on  " : "off ");
	}
      }
    }
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_RESTORE_TRACK_SELECTIONS: {
    enabled_value = seq_ui_options.RESTORE_TRACK_SELECTIONS;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_MODIFY_PATTERN_BANKS: {
    enabled_value = seq_ui_options.MODIFY_PATTERN_BANKS;
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_INIT_WITH_TRIGGERS: {
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(25);
    } else {
      SEQ_LCD_PrintString(seq_core_options.INIT_WITH_TRIGGERS ? "Set gate on each 4th step" : "Empty                    ");
    }
    SEQ_LCD_PrintSpaces(40-25);
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_INIT_CC: {
    SEQ_LCD_PrintString(str);
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      if( seq_core_options.INIT_CC >= 0x80 ) {
	SEQ_LCD_PrintString("---");
      } else {
	SEQ_LCD_PrintFormattedString("%3d", seq_core_options.INIT_CC);
      }
    }
    SEQ_LCD_PrintSpaces(40-3-len);
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_DRUM_CC: {
#ifndef MBSEQV4P
    SEQ_LCD_PrintStringPadded("Only supported by V4+ Firmware!", 40);
#else
    SEQ_LCD_PrintString("Drum #");
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(2);
    } else {
      SEQ_LCD_PrintFormattedString("%2d", ui_selected_instrument+1);
    }

    u8 visible_track = SEQ_UI_VisibleTrackGet();
    seq_cc_trk_t *tcc = &seq_cc_trk[visible_track];
    if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
      SEQ_LCD_PrintString(" (");
      if( ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	SEQ_LCD_PrintTrackDrum(visible_track, ui_selected_instrument, (char *)seq_core_trk[visible_track].name);
      }
      SEQ_LCD_PrintString(")");
    } else {
      SEQ_LCD_PrintSpaces(8);
    }

    SEQ_LCD_PrintString("    PLayer:");
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintChar(' ');
    } else {
      SEQ_LCD_PrintChar('A' + ui_selected_par_layer);
    }

    SEQ_LCD_PrintString("  CC:#");
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      u8 cc = seq_layer_drum_cc[ui_selected_instrument][ui_selected_par_layer];
      if( cc >= 128 ) {
	SEQ_LCD_PrintString("---");
      } else {
	SEQ_LCD_PrintFormattedString("%3d", cc);
      }
    }

    SEQ_LCD_PrintSpaces(3);
#endif
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_LIVE_LAYER_MUTE: {
    SEQ_LCD_PrintString(str);

    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(22);
    } else {
      if( seq_core_options.LIVE_LAYER_MUTE_STEPS == 0 ) {
	SEQ_LCD_PrintStringPadded("do nothing", 22);
      } else if( seq_core_options.LIVE_LAYER_MUTE_STEPS == 1 ) {
	SEQ_LCD_PrintStringPadded("mute the appr. layer", 22);
      } else if( seq_core_options.LIVE_LAYER_MUTE_STEPS == 2 ) {
	SEQ_LCD_PrintStringPadded("mute layer for 1 step ", 22);
      } else {
	SEQ_LCD_PrintFormattedString("mute layer for %d steps", seq_core_options.LIVE_LAYER_MUTE_STEPS-1);
      }
    }
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_TPD_MODE: {
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(40);
    } else {
      const char *tpd_mode_str[SEQ_TPD_NUM_MODES] = {
       //<-------------------------------------->
	"Green LED: Pos      Red LED: Track",
	"Green LED: Pos      Red LED: Track (Rot)",
	"Green LED: Meter    Red LED: Pos",
	"Green LED: Meter    Red LED: Pos   (Rot)",
	"Green LED: DotMeter Red LED: Pos",
	"Green LED: DotMeter Red LED: Pos   (Rot)",
      };

      u8 mode = SEQ_TPD_ModeGet();
      if( mode >= SEQ_TPD_NUM_MODES )
	mode = 0;

      SEQ_LCD_PrintStringPadded((char *)tpd_mode_str[mode], 40);
    }
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_BLM_ALWAYS_USE_FTS: {
    SEQ_LCD_PrintString(str);

    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(40-len);
    } else {
      SEQ_LCD_PrintStringPadded(seq_blm_options.ALWAYS_USE_FTS ? "yes" : "no", 40-len);
    }
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_BLM_FADERS: {
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(40);
    } else {
      seq_blm_fader_t *fader = &seq_blm_fader[selected_blm_fader];

      SEQ_LCD_PrintFormattedString("Fader:%d ", selected_blm_fader+1);

      SEQ_LCD_PrintString("Port:");
      if( fader->port == DEFAULT ) {
	SEQ_LCD_PrintString("Trk ");
      } else {
	SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(fader->port)));
      }

      SEQ_LCD_PrintString(" Chn:");
      if( fader->chn == 0 ) {
	SEQ_LCD_PrintString("Trk");
      } else {
	SEQ_LCD_PrintFormattedString("%2d ", fader->chn);
      }
      SEQ_LCD_PrintString(" Send:");

      if( fader->send_function < 128 ) {
	SEQ_LCD_PrintFormattedString("CC#%3d   ", fader->send_function);
      } else if( fader->send_function < 256 ) {
	SEQ_LCD_PrintFormattedString("CC#%3dInv", fader->send_function - 128);
      } else {
	SEQ_LCD_PrintFormattedString("TODO#%3d ", fader->send_function & 0x7f);
      }
    }
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_MIXER_CC1234: {
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(40);
    } else {
      SEQ_LCD_PrintFormattedString(" CC1:%s   CC2:%s   CC3:%s   CC4:%s  ",
				   (seq_mixer_cc1234_before_pc & 0x1) ? " no" : "yes",
				   (seq_mixer_cc1234_before_pc & 0x2) ? " no" : "yes",
				   (seq_mixer_cc1234_before_pc & 0x4) ? " no" : "yes",
				   (seq_mixer_cc1234_before_pc & 0x8) ? " no" : "yes");
    }
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_MENU_SHORTCUTS: {
    SEQ_LCD_PrintString(str);

    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(4);
    } else {
      SEQ_LCD_PrintFormattedString("%2d  ", selected_menu_assignment_button+1);
    }

    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(25);
    } else {
      seq_ui_page_t page = SEQ_UI_PAGES_MenuShortcutPageGet(selected_menu_assignment_button);
      SEQ_LCD_PrintString(SEQ_UI_PAGES_MenuShortcutNameGet(selected_menu_assignment_button));
      SEQ_LCD_PrintString(": ");
      SEQ_LCD_PrintStringPadded((char *)SEQ_UI_PAGES_PageNameGet(page), 18);
    }

  } break;

  ///////////////////////////////////////////////////////////////////////////
  case ITEM_SCREEN_SAVER: {
    if( !seq_lcd_logo_screensaver_delay ) {
      if( ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	SEQ_LCD_PrintString("off");
      }
      SEQ_LCD_PrintSpaces(40-3);
    } else {
      int delay_len;
      if( seq_lcd_logo_screensaver_delay < 10 )
	delay_len = 1;
      else if( seq_lcd_logo_screensaver_delay < 100 )
	delay_len = 2;
      else
	delay_len = 3;

      SEQ_LCD_PrintString("after ");
      if( ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(delay_len);
      } else {
	SEQ_LCD_PrintFormattedString("%d", seq_lcd_logo_screensaver_delay);
      }
      SEQ_LCD_PrintString(" minutes");
      SEQ_LCD_PrintSpaces(40-6-delay_len-8);
    }
  } break;

  ///////////////////////////////////////////////////////////////////////////
  default:
    SEQ_LCD_PrintSpaces(40);
  }
  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////


  // for cheap enabled/disabled
  if( enabled_value >= 0 ) {
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(40);
    } else {
      SEQ_LCD_PrintStringPadded(enabled_value ? "enabled" : "disabled", 40);
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
