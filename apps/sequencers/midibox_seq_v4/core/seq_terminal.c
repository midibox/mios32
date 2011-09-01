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

#include <aout.h>

#include "tasks.h"

#include "app.h"
#include "seq_terminal.h"


#include "seq_core.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_midi_port.h"
#include "seq_song.h"
#include "seq_mixer.h"

#include "file.h"
#include "seq_file.h"
#include "seq_file_b.h"
#include "seq_file_s.h"
#include "seq_file_m.h"
#include "seq_file_g.h"
#include "seq_file_c.h"
#include "seq_file_t.h"
#include "seq_file_gc.h"
#include "seq_file_bm.h"
#include "seq_file_hw.h"

#include "seq_ui.h"

#include "seq_statistics.h"

#if !defined(MIOS32_FAMILY_EMULATION)
#include "umm_malloc.h"
#include "uip.h"
#include "uip_task.h"
#include "osc_server.h"
#endif


#if !defined(MIOS32_FAMILY_EMULATION)
// for AOUT interface testmode
// TODO: allow access to these pins via MIOS32_SPI driver
#if defined(MIOS32_FAMILY_STM32F10x)
#define MIOS32_SPI2_HIGH_VOLTAGE 4

#define MIOS32_SPI2_SCLK_PORT  GPIOB
#define MIOS32_SPI2_SCLK_PIN   GPIO_Pin_6
#define MIOS32_SPI2_MOSI_PORT  GPIOB
#define MIOS32_SPI2_MOSI_PIN   GPIO_Pin_5

#define MIOS32_SPI2_SCLK_INIT   { } // already configured as GPIO
#define MIOS32_SPI2_SCLK_SET(b) { MIOS32_SPI2_SCLK_PORT->BSRR = (b) ? MIOS32_SPI2_SCLK_PIN : (MIOS32_SPI2_SCLK_PIN << 16); }
#define MIOS32_SPI2_MOSI_INIT   { } // already configured as GPIO
#define MIOS32_SPI2_MOSI_SET(b) { MIOS32_SPI2_MOSI_PORT->BSRR = (b) ? MIOS32_SPI2_MOSI_PIN : (MIOS32_SPI2_MOSI_PIN << 16); }

#elif defined(MIOS32_FAMILY_LPC17xx)
#define MIOS32_SPI2_HIGH_VOLTAGE 5

#define MIOS32_SPI2_SCLK_INIT    { MIOS32_SYS_LPC_PINSEL(0, 15, 0); MIOS32_SYS_LPC_PINDIR(0, 15, 1); }
#define MIOS32_SPI2_SCLK_SET(v)  { MIOS32_SYS_LPC_PINSET(0, 15, v); }
#define MIOS32_SPI2_MOSI_INIT    { MIOS32_SYS_LPC_PINSEL(0, 18, 0); MIOS32_SYS_LPC_PINDIR(0, 18, 1); }
#define MIOS32_SPI2_MOSI_SET(v)  { MIOS32_SYS_LPC_PINSET(0, 18, v); }
#else
# error "Please adapt MIOS32_SPI settings!"
#endif
#else
# define MIOS32_SPI2_HIGH_VOLTAGE 5
#endif



/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define STRING_MAX 80


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static char line_buffer[STRING_MAX];
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
    MUTEX_MIDIOUT_TAKE;
    SEQ_TERMINAL_ParseLine(line_buffer, MIOS32_MIDI_SendDebugMessage);
    MUTEX_MIDIOUT_GIVE;
    line_ix = 0;
    line_buffer[line_ix] = 0;
  } else if( line_ix < (STRING_MAX-1) ) {
    line_buffer[line_ix++] = byte;
    line_buffer[line_ix] = 0;
  }

  // restore debug port
  MIOS32_MIDI_DebugPortSet(prev_debug_port);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line - also used by shell.c for telnet
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TERMINAL_ParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

  if( (parameter = strtok_r(line_buffer, separators, &brkt)) ) {
    if( strcmp(parameter, "help") == 0 ) {
      SEQ_TERMINAL_PrintHelp(out);
    } else if( strcmp(parameter, "system") == 0 ) {
      SEQ_TERMINAL_PrintSystem(out);
    } else if( strcmp(parameter, "global") == 0 ) {
      SEQ_TERMINAL_PrintGlobalConfig(out);
    } else if( strcmp(parameter, "bookmarks") == 0 ) {
      SEQ_TERMINAL_PrintBookmarks(out);
    } else if( strcmp(parameter, "config") == 0 ) {
      SEQ_TERMINAL_PrintSessionConfig(out);
    } else if( strcmp(parameter, "tracks") == 0 ) {
      SEQ_TERMINAL_PrintTracks(out);
    } else if( strcmp(parameter, "track") == 0 ) {
      char *arg;
      if( (arg = strtok_r(NULL, separators, &brkt)) ) {
	int track = get_dec(arg);
	if( track < 1 || track > SEQ_CORE_NUM_TRACKS ) {
	  out("Wrong track number %d - expected track 1..%d\n", track, SEQ_CORE_NUM_TRACKS);
	} else {
	  SEQ_TERMINAL_PrintTrack(out, track-1);
	}
      } else {
	out("Please specify track, e.g. \"track 1\"\n");
      }
    } else if( strcmp(parameter, "mixer") == 0 ) {
      SEQ_TERMINAL_PrintCurrentMixerMap(out);
    } else if( strcmp(parameter, "song") == 0 ) {
      SEQ_TERMINAL_PrintCurrentSong(out);
    } else if( strcmp(parameter, "grooves") == 0 ) {
      SEQ_TERMINAL_PrintGrooveTemplates(out);
    } else if( strcmp(parameter, "memory") == 0 ) {
      SEQ_TERMINAL_PrintMemoryInfo(out);
#if !defined(MIOS32_FAMILY_EMULATION)
    } else if( strcmp(parameter, "network") == 0 ) {
      SEQ_TERMINAL_PrintNetworkInfo(out);
    } else if( strcmp(parameter, "udpmon") == 0 ) {
      char *arg;
      if( (arg = strtok_r(NULL, separators, &brkt)) ) {
	int level = get_dec(arg);
	switch( level ) {
	case UDP_MONITOR_LEVEL_0_OFF:
	  out("Set UDP monitor level to %d (off)\n", level);
	  break;
	case UDP_MONITOR_LEVEL_1_OSC_REC:
	  out("Set UDP monitor level to %d (received packets assigned to a OSC1..4 port)\n", level);
	  break;
	case UDP_MONITOR_LEVEL_2_OSC_REC_AND_SEND:
	  out("Set UDP monitor level to %d (received and sent packets assigned to a OSC1..4 port)\n", level);
	  break;
	case UDP_MONITOR_LEVEL_3_ALL_GEQ_1024:
	  out("Set UDP monitor level to %d (all received and sent packets with port number >= 1024)\n", level);
	  break;
	case UDP_MONITOR_LEVEL_4_ALL:
	  out("Set UDP monitor level to %d (all received and sent packets)\n", level);
	  break;
	default:
	  out("Invalid level %d - please specify monitor level 0..4\n", level);
	  level = -1; // invalidate level for next if() check
	}
	
	if( level >= 0 )
	  UIP_TASK_UDP_MonitorLevelSet(level);
      } else {
	out("Please specify monitor level (0..4)\n");
      }
#endif
    } else if( strcmp(parameter, "sdcard") == 0 ) {
      SEQ_TERMINAL_PrintSdCardInfo(out);
    } else if( strcmp(parameter, "testaoutpin") == 0 ) {
      char *arg;
      int pin_number = -1;
      int level = -1;
      
      if( (arg = strtok_r(NULL, separators, &brkt)) ) {
	if( strcmp(arg, "cs") == 0 )
	  pin_number = 1;
	else if( strcmp(arg, "si") == 0 )
	  pin_number = 2;
	else if( strcmp(arg, "sc") == 0 )
	  pin_number = 3;
	else if( strcmp(arg, "reset") == 0 ) {
	  pin_number = 0;
	  level = 0; // dummy
	}
      }
      
      if( pin_number < 0 ) {
	out("Please specifiy valid AOUT pin name: cs, si or sc\n");
      } else {
	if( (arg = strtok_r(NULL, separators, &brkt)) )
	  level = get_dec(arg);
	
	if( level != 0 && level != 1 ) {
	  out("Please specifiy valid logic level for AOUT pin: 0 or 1\n");
	}
      }

      if( pin_number >= 0 && level >= 0 ) {
	SEQ_TERMINAL_TestAoutPin(out, pin_number, level);
      } else {
	out("Following commands are supported:\n");
	out("testaoutpin cs 0  -> sets AOUT:CS to 0.4V");
	out("testaoutpin cs 1  -> sets AOUT:CS to ca. 4V");
	out("testaoutpin si 0  -> sets AOUT:SI to ca. 0.4V");
	out("testaoutpin si 1  -> sets AOUT:SI to ca. 4V");
	out("testaoutpin sc 0  -> sets AOUT:SC to ca. 0.4V");
	out("testaoutpin sc 1  -> sets AOUT:SC to ca. 4V");
	out("testaoutpin reset -> re-initializes AOUT module so that it can be used again.");
      }
    } else if( strcmp(parameter, "play") == 0 ) {
      SEQ_UI_Button_Play(0);
      out("Sequencer started...\n");
    } else if( strcmp(parameter, "stop") == 0 ) {
      SEQ_UI_Button_Stop(0);
      out("Sequencer stopped...\n");
    } else if( strcmp(parameter, "reset") == 0 ) {
      MIOS32_SYS_Reset();
    } else {
      out("Unknown command - type 'help' to list available commands!\n");
    }
  }

  return 0; // no error
}


s32 SEQ_TERMINAL_PrintHelp(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;
  out("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
  out("Following commands are available:");
  out("  system:         print system info\n");
  out("  global:         print global configuration\n");
  out("  config:         print local session configuration\n");
  out("  tracks:         print overview of all tracks\n");
  out("  track <track>:  print info about a specific track\n");
  out("  mixer:          print current mixer map\n");
  out("  song:           print current song info\n");
  out("  grooves:        print groove templates\n");
  out("  bookmarks:      print bookmarks\n");
  out("  memory:         print memory allocation info\n");
  out("  sdcard:         print SD Card info\n");
#if !defined(MIOS32_FAMILY_EMULATION)
  out("  network:        print ethernet network info\n");
  out("  udpmon <0..4>:  enables UDP monitor to check OSC packets (current: %d)\n", UIP_TASK_UDP_MonitorLevelGet());
#endif
  out("  testaoutpin:    type this command to get further informations about the testmode.");
  out("  play:           emulates the PLAY button\n");
  out("  stop:           emulates the STOP button\n");
  out("  reset:          resets the MIDIbox SEQ (!)\n");
  out("  help:           this page\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}


s32 SEQ_TERMINAL_PrintSystem(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char str_buffer[128];

  MUTEX_MIDIOUT_TAKE;

  out("System Informations:\n");
  out("====================\n");
  out(MIOS32_LCD_BOOT_MSG_LINE1 " " MIOS32_LCD_BOOT_MSG_LINE2 "\n");

  mios32_sys_time_t t = MIOS32_SYS_TimeGet();
  int hours = (t.seconds / 3600) % 24;
  int minutes = (t.seconds % 3600) / 60;
  int seconds = (t.seconds % 3600) % 60;

  out("Operating System: MIOS32\n");
  out("Board: " MIOS32_BOARD_STR "\n");
  out("Chip Family: " MIOS32_FAMILY_STR "\n");
  if( MIOS32_SYS_SerialNumberGet((char *)str_buffer) >= 0 )
    out("Serial Number: %s\n", str_buffer);
  else
    out("Serial Number: ?\n");
  out("Flash Memory Size: %d bytes\n", MIOS32_SYS_FlashSizeGet());
  out("RAM Size: %d bytes\n", MIOS32_SYS_RAMSizeGet());

  out("Systime: %02d:%02d:%02d\n", hours, minutes, seconds);
  out("CPU Load: %02d%%\n", SEQ_STATISTICS_CurrentCPULoad());
  out("MIDI Scheduler: Alloc %3d/%3d Drops: %3d",
	    seq_midi_out_allocated, seq_midi_out_max_allocated, seq_midi_out_dropouts);

  u32 stopwatch_value_max = SEQ_STATISTICS_StopwatchGetValueMax();
  u32 stopwatch_value = SEQ_STATISTICS_StopwatchGetValue();
  if( stopwatch_value_max == 0xffffffff ) {
    out("Stopwatch: Overrun!\n");
  } else if( !stopwatch_value_max ) {
    out("Stopwatch: no result yet\n");
  } else {
    out("Stopwatch: %d/%d uS\n", stopwatch_value, stopwatch_value_max);
  }

#if !defined(MIOS32_FAMILY_EMULATION) && configGENERATE_RUN_TIME_STATS
  // send Run Time Stats to MIOS terminal
  out("FreeRTOS Task RunTime Stats:\n");
  FREERTOS_UTILS_RunTimeStats();
#endif

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}


s32 SEQ_TERMINAL_PrintGlobalConfig(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;
  out("Global Configuration:\n");
  out("=====================\n");
  SEQ_FILE_GC_Debug();

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintBookmarks(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;
  out("Global Bookmarks:\n");
  out("=================\n");
  SEQ_FILE_BM_Debug(1);

  out("\n");

  out("Session Bookmarks:\n");
  out("==================\n");
  SEQ_FILE_BM_Debug(0);

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintSessionConfig(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;
  out("Session Configuration:\n");
  out("======================\n");
  SEQ_FILE_C_Debug();

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintTracks(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char str_buffer[128];

  MUTEX_MIDIOUT_TAKE;
  out("Track Overview:\n");
  out("===============\n");

  out("| Track | Mode  | Layer P/T/I | Steps P/T | Length | Port  | Chn. | Muted |\n");
  out("+-------+-------+-------------+-----------+--------+-------+------+-------+\n");

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

    if( seq_core_trk_muted & (1 << track) )
      sprintf((char *)(str_buffer + strlen(str_buffer)), "  yes  |\n");
    else if( seq_core_trk[track].layer_muted )
      sprintf((char *)(str_buffer + strlen(str_buffer)), " layer |\n");
    else
      sprintf((char *)(str_buffer + strlen(str_buffer)), "  no   |\n");

    out(str_buffer);
  }

  out("+-------+-------+-------------+-----------+--------+-------+------+-------+\n");

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintTrack(void *_output_function, u8 track)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;

  out("Track Parameters of G%dT%d", (track/4)+1, (track%4)+1);
  out("========================\n");

  SEQ_FILE_T_Debug(track);

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintCurrentMixerMap(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;
  char str_buffer[128];
  u8 map = SEQ_MIXER_NumGet();
  int i;

  out("Mixer Map #%3d\n", map+1);
  out("==============\n");

  out("|Num|Port|Chn|Prg|Vol|Pan|Rev|Cho|Mod|CC1|CC2|CC3|CC4|C1A|C2A|C3A|C4A|\n");
  out("+---+----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n");

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

    out("%s\n", str_buffer);
  }

  out("+---+----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n");
  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}


s32 SEQ_TERMINAL_PrintCurrentSong(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;
  u8 song = SEQ_SONG_NumGet();

  out("Song #%2d\n", song+1);
  out("========\n");

  out("Name: '%s'\n", seq_song_name);
  MIOS32_MIDI_SendDebugHexDump((u8 *)&seq_song_steps[0], SEQ_SONG_NUM_STEPS*sizeof(seq_song_step_t));

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintGrooveTemplates(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;
  out("Groove Templates:\n");
  out("=================\n");
  SEQ_FILE_G_Debug();

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

extern void * ptr_array[256];
s32 SEQ_TERMINAL_PrintMemoryInfo(void *_output_function)
{
  //void (*out)(char *format, ...) = _output_function;
  // TODO: umm_info doesn't allow to define output function

#if !defined(MIOS32_FAMILY_EMULATION)
  umm_info( NULL, 1 );
#endif

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


s32 SEQ_TERMINAL_PrintSdCardInfo(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  FRESULT res;
  FILINFO fno;
  DIR dir;
  char *fn;
  char str_buffer[128];

  MUTEX_MIDIOUT_TAKE;

  out("SD Card Informations\n");
  out("====================\n");

#if !defined(MIOS32_FAMILY_EMULATION)
  // this yield ensures, that Debug Messages are sent before we continue the execution
  // Since MIOS Studio displays the time at which the messages arrived, this allows
  // us to measure the delay of following operations
  taskYIELD();

  MUTEX_SDCARD_TAKE;
  FILE_PrintSDCardInfos();
  MUTEX_SDCARD_GIVE;
#endif

  out("\n");
  out("Reading Root Directory\n");
  out("======================\n");

  taskYIELD();

  if( !FILE_SDCardAvailable() ) {
    sprintf(str_buffer, "not connected");
  } else if( !FILE_VolumeAvailable() ) {
    sprintf(str_buffer, "Invalid FAT");
  } else {
    out("Retrieving SD Card informations - please wait!\n");
    MUTEX_MIDIOUT_GIVE;
    MUTEX_SDCARD_TAKE;
    FILE_UpdateFreeBytes();
    MUTEX_SDCARD_GIVE;
    MUTEX_MIDIOUT_TAKE;

    sprintf(str_buffer, "'%s': %u of %u MB free", 
	    FILE_VolumeLabel(),
	    (unsigned int)(FILE_VolumeBytesFree()/1000000),
	    (unsigned int)(FILE_VolumeBytesTotal()/1000000));
  }
  out("SD Card: %s\n", str_buffer);

  taskYIELD();

#if _USE_LFN
  static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];
  fno.lfname = lfn;
  fno.lfsize = sizeof(lfn);
#endif

  MUTEX_SDCARD_TAKE;
  if( (res=f_opendir(&dir, "/")) != FR_OK ) {
    out("Failed to open root directory - error status: %d\n", res);
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
      out("[%s%s%s%s%s%s%s] %s  %s   %s %u %s\n",
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

  out("\n");
  out("Checking SD Card at application layer\n");
  out("=====================================\n");

  out("Current session: /SESSIONS/%s\n", seq_file_session_name);

  {
    u8 bank;
    for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank) {
      int num_patterns = SEQ_FILE_B_NumPatterns(bank);
      if( num_patterns )
	out("File /SESSIONS/%s/MBSEQ_B%d.V4: valid (%d patterns)\n", seq_file_session_name, bank+1, num_patterns);
      else
	out("File /SESSIONS/%s/MBSEQ_B%d.V4: doesn't exist\n", seq_file_session_name, bank+1, num_patterns);
    }

    int num_maps = SEQ_FILE_M_NumMaps();
    if( num_maps )
      out("File /SESSIONS/%sMBSEQ_M.V4: valid (%d mixer maps)\n", seq_file_session_name, num_maps);
    else
      out("File /SESSIONS/%s/MBSEQ_M.V4: doesn't exist\n", seq_file_session_name);
    
    int num_songs = SEQ_FILE_S_NumSongs();
    if( num_songs )
      out("File /SESSIONS/%s/MBSEQ_S.V4: valid (%d songs)\n", seq_file_session_name, num_songs);
    else
      out("File /SESSIONS/%s/MBSEQ_S.V4: doesn't exist\n", seq_file_session_name);

    if( SEQ_FILE_G_Valid() )
      out("File /SESSIONS/%s/MBSEQ_G.V4: valid\n", seq_file_session_name);
    else
      out("File /SESSIONS/%s/MBSEQ_G.V4: doesn't exist\n", seq_file_session_name);
    
    if( SEQ_FILE_C_Valid() )
      out("File /SESSIONS/%s/MBSEQ_C.V4: valid\n", seq_file_session_name);
    else
      out("File /SESSIONS/%s/MBSEQ_C.V4: doesn't exist\n", seq_file_session_name);

#ifndef MBSEQV4L    
    if( SEQ_FILE_HW_Valid() )
      out("File /SESSIONS/%s/MBSEQ_HW.V4: valid\n", seq_file_session_name);
    else
      out("File /SESSIONS/%s/MBSEQ_HW.V4: doesn't exist or hasn't been re-loaded\n", seq_file_session_name);
#else
    if( SEQ_FILE_HW_Valid() )
      out("File /SESSIONS/%s/MBSEQ_HW.V4L: valid\n", seq_file_session_name);
    else
      out("File /SESSIONS/%s/MBSEQ_HW.V4L: doesn't exist or hasn't been re-loaded\n", seq_file_session_name);
#endif
  }

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}


s32 SEQ_TERMINAL_PrintNetworkInfo(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;

#if defined(MIOS32_FAMILY_EMULATION)
  out("No network informations available in emulation!");
#else

  out("Ethernet link available: %s", UIP_TASK_NetworkDeviceAvailable() ? "yes" : "no");
  if( !UIP_TASK_NetworkDeviceAvailable() ) {
#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
    // since MBHP_ETH module is used
    out("Please reboot your MIDIbox SEQ to restart module detection! (or just type \"reset\")");
#endif
  } else {
    out("Ethernet services running: %s", UIP_TASK_ServicesRunning() ? "yes" : "no");
    out("DHCP: %s", UIP_TASK_DHCP_EnableGet() ? "enabled" : "disabled");

    if( UIP_TASK_DHCP_EnableGet() && !UIP_TASK_ServicesRunning() ) {
      out("IP address: not available yet");
      out("Netmask: not available yet");
      out("Default Router (Gateway): not available yet");
    } else {
      uip_ipaddr_t ipaddr;
      uip_gethostaddr(&ipaddr);
      out("IP address: %d.%d.%d.%d",
	  uip_ipaddr1(ipaddr), uip_ipaddr2(ipaddr),
	  uip_ipaddr3(ipaddr), uip_ipaddr4(ipaddr));

      uip_ipaddr_t netmask;
      uip_getnetmask(&netmask);
      out("Netmask: %d.%d.%d.%d",
	  uip_ipaddr1(netmask), uip_ipaddr2(netmask),
	  uip_ipaddr3(netmask), uip_ipaddr4(netmask));

      uip_ipaddr_t draddr;
      uip_getdraddr(&draddr);
      out("Default Router (Gateway): %d.%d.%d.%d",
	  uip_ipaddr1(draddr), uip_ipaddr2(draddr),
	  uip_ipaddr3(draddr), uip_ipaddr4(draddr));
    }

    int con;
    for(con=0; con<OSC_SERVER_NUM_CONNECTIONS; ++con) {
      u32 osc_remote_ip = OSC_SERVER_RemoteIP_Get(con);
      out("OSC%d Remote address: %d.%d.%d.%d",
	  con+1,
	  (osc_remote_ip>>24)&0xff, (osc_remote_ip>>16)&0xff,
	  (osc_remote_ip>>8)&0xff, (osc_remote_ip>>0)&0xff);
      out("OSC%d Remote port: %d", con+1, OSC_SERVER_RemotePortGet(con));
      out("OSC%d Local port: %d", con+1, OSC_SERVER_LocalPortGet(con));
    }
  }
#endif
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}


s32 SEQ_TERMINAL_TestAoutPin(void *_output_function, u8 pin_number, u8 level)
{
  void (*out)(char *format, ...) = _output_function;
  s32 status = 0;

  MUTEX_MIDIOUT_TAKE;

  switch( pin_number ) {
  case 0:
    AOUT_SuspendSet(0);
    out("Module has been re-initialized and can be used again!\n");
    break;

  case 1:
    AOUT_SuspendSet(1);
    out("Setting AOUT:CS pin to ca. %dV - please measure now!\n", level ? MIOS32_SPI2_HIGH_VOLTAGE : 0);
#if !defined(MIOS32_FAMILY_EMULATION)
    MIOS32_SPI_RC_PinSet(2, 0, level ? 1 : 0); // spi, rc_pin, pin_value
#endif
    break;

  case 2:
    AOUT_SuspendSet(1);
    out("Setting AOUT:SI pin to ca. %dV - please measure now!\n", level ? MIOS32_SPI2_HIGH_VOLTAGE : 0);
#if !defined(MIOS32_FAMILY_EMULATION)
    MIOS32_SPI2_MOSI_INIT;
    MIOS32_SPI2_MOSI_SET(level ? 1 : 0);
#endif
    break;

  case 3:
    AOUT_SuspendSet(1);
    out("Setting AOUT:SC pin to ca. %dV - please measure now!\n", level ? MIOS32_SPI2_HIGH_VOLTAGE : 0);
#if !defined(MIOS32_FAMILY_EMULATION)
    MIOS32_SPI2_SCLK_INIT;
    MIOS32_SPI2_SCLK_SET(level ? 1 : 0);
#endif
    break;

  default:
    out("ERROR: unsupported pin #%d", pin_number);
  }

  MUTEX_MIDIOUT_GIVE;

  return status;
}

