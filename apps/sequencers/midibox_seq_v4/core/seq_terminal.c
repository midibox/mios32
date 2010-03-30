// $Id$
/*
 * MIDIbox SEQ MIDI Terminal
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
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

#include <seq_midi_out.h>
#include <ff.h>

#include "tasks.h"

#include "app.h"
#include "seq_terminal.h"


#include "seq_core.h"
#include "seq_song.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_mixer.h"
#include "seq_midi_port.h"

#include "seq_file.h"
#include "seq_file_b.h"
#include "seq_file_s.h"
#include "seq_file_m.h"
#include "seq_file_g.h"
#include "seq_file_c.h"
#include "seq_file_hw.h"

#include "seq_statistics.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define STRING_MAX 80


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 line_buffer[STRING_MAX];
static u16 line_ix;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TERMINAL_Init(u32 mode)
{
  // install the callback function which is called on incoming characters
  // from MIOS Terminal
  MIOS32_MIDI_DebugCommandCallback_Init(SEQ_TERMINAL_Parse);

  // clear line buffer
  line_buffer[0] = 0;
  line_ix = 0;

  return 0; // no error
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
// Parser
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TERMINAL_Parse(mios32_midi_port_t port, u8 byte)
{
  // temporary change debug port (will be restored at the end of this function)
  mios32_midi_port_t prev_debug_port = MIOS32_MIDI_DebugPortGet();
  MIOS32_MIDI_DebugPortSet(port);

  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    // example for parsing the command:
    char *separators = " \t";
    char *brkt;
    char *parameter;

    if( parameter = strtok_r(line_buffer, separators, &brkt) ) {
      if( strncmp(parameter, "help", 4) == 0 ) {
	MUTEX_MIDIOUT_TAKE;
	DEBUG_MSG("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
	DEBUG_MSG("Following commands are available:");
	DEBUG_MSG("  system:         print system info\n");
	DEBUG_MSG("  global:         print global settings\n");
	DEBUG_MSG("  tracks:         print overview of all tracks\n");
	DEBUG_MSG("  track <track>:  print info about a specific track\n");
	DEBUG_MSG("  mixer:          print current mixer map\n");
	DEBUG_MSG("  song:           print current song info\n");
	DEBUG_MSG("  grooves:        print groove templates\n");
	DEBUG_MSG("  sdcard:         print SD Card info\n");
	DEBUG_MSG("  help:           this page\n");
	MUTEX_MIDIOUT_GIVE;
      } else if( strncmp(parameter, "system", 6) == 0 ) {
	SEQ_TERMINAL_PrintSystem();
      } else if( strncmp(parameter, "global", 6) == 0 ) {
	SEQ_TERMINAL_PrintGlobals();
      } else if( strncmp(parameter, "tracks", 6) == 0 ) {
	SEQ_TERMINAL_PrintTracks();
      } else if( strncmp(parameter, "track", 5) == 0 ) {
	  parameter += 5;
	  char *arg;
	  if( (arg = strtok_r(NULL, separators, &brkt)) ) {
	    int track = get_dec(arg);
	    if( track < 1 || track > SEQ_CORE_NUM_TRACKS ) {
	      MUTEX_MIDIOUT_TAKE;
	      DEBUG_MSG("Wrong track number %d - expected track 1..%d\n", track, SEQ_CORE_NUM_TRACKS);
	      MUTEX_MIDIOUT_GIVE;
	    } else {
	      SEQ_TERMINAL_PrintTrack(track-1);
	    }
	  } else {
	    MUTEX_MIDIOUT_TAKE;
	    DEBUG_MSG("Please specify track, e.g. \"track 1\"\n");
	    MUTEX_MIDIOUT_GIVE;
	  }
      } else if( strncmp(parameter, "mixer", 5) == 0 ) {
	SEQ_TERMINAL_PrintCurrentMixerMap();
      } else if( strncmp(parameter, "song", 6) == 0 ) {
	SEQ_TERMINAL_PrintCurrentSong();
      } else if( strncmp(parameter, "grooves", 7) == 0 ) {
	SEQ_TERMINAL_PrintGrooveTemplates();
      } else if( strncmp(parameter, "sdcard", 6) == 0 ) {
	SEQ_TERMINAL_PrintSdCardInfo();
      } else {
	MUTEX_MIDIOUT_TAKE;
	DEBUG_MSG("Unknown command - type 'help' to list available commands!\n");
	MUTEX_MIDIOUT_GIVE;
      }
    }

    line_ix = 0;

    MUTEX_MIDIOUT_GIVE;
  } else if( line_ix < (STRING_MAX-1) ) {
    line_buffer[line_ix++] = byte;
    line_buffer[line_ix] = 0;
  }

  // restore debug port
  MIOS32_MIDI_DebugPortSet(prev_debug_port);

  return 0; // no error
}


s32 SEQ_TERMINAL_PrintSystem(void)
{
  char str_buffer[128];

  MUTEX_MIDIOUT_TAKE;

  DEBUG_MSG("System Informations:\n");
  DEBUG_MSG("====================\n");
  DEBUG_MSG(MIOS32_LCD_BOOT_MSG_LINE1 " " MIOS32_LCD_BOOT_MSG_LINE2 "\n");

  mios32_sys_time_t t = MIOS32_SYS_TimeGet();
  int hours = (t.seconds / 3600) % 24;
  int minutes = (t.seconds % 3600) / 60;
  int seconds = (t.seconds % 3600) % 60;

  DEBUG_MSG("Operating System: MIOS32\n");
  DEBUG_MSG("Board: " MIOS32_BOARD_STR "\n");
  DEBUG_MSG("Chip Family: " MIOS32_FAMILY_STR "\n");
  if( MIOS32_SYS_SerialNumberGet((char *)str_buffer) >= 0 )
    DEBUG_MSG("Serial Number: %s\n", str_buffer);
  else
    DEBUG_MSG("Serial Number: ?\n");
  DEBUG_MSG("Flash Memory Size: %d bytes\n", MIOS32_SYS_FlashSizeGet());
  DEBUG_MSG("RAM Size: %d bytes\n", MIOS32_SYS_RAMSizeGet());

  DEBUG_MSG("Systime: %02d:%02d:%02d\n", hours, minutes, seconds);
  DEBUG_MSG("CPU Load: %02d%%\n", SEQ_STATISTICS_CurrentCPULoad());
  DEBUG_MSG("MIDI Scheduler: Alloc %3d/%3d Drops: %3d",
	    seq_midi_out_allocated, seq_midi_out_max_allocated, seq_midi_out_dropouts);

  u32 stopwatch_value_max = SEQ_STATISTICS_StopwatchGetValueMax();
  u32 stopwatch_value = SEQ_STATISTICS_StopwatchGetValue();
  if( stopwatch_value_max == 0xffffffff ) {
    DEBUG_MSG("Stopwatch: Overrun!\n");
  } else if( !stopwatch_value_max ) {
    DEBUG_MSG("Stopwatch: no result yet\n");
  } else {
    DEBUG_MSG("Stopwatch: %d/%d uS\n", stopwatch_value, stopwatch_value_max);
  }

#if !defined(MIOS32_FAMILY_EMULATION) && configGENERATE_RUN_TIME_STATS
  // send Run Time Stats to MIOS terminal
  DEBUG_MSG("FreeRTOS Task RunTime Stats:\n");
  FREERTOS_UTILS_RunTimeStats();
#endif

  DEBUG_MSG("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}


s32 SEQ_TERMINAL_PrintGlobals(void)
{
  MUTEX_MIDIOUT_TAKE;
  DEBUG_MSG("Global Settings:\n");
  DEBUG_MSG("================\n");
  SEQ_FILE_C_Debug();

  DEBUG_MSG("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintTracks(void)
{
  char str_buffer[128];

  MUTEX_MIDIOUT_TAKE;
  DEBUG_MSG("Track Overview:\n");
  DEBUG_MSG("===============\n");

  DEBUG_MSG("| Track | Mode  | Layer P/T/I | Steps P/T | Length | Port  | Chn. | Muted |\n");
  DEBUG_MSG("+-------+-------+-------------+-----------+--------+-------+------+-------+\n");

  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    seq_event_mode_t event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);
    u16 num_instruments = SEQ_TRG_NumInstrumentsGet(track);
    u16 num_par_layers = SEQ_PAR_NumLayersGet(track);
    u16 num_par_steps = SEQ_PAR_NumStepsGet(track);
    u16 num_trg_layers = SEQ_TRG_NumLayersGet(track);
    u16 num_trg_steps = SEQ_TRG_NumStepsGet(track);
    u16 length = (u16)SEQ_CC_Get(track, SEQ_CC_LENGTH) + 1;
    mios32_midi_port_t midi_port = SEQ_CC_Get(track, SEQ_CC_MIDI_PORT);
    u8 midi_chn = SEQ_CC_Get(track, SEQ_CC_MIDI_CHANNEL) + 1;

    sprintf(str_buffer, "| G%dT%d  | %s |",
	    (track/4)+1, (track%4)+1,
	    SEQ_LAYER_GetEvntModeName(event_mode));

    sprintf((char *)(str_buffer + strlen(str_buffer)), "   %2d/%2d/%2d  |  %3d/%3d  |   %3d  | %s%c |  %2d  |",
	    num_par_layers, num_trg_layers, num_instruments, 
	    num_par_steps, num_trg_steps,
	    length,
	    SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(midi_port)),
	    SEQ_MIDI_PORT_OutCheckAvailable(midi_port) ? ' ' : '*',
	    midi_chn);

    if( seq_core_trk[track].state.MUTED )
      sprintf((char *)(str_buffer + strlen(str_buffer)), "  yes  |\n");
    else if( seq_core_trk[track].layer_muted )
      sprintf((char *)(str_buffer + strlen(str_buffer)), " layer |\n");
    else
      sprintf((char *)(str_buffer + strlen(str_buffer)), "  no   |\n");

    DEBUG_MSG(str_buffer);
  }

  DEBUG_MSG("+-------+-------+-------------+-----------+--------+-------+------+-------+\n");

  DEBUG_MSG("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintTrack(u8 track)
{
  MUTEX_MIDIOUT_TAKE;

  DEBUG_MSG("Track Parameters of G%dT%d", (track/4)+1, (track%4)+1);
  DEBUG_MSG("========================\n");

  SEQ_FILE_T_Debug(track);

  DEBUG_MSG("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintCurrentMixerMap(void)
{
  char str_buffer[128];

  MUTEX_MIDIOUT_TAKE;
  u8 map = SEQ_MIXER_NumGet();
  int i;

  DEBUG_MSG("Mixer Map #%3d\n", map+1);
  DEBUG_MSG("==============\n");

  DEBUG_MSG("|Num|Port|Chn|Prg|Vol|Pan|Rev|Cho|Mod|CC1|CC2|CC3|CC4|C1A|C2A|C3A|C4A|\n");
  DEBUG_MSG("+---+----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n");

  for(i=0; i<16; ++i) {
    sprintf(str_buffer, "|%3d|%s|", i, SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(SEQ_MIXER_Get(i, SEQ_MIXER_PAR_PORT))));

    int par;

    for(par=1; par<2; ++par)
      sprintf((char *)(str_buffer + strlen(str_buffer)), "%3d|", SEQ_MIXER_Get(i, par)+1);

    for(par=2; par<12; ++par) {
      u8 value = SEQ_MIXER_Get(i, par);
      if( value )
	sprintf((char *)(str_buffer + strlen(str_buffer)), "%3d|", value-1);
      else
	sprintf((char *)(str_buffer + strlen(str_buffer)), " - |");
    }

    for(par=12; par<16; ++par)
      sprintf((char *)(str_buffer + strlen(str_buffer)), "%3d|", SEQ_MIXER_Get(i, par));

    DEBUG_MSG("%s\n", str_buffer);
  }

  DEBUG_MSG("+---+----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n");
  DEBUG_MSG("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}


s32 SEQ_TERMINAL_PrintCurrentSong(void)
{
  u8 song = SEQ_SONG_NumGet();

  MUTEX_MIDIOUT_TAKE;
  int i;

  DEBUG_MSG("Song #%2d\n", song+1);
  DEBUG_MSG("========\n");

  DEBUG_MSG("Name: '%s'\n", seq_song_name);
  MIOS32_MIDI_SendDebugHexDump((u8 *)&seq_song_steps[0], SEQ_SONG_NUM_STEPS*sizeof(seq_song_step_t));

  DEBUG_MSG("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintGrooveTemplates(void)
{
  MUTEX_MIDIOUT_TAKE;
  DEBUG_MSG("Groove Templates:\n");
  DEBUG_MSG("=================\n");
  SEQ_FILE_G_Debug();

  DEBUG_MSG("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}




///////////////////////////////////////////////////////////////////
// These time and date functions and other bits of following code were adapted from 
// Rickey's world of Microelectronics under the creative commons 2.5 license.
// http://www.8051projects.net/mmc-sd-interface-fat16/final-code.php
static void ShowFatTime(u32 ThisTime, char* msg)
{
   u8 AM = 1;

   int Hour, Minute, Second;

   Hour = ThisTime >> 11;        // bits 15 through 11 hold Hour...
   Minute = ThisTime & 0x07E0;   // bits 10 through 5 hold Minute... 0000 0111 1110 0000
   Minute = Minute >> 5;
   Second = ThisTime & 0x001F;   //bits 4 through 0 hold Second...   0000 0000 0001 1111
   
   if( Hour > 11 )
   {
      AM = 0;
      if( Hour > 12 )
         Hour -= 12;
   }
     
   sprintf( msg, "%02d:%02d:%02d %s", Hour, Minute, Second*2,
         (AM)?"AM":"PM");
   return;
}

static void ShowFatDate(u32 ThisDate, char* msg)
{

   int Year, Month, Day;

   Year = ThisDate >> 9;         // bits 15 through 9 hold year...
   Month = ThisDate & 0x01E0;    // bits 8 through 5 hold month... 0000 0001 1110 0000
   Month = Month >> 5;
   Day = ThisDate & 0x001F;      //bits 4 through 0 hold day...    0000 0000 0001 1111
   sprintf( msg, "%02d/%02d/%02d", Month, Day, Year-20);
   return;
}

s32 SEQ_TERMINAL_PrintSdCardInfo(void)
{
  FRESULT res;
  FILINFO fno;
  DIR dir;
  int i;
  char *fn;
  char str_buffer[128];

  MUTEX_MIDIOUT_TAKE;

  DEBUG_MSG("SD Card Informations\n");
  DEBUG_MSG("====================\n");

#if !defined(MIOS32_FAMILY_EMULATION)
  // this yield ensures, that Debug Messages are sent before we continue the execution
  // Since MIOS Studio displays the time at which the messages arrived, this allows
  // us to measure the delay of following operations
  taskYIELD();

  MUTEX_SDCARD_TAKE;
  SEQ_FILE_PrintSDCardInfos();
  MUTEX_SDCARD_GIVE;
#endif

  DEBUG_MSG("\n");
  DEBUG_MSG("Reading Root Directory\n");
  DEBUG_MSG("======================\n");

  taskYIELD();

  if( !SEQ_FILE_SDCardAvailable() ) {
    sprintf(str_buffer, "not connected");
  } else if( !SEQ_FILE_VolumeAvailable() ) {
    sprintf(str_buffer, "Invalid FAT");
  } else {
    DEBUG_MSG("Retrieving SD Card informations - please wait!\n");
    MUTEX_MIDIOUT_GIVE;
    MUTEX_SDCARD_TAKE;
    SEQ_FILE_UpdateFreeBytes();
    MUTEX_SDCARD_GIVE;
    MUTEX_MIDIOUT_TAKE;

    sprintf(str_buffer, "'%s': %u of %u MB free", 
	    SEQ_FILE_VolumeLabel(),
	    (unsigned int)(SEQ_FILE_VolumeBytesFree()/1000000),
	    (unsigned int)(SEQ_FILE_VolumeBytesTotal()/1000000));
  }
  DEBUG_MSG("SD Card: %s\n", str_buffer);

  taskYIELD();

#if _USE_LFN
  static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];
  fno.lfname = lfn;
  fno.lfsize = sizeof(lfn);
#endif

  MUTEX_SDCARD_TAKE;
  if( (res=f_opendir(&dir, "/")) != FR_OK ) {
    DEBUG_MSG("Failed to open root directory - error status: %d\n", res);
  } else {
    while( (f_readdir(&dir, &fno) == FR_OK) && fno.fname[0] ) {
#if _USE_LFN
      fn = *fno.lfname ? fno.lfname : fno.fname;
#else
      fn = fno.fname;
#endif
      char date[10];
      ShowFatDate(fno.fdate,(char*)&date);
      char time[12];
      ShowFatTime(fno.ftime,(char*)&time);
      DEBUG_MSG("[%s%s%s%s%s%s%s] %s  %s   %s %u %s\n",
		(fno.fattrib & AM_RDO ) ? "r" : ".",
		(fno.fattrib & AM_HID ) ? "h" : ".",
		(fno.fattrib & AM_SYS ) ? "s" : ".",
		(fno.fattrib & AM_VOL ) ? "v" : ".",
		(fno.fattrib & AM_LFN ) ? "l" : ".",
		(fno.fattrib & AM_DIR ) ? "d" : ".",
		(fno.fattrib & AM_ARC ) ? "a" : ".",
		date,time,
		(fno.fattrib & AM_DIR) ? "<DIR>" : " ",
		fno.fsize,fn);
    }
  }
  MUTEX_SDCARD_GIVE;

  taskYIELD();

  DEBUG_MSG("\n");
  DEBUG_MSG("Checking SD Card at application layer\n");
  DEBUG_MSG("=====================================\n");


  {
    u8 bank;
    for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank) {
      int num_patterns = SEQ_FILE_B_NumPatterns(bank);
      if( num_patterns )
	DEBUG_MSG("File MBSEQ_B%d.V4: valid (%d patterns)\n", bank+1, num_patterns);
      else
	DEBUG_MSG("File MBSEQ_B%d.V4: doesn't exist\n", bank+1, num_patterns);
    }

    int num_maps = SEQ_FILE_M_NumMaps();
    if( num_maps )
      DEBUG_MSG("File MBSEQ_M.V4: valid (%d mixer maps)\n", num_maps);
    else
      DEBUG_MSG("File MBSEQ_M.V4: doesn't exist\n");
    
    int num_songs = SEQ_FILE_S_NumSongs();
    if( num_songs )
      DEBUG_MSG("File MBSEQ_S.V4: valid (%d songs)\n", num_songs);
    else
      DEBUG_MSG("File MBSEQ_S.V4: doesn't exist\n");

    if( SEQ_FILE_G_Valid() )
      DEBUG_MSG("File MBSEQ_G.V4: valid\n");
    else
      DEBUG_MSG("File MBSEQ_G.V4: doesn't exist\n");
    
    if( SEQ_FILE_C_Valid() )
      DEBUG_MSG("File MBSEQ_C.V4: valid\n");
    else
      DEBUG_MSG("File MBSEQ_C.V4: doesn't exist\n");
    
    if( SEQ_FILE_HW_Valid() )
      DEBUG_MSG("File MBSEQ_HW.V4: valid\n");
    else
      DEBUG_MSG("File MBSEQ_HW.V4: doesn't exist or hasn't been re-loaded\n");
  }

  DEBUG_MSG("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}
