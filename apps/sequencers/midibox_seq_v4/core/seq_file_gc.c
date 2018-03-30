// $Id$
/*
 * Global Config File access functions
 *
 * NOTE: before accessing the SD Card, the upper level function should
 * synchronize with the SD Card semaphore!
 *   MUTEX_SDCARD_TAKE; // to take the semaphore
 *   MUTEX_SDCARD_GIVE; // to release the semaphore
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

#include <string.h>

#include <osc_client.h>
#include <blm_scalar_master.h>

#include "file.h"
#include "seq_file.h"
#include "seq_file_gc.h"
#include "seq_file_b.h"


#include "seq_ui.h"
#include "seq_bpm.h"
#include "seq_song.h"
#include "seq_mixer.h"
#include "seq_midi_in.h"
#include "seq_midi_port.h"
#include "seq_midi_router.h"
#include "seq_midi_sysex.h"
#include "seq_pattern.h"
#include "seq_record.h"
#include "seq_core.h"
#include "seq_cv.h"
#include "seq_blm.h"
#include "seq_tpd.h"
#include "seq_lcd_logo.h"

#if !defined(MIOS32_FAMILY_EMULATION)
#include "uip.h"
#include "uip_task.h"
#include "osc_server.h"
#endif


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the MBSEQ files located?
// use "/" for root
// use "/<dir>/" for a subdirectory in root
// use "/<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define SEQ_FILES_PATH "/"
//#define SEQ_FILES_PATH "/MySongs/"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// file informations stored in RAM
typedef struct {
  unsigned valid: 1;   // file is accessible
} seq_file_gc_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_file_gc_info_t seq_file_gc_info;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_GC_Init(u32 mode)
{
  // invalidate file info
  SEQ_FILE_GC_Unload();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads global config file
// Called from SEQ_FILE_GCheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_GC_Load(void)
{
  s32 error;
  error = SEQ_FILE_GC_Read();
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_GC] Tried to open global config file, status: %d\n", error);
#endif

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads global config file
// Called from SEQ_FILE_GCheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_GC_Unload(void)
{
  seq_file_gc_info.valid = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Returns 1 if global config file valid
// Returns 0 if global config file not valid
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_GC_Valid(void)
{
  return seq_file_gc_info.valid;
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1;

  return l; // value is valid
}

/////////////////////////////////////////////////////////////////////////////
// help function which parses an IP value
// returns > 0 if value is valid
// returns 0 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static u32 get_ip(char *brkt)
{
  u8 ip[4];
  char *word;

  int i;
  for(i=0; i<4; ++i) {
    if( (word=strtok_r(NULL, ".", &brkt)) ) {
      s32 value = get_dec(word);
      if( value >= 0 && value <= 255 )
	ip[i] = value;
      else
	return 0;
    }
  }

  if( i == 4 )
    return (ip[0]<<24)|(ip[1]<<16)|(ip[2]<<8)|(ip[3]<<0);
  else
    return 0; // invalid IP
}


/////////////////////////////////////////////////////////////////////////////
// reads the global config file content (again)
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_GC_Read(void)
{
  s32 status = 0;
  seq_file_gc_info_t *info = &seq_file_gc_info;
  file_t file;

  info->valid = 0; // will be set to valid if file content has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMBSEQ_GC.V4", SEQ_FILES_PATH);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_GC] Open global config file '%s'\n", filepath);
#endif

  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_GC] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read global config values
  char line_buffer[128];
  do {
    status=FILE_ReadLine((u8 *)line_buffer, 128);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[SEQ_FILE_GC] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t";
      char *brkt;
      char *parameter;

      if( (parameter = strtok_r(line_buffer, separators, &brkt)) ) {

	if( *parameter == '#' ) {
	  // ignore comments
#if !defined(MIOS32_FAMILY_EMULATION)
	} else if( strcmp(parameter, "ETH_LocalIp") == 0 ) {
	  u32 value;
	  if( !(value=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	  } else {
	    UIP_TASK_IP_AddressSet(value);
	  }
	} else if( strcmp(parameter, "ETH_Netmask") == 0 ) {
	  u32 value;
	  if( !(value=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	  } else {
	    UIP_TASK_NetmaskSet(value);
	  }
	} else if( strcmp(parameter, "ETH_Gateway") == 0 ) {
	  u32 value;
	  if( !(value=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	  } else {
	    UIP_TASK_GatewaySet(value);
	  }
#endif /* !defined(MIOS32_FAMILY_EMULATION) */
	} else {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 value = get_dec(word);

	  if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid value for parameter '%s'\n", parameter);
#endif
	  } else if( strcmp(parameter, "MetronomePort") == 0 ) {
	    seq_core_metronome_port = (mios32_midi_port_t)value;
	  } else if( strcmp(parameter, "MetronomeChannel") == 0 ) {
	    seq_core_metronome_chn = value;
	  } else if( strcmp(parameter, "MetronomeNoteM") == 0 ) {
	    seq_core_metronome_note_m = value;
	  } else if( strcmp(parameter, "MetronomeNoteB") == 0 ) {
	    seq_core_metronome_note_b = value;
	  } else if( strcmp(parameter, "ShadowOutPort") == 0 ) {
	    seq_core_shadow_out_port = (mios32_midi_port_t)value;
	  } else if( strcmp(parameter, "ShadowOutChannel") == 0 ) {
	    seq_core_shadow_out_chn = value;
	  } else if( strcmp(parameter, "MidiRemoteKey") == 0 ) {
	    seq_midi_in_remote.value = value;
	  } else if( strcmp(parameter, "MidiRemoteCCorKey") == 0 ) {
	    seq_midi_in_remote.cc_or_key = value;
#ifndef MBSEQV4L
	  } else if( strcmp(parameter, "TrackCCMode") == 0 ) {
	    seq_ui_track_cc.mode = value;
	  } else if( strcmp(parameter, "TrackCCPort") == 0 ) {
	    seq_ui_track_cc.port = (mios32_midi_port_t)value;
	  } else if( strcmp(parameter, "TrackCCChannel") == 0 ) {
	    seq_ui_track_cc.chn = value;
	  } else if( strcmp(parameter, "TrackCCNumber") == 0 ) {
	    seq_ui_track_cc.cc = value;
#endif
	  } else if( strcmp(parameter, "MidiOutRSOpt") == 0 ) {
	    mios32_midi_port_t port = (mios32_midi_port_t)value;
	    if( value < UART0 || value > UART3 ) {
	      DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid MIDI port 0x%02x (%u) for parameter '%s'\n", value, value, parameter);
	    } else {
	      word = strtok_r(NULL, separators, &brkt);
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid RS mode for parameter '%s'\n", parameter);
	      } else {
		MIOS32_MIDI_RS_OptimisationSet(port, value);
	      }
	    }
#ifndef MBSEQV4L
	  } else if( strcmp(parameter, "MenuShortcuts") == 0 ) {
	    int i;
	    for(i=0; i<16; ++i) {
	      u8 valid = 1;
	      if( i > 0 ) {
		word = strtok_r(NULL, separators, &brkt);
		if( (value=get_dec(word)) < 0 ) {
		  DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid menu page number for parameter '%s'\n", parameter);
		  valid = 0;
		}
	      }
	      if( valid && SEQ_UI_PAGES_MenuShortcutPageSet(i, (seq_ui_page_t)value) < 0 ) {
		DEBUG_MSG("[SEQ_FILE_GC] ERROR unsupported menu page number for parameter '%s'\n", parameter);
	      }
	    }

#endif
	  } else if( strcmp(parameter, "RecQuantisation") == 0 ) {
	    seq_record_quantize = value; // only for legacy reasons - quantisation moved to local configuration file seq_file_c.c
	  } else if( strcmp(parameter, "PasteClrAll") == 0 ) {
	    seq_core_options.PASTE_CLR_ALL = value;
#ifndef MBSEQV4L
	  } else if( strcmp(parameter, "DatawheelMode") == 0 ) {
	    seq_ui_edit_datawheel_mode = value;
#endif
	  } else if( strcmp(parameter, "MixerLiveSend") == 0 ) {
	    seq_core_options.MIXER_LIVE_SEND = value;
	  } else if( strcmp(parameter, "InitCC") == 0 ) {
	    seq_core_options.INIT_CC = value;
	  } else if( strcmp(parameter, "InitWithTriggers") == 0 ) {
	    seq_core_options.INIT_WITH_TRIGGERS = value;
	  } else if( strcmp(parameter, "LiveLayerMuteSteps") == 0 ) {
	    seq_core_options.LIVE_LAYER_MUTE_STEPS = value;
	  } else if( strcmp(parameter, "PatternMixerMapCoupling") == 0 ) {
	    seq_core_options.PATTERN_MIXER_MAP_COUPLING = value;
	  } else if( strcmp(parameter, "MultiPortEnableFlags") == 0 ) {
	    seq_midi_port_multi_enable_flags = value;
	  } else if( strcmp(parameter, "UiRestoreTrackSelections") == 0 ) {
#ifndef MBSEQV4L
	    seq_ui_options.RESTORE_TRACK_SELECTIONS = value;
#endif
	  } else if( strcmp(parameter, "UiModifyPatternBanks") == 0 ) {
#ifndef MBSEQV4L
	    seq_ui_options.MODIFY_PATTERN_BANKS = value;
#endif
	  } else if( strcmp(parameter, "UiPrintAndModifyWithoutGates") == 0 ) {
#ifndef MBSEQV4L
	    seq_ui_options.PRINT_AND_MODIFY_WITHOUT_GATES = value;
#endif
	  } else if( strcmp(parameter, "UiPrintTransposedNotes") == 0 ) {
#ifndef MBSEQV4L
	    seq_ui_options.PRINT_TRANSPOSED_NOTES = value;
#endif
	  } else if( strcmp(parameter, "RemoteMode") == 0 ) {
	    seq_midi_sysex_remote_mode = (value > 2) ? 0 : value;
	  } else if( strcmp(parameter, "RemotePort") == 0 ) {
	    seq_midi_sysex_remote_port = value;
	  } else if( strcmp(parameter, "RemoteID") == 0 ) {
	    seq_midi_sysex_remote_id = (value > 128) ? 0 : value;
#ifndef MBSEQV4L
	  } else if( strcmp(parameter, "ScreenSaverDelay") == 0 ) {
	    seq_lcd_logo_screensaver_delay = (value > 255) ? 255 : value;
#endif
	  } else if( strcmp(parameter, "CV_AOUT_Type") == 0 ) {
	    SEQ_CV_IfSet(value);
	  } else if( strcmp(parameter, "CV_PinMode") == 0 ) {
	    u32 cv = value;
	    if( cv >= SEQ_CV_NUM ) {
	      DEBUG_MSG("[SEQ_FILE_GC] ERROR wrong CV channel %u for parameter '%s'\n", value, parameter);
	    } else {
	      word = strtok_r(NULL, separators, &brkt);
	      u32 curve = get_dec(word);
	      if( curve >= SEQ_CV_NUM_CURVES ) {
		DEBUG_MSG("[SEQ_FILE_GC] ERROR wrong curve %u for parameter '%s', CV channel %u\n", curve, parameter, cv);
	      } else {
		word = strtok_r(NULL, separators, &brkt);
		u32 slewrate = get_dec(word);
		if( slewrate >= 256 ) // saturate
		  slewrate = 255;

		word = strtok_r(NULL, separators, &brkt);
		u32 range = get_dec(word);
		if( range >= 127 ) // saturate
		  range = 2; // default value

		SEQ_CV_CurveSet(cv, curve);
		SEQ_CV_SlewRateSet(cv, slewrate);
		SEQ_CV_PitchRangeSet(cv, range);
	      }
	    }
	  } else if( strcmp(parameter, "CV_GateInv") == 0 ) {
	    SEQ_CV_GateInversionAllSet(value);
	  } else if( strcmp(parameter, "CV_SusKey") == 0 ) {
	    SEQ_CV_SusKeyAllSet(value);
	  } else if( strcmp(parameter, "CV_ClkPulsewidth") == 0 ) {
	    SEQ_CV_ClkPulseWidthSet(0, value); // Legacy Value - replaced by CV_ExtClk
	  } else if( strcmp(parameter, "CV_ClkDivider") == 0 ) {
	    SEQ_CV_ClkDividerSet(0, value); // Legacy Value - replaced by CV_ExtClk
	  } else if( strcmp(parameter, "CV_ExtClk") == 0 ) {
	    u32 clkout = value;
	    if( clkout >= SEQ_CV_NUM_CLKOUT ) {
	      DEBUG_MSG("[SEQ_FILE_GC] ERROR wrong clock output %u for parameter '%s'\n", value, parameter);
	    } else {
	      word = strtok_r(NULL, separators, &brkt);
	      u32 divider = get_dec(word);
	      if( divider >= 65536 ) {
		DEBUG_MSG("[SEQ_FILE_GC] ERROR wrong divider value %u for parameter '%s', clock output %u\n", divider, parameter, clkout);
	      } else {
		word = strtok_r(NULL, separators, &brkt);
		u32 pulsewidth = get_dec(word);
		if( pulsewidth >= 256 ) // saturate
		  pulsewidth = 255;

		SEQ_CV_ClkDividerSet(clkout, divider);
		SEQ_CV_ClkPulseWidthSet(clkout, pulsewidth);
	      }
	    }
	  } else if( strcmp(parameter, "TpdMode") == 0 ) {
	    SEQ_TPD_ModeSet(value);
	  } else if( strcmp(parameter, "BLM_SCALAR_Port") == 0 ) {
	    BLM_SCALAR_MASTER_MIDI_PortSet(0, value);

	    BLM_SCALAR_MASTER_TimeoutCtrSet(0, 0); // fake timeout (so that "BLM not found" message will be displayed)
	    BLM_SCALAR_MASTER_SendRequest(0, 0x00); // request layout from BLM_SCALAR

#if !defined(MIOS32_FAMILY_EMULATION)
	  } else if( strcmp(parameter, "BLM_SCALAR_AlwaysUseFts") == 0 ) {
	    seq_blm_options.ALWAYS_USE_FTS = value;
	  } else if( strcmp(parameter, "ETH_Dhcp") == 0 ) {
	    UIP_TASK_DHCP_EnableSet((value >= 1) ? 1 : 0);
	  } else if( strcmp(parameter, "OSC_RemoteIp") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      u32 ip;
	      if( !(ip=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	      } else {
		OSC_SERVER_RemoteIP_Set(con, ip);
	      }
	    }
	  } else if( strcmp(parameter, "OSC_RemotePort") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      word = strtok_r(NULL, separators, &brkt);
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid port number for parameter '%s'\n", parameter);
	      } else {
		OSC_SERVER_RemotePortSet(con, value);
	      }
	    }
	  } else if( strcmp(parameter, "OSC_LocalPort") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      word = strtok_r(NULL, separators, &brkt);
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid port number for parameter '%s'\n", parameter);
	      } else {
		OSC_SERVER_LocalPortSet(con, value);
	      }
	    }
	  } else if( strcmp(parameter, "OSC_TransferMode") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      word = strtok_r(NULL, separators, &brkt);
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[SEQ_FILE_GC] ERROR invalid transfer mode number for parameter '%s'\n", parameter);
	      } else {
		OSC_CLIENT_TransferModeSet(con, value);
	      }
	    }
#endif
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	    // changed error level from 1 to 2 here, since people are sometimes confused about these messages
	    // on file format changes
	    DEBUG_MSG("[SEQ_FILE_GC] ERROR: unknown parameter: %s", line_buffer);
#endif
	  }
	}
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ_FILE_GC] ERROR no space separator in following line: %s", line_buffer);
#endif
      }
    }

  } while( status >= 1 );

  // close file
  status |= FILE_ReadClose(&file);

#if !defined(MIOS32_FAMILY_EMULATION)
  // OSC_SERVER_Init(0) has to be called after all settings have been done!
  OSC_SERVER_Init(0);
#endif

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_GC] ERROR while reading file, status: %d\n", status);
#endif
    return SEQ_FILE_GC_ERR_READ;
  }

  // file is valid! :)
  info->valid = 1;

  // change tempo to given preset
  SEQ_CORE_BPM_Update(seq_core_bpm_preset_tempo[seq_core_bpm_preset_num], seq_core_bpm_preset_ramp[seq_core_bpm_preset_num]);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function to write data into file or send to debug terminal
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_FILE_GC_Write_Hlp(u8 write_to_file)
{
  s32 status = 0;
  char line_buffer[128];

#define FLUSH_BUFFER if( !write_to_file ) { DEBUG_MSG(line_buffer); } else { status |= FILE_WriteBuffer((u8 *)line_buffer, strlen(line_buffer)); }

  // write global config values
  sprintf(line_buffer, "MetronomePort 0x%02x\n", (u8)seq_core_metronome_port);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MetronomeChannel %d\n", (u8)seq_core_metronome_chn);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MetronomeNoteM %d\n", (u8)seq_core_metronome_note_m);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MetronomeNoteB %d\n", (u8)seq_core_metronome_note_b);
  FLUSH_BUFFER;

  sprintf(line_buffer, "ShadowOutPort 0x%02x\n", (u8)seq_core_shadow_out_port);
  FLUSH_BUFFER;

  sprintf(line_buffer, "ShadowOutChannel %d\n", (u8)seq_core_shadow_out_chn);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MidiRemoteKey %d\n", (u8)seq_midi_in_remote.value);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MidiRemoteCCorKey %d\n", (u8)seq_midi_in_remote.cc_or_key);
  FLUSH_BUFFER;

#ifndef MBSEQV4L
  sprintf(line_buffer, "TrackCCMode %d\n", (u8)seq_ui_track_cc.mode);
  FLUSH_BUFFER;

  sprintf(line_buffer, "TrackCCPort 0x%02x\n", (u8)seq_ui_track_cc.port);
  FLUSH_BUFFER;

  sprintf(line_buffer, "TrackCCChannel %d\n", (u8)seq_ui_track_cc.chn);
  FLUSH_BUFFER;

  sprintf(line_buffer, "TrackCCNumber %d\n", (u8)seq_ui_track_cc.cc);
  FLUSH_BUFFER;
#endif

  {
    mios32_midi_port_t port;
    for(port=UART0; port<=UART3; ++port) {
      s32 enabled = MIOS32_MIDI_RS_OptimisationGet(port);
      if( enabled >= 0 ) {
	sprintf(line_buffer, "MidiOutRSOpt 0x%02x %d\n", (u8)port, enabled);
	FLUSH_BUFFER;
      }
    }
  }

#ifndef MBSEQV4L
  sprintf(line_buffer, "MenuShortcuts %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(0),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(1),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(2),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(3),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(4),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(5),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(6),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(7),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(8),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(9),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(10),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(11),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(12),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(13),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(14),
	  (int)SEQ_UI_PAGES_MenuShortcutPageGet(15)
	  );
  FLUSH_BUFFER;    
#endif

  sprintf(line_buffer, "PasteClrAll %d\n", seq_core_options.PASTE_CLR_ALL);
  FLUSH_BUFFER;

#ifndef MBSEQV4L
  sprintf(line_buffer, "DatawheelMode %d\n", seq_ui_edit_datawheel_mode);
  FLUSH_BUFFER;
#endif

  sprintf(line_buffer, "MixerLiveSend %d\n", seq_core_options.MIXER_LIVE_SEND);
  FLUSH_BUFFER;

  sprintf(line_buffer, "InitCC %d\n", seq_core_options.INIT_CC);
  FLUSH_BUFFER;

  sprintf(line_buffer, "InitWithTriggers %d\n", seq_core_options.INIT_WITH_TRIGGERS);
  FLUSH_BUFFER;

  sprintf(line_buffer, "LiveLayerMuteSteps %d\n", seq_core_options.LIVE_LAYER_MUTE_STEPS);
  FLUSH_BUFFER;

  sprintf(line_buffer, "PatternMixerMapCoupling %d\n", seq_core_options.PATTERN_MIXER_MAP_COUPLING);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MultiPortEnableFlags 0x%06x\n", seq_midi_port_multi_enable_flags);
  FLUSH_BUFFER;

#ifndef MBSEQV4L
  sprintf(line_buffer, "UiRestoreTrackSelections %d\n", seq_ui_options.RESTORE_TRACK_SELECTIONS);
  FLUSH_BUFFER;
#endif

#ifndef MBSEQV4L
  sprintf(line_buffer, "UiModifyPatternBanks %d\n", seq_ui_options.MODIFY_PATTERN_BANKS);
  FLUSH_BUFFER;
#endif

#ifndef MBSEQV4L
  sprintf(line_buffer, "UiPrintAndModifyWithoutGates %d\n", seq_ui_options.PRINT_AND_MODIFY_WITHOUT_GATES);
  FLUSH_BUFFER;
#endif

#ifndef MBSEQV4L
  sprintf(line_buffer, "UiPrintTransposedNotes %d\n", seq_ui_options.PRINT_TRANSPOSED_NOTES);
  FLUSH_BUFFER;
#endif

  sprintf(line_buffer, "RemoteMode %d\n", (u8)seq_midi_sysex_remote_mode);
  FLUSH_BUFFER;

  sprintf(line_buffer, "RemotePort 0x%02x\n", (u8)seq_midi_sysex_remote_port);
  FLUSH_BUFFER;

  sprintf(line_buffer, "RemoteID %d\n", (u8)seq_midi_sysex_remote_id);
  FLUSH_BUFFER;

  sprintf(line_buffer, "CV_AOUT_Type %d\n", (u8)SEQ_CV_IfGet());
  FLUSH_BUFFER;

  {
    int cv;
    for(cv=0; cv<SEQ_CV_NUM; ++cv) {
      sprintf(line_buffer, "CV_PinMode %d %d %d %d\n", cv, SEQ_CV_CurveGet(cv), (int)SEQ_CV_SlewRateGet(cv), (int)SEQ_CV_PitchRangeGet(cv));
      FLUSH_BUFFER;
    }
  }

  sprintf(line_buffer, "CV_GateInv 0x%02x\n", (u8)SEQ_CV_GateInversionAllGet());
  FLUSH_BUFFER;

  sprintf(line_buffer, "CV_SusKey 0x%02x\n", (u8)SEQ_CV_SusKeyAllGet());
  FLUSH_BUFFER;

  {
    int clkout;

    for(clkout=0; clkout<SEQ_CV_NUM_CLKOUT; ++clkout) {
      sprintf(line_buffer, "CV_ExtClk %d %d %d\n", clkout, SEQ_CV_ClkDividerGet(clkout), SEQ_CV_ClkPulseWidthGet(clkout));
      FLUSH_BUFFER;
    }
  }

  sprintf(line_buffer, "TpdMode %d\n", SEQ_TPD_ModeGet());
  FLUSH_BUFFER;

  sprintf(line_buffer, "BLM_SCALAR_Port 0x%02x\n", (u8)BLM_SCALAR_MASTER_MIDI_PortGet(0));
  FLUSH_BUFFER;

  sprintf(line_buffer, "BLM_SCALAR_AlwaysUseFts %d\n", (u8)seq_blm_options.ALWAYS_USE_FTS);
  FLUSH_BUFFER;

#if !defined(MIOS32_FAMILY_EMULATION)
  {
    u32 value = UIP_TASK_IP_AddressGet();
    sprintf(line_buffer, "ETH_LocalIp %d.%d.%d.%d\n",
	    (value >> 24) & 0xff,
	    (value >> 16) & 0xff,
	    (value >>  8) & 0xff,
	    (value >>  0) & 0xff);
    FLUSH_BUFFER;
  }

  {
    u32 value = UIP_TASK_NetmaskGet();
    sprintf(line_buffer, "ETH_Netmask %d.%d.%d.%d\n",
	    (value >> 24) & 0xff,
	    (value >> 16) & 0xff,
	    (value >>  8) & 0xff,
	    (value >>  0) & 0xff);
    FLUSH_BUFFER;
  }

  {
    u32 value = UIP_TASK_GatewayGet();
    sprintf(line_buffer, "ETH_Gateway %d.%d.%d.%d\n",
	    (value >> 24) & 0xff,
	    (value >> 16) & 0xff,
	    (value >>  8) & 0xff,
	    (value >>  0) & 0xff);
    FLUSH_BUFFER;
  }

  sprintf(line_buffer, "ETH_Dhcp %d\n", UIP_TASK_DHCP_EnableGet());
  FLUSH_BUFFER;

  int con;
  for(con=0; con<OSC_SERVER_NUM_CONNECTIONS; ++con) {
    u32 value = OSC_SERVER_RemoteIP_Get(con);
    sprintf(line_buffer, "OSC_RemoteIp %d %d.%d.%d.%d\n",
	    con,
	    (value >> 24) & 0xff,
	    (value >> 16) & 0xff,
	    (value >>  8) & 0xff,
	    (value >>  0) & 0xff);
    FLUSH_BUFFER;

    sprintf(line_buffer, "OSC_RemotePort %d %d\n", con, OSC_SERVER_RemotePortGet(con));
    FLUSH_BUFFER;

    sprintf(line_buffer, "OSC_LocalPort %d %d\n", con, OSC_SERVER_LocalPortGet(con));
    FLUSH_BUFFER;

    sprintf(line_buffer, "OSC_TransferMode %d %d\n", con, OSC_CLIENT_TransferModeGet(con));
    FLUSH_BUFFER;
  }
#endif

#ifndef MBSEQV4L
  sprintf(line_buffer, "ScreenSaverDelay %d\n", seq_lcd_logo_screensaver_delay);
  FLUSH_BUFFER;
#endif

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// writes data into global config file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_GC_Write(void)
{
  seq_file_gc_info_t *info = &seq_file_gc_info;

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMBSEQ_GC.V4", SEQ_FILES_PATH);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_GC] Open global config file '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_GC] Failed to open/create global config file, status: %d\n", status);
#endif
    FILE_WriteClose(); // important to free memory given by malloc
    info->valid = 0;
    return status;
  }

  // write file
  status |= SEQ_FILE_GC_Write_Hlp(1);

  // close file
  status |= FILE_WriteClose();


  // check if file is valid
  if( status >= 0 )
    info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_GC] global config file written with status %d\n", status);
#endif

  return (status < 0) ? SEQ_FILE_GC_ERR_WRITE : 0;

}

/////////////////////////////////////////////////////////////////////////////
// sends global config data to debug terminal
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_GC_Debug(void)
{
  return SEQ_FILE_GC_Write_Hlp(0); // send to debug terminal
}
