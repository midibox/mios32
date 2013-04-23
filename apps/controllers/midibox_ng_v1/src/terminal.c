// $Id$
//! \defgroup TERMINAL
//! The MIOS Terminal Handler
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
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
#include <ff.h>

#include <midi_port.h>
#include <midi_router.h>
#include <midimon.h>
#include <keyboard.h>
#include <aout.h>
#include <file.h>
#include <app_lcd.h>

#include "app.h"
#include "terminal.h"
#include "mbng_patch.h"
#include "uip_terminal.h"
#include "tasks.h"
#include "mbng_event.h"
#include "mbng_lcd.h"
#include "mbng_file.h"
#include "mbng_file_c.h"
#include "mbng_file_r.h"

#if !defined(MIOS32_FAMILY_EMULATION)
#include <umm_malloc.h>
#endif

/////////////////////////////////////////////////////////////////////////////
//! Local defines
/////////////////////////////////////////////////////////////////////////////

#define STRING_MAX 100 // recommended size for file transfers via FILE_BrowserHandler()


/////////////////////////////////////////////////////////////////////////////
//! Local variables
/////////////////////////////////////////////////////////////////////////////

static char line_buffer[STRING_MAX];
static u16 line_ix;

static char autoload_ngc_file[9];
static u8 autoload_enabled;

static u8 ngr_section;
static s16 ngr_value;


/////////////////////////////////////////////////////////////////////////////
//! Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 TERMINAL_ParseFilebrowser(mios32_midi_port_t port, char byte);

static s32 TERMINAL_BrowserUploadCallback(char *filename);


/////////////////////////////////////////////////////////////////////////////
//! Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_Init(u32 mode)
{
  // install the callback function which is called on incoming characters from MIOS Terminal
  MIOS32_MIDI_DebugCommandCallback_Init(TERMINAL_Parse);

  // install the callback function which is called on incoming characters from MIOS Filebrowser
  MIOS32_MIDI_FilebrowserCommandCallback_Init(TERMINAL_ParseFilebrowser);

  // clear line buffer
  line_buffer[0] = 0;
  line_ix = 0;
  
  // invalidate autoload file
  autoload_ngc_file[0] = 0;
  // enable autoload by default
  autoload_enabled = 1;

  // for .NGR file/command handling
  ngr_section = 0;
  ngr_value = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! help function which parses a decimal or hex value
//! \returns >= 0 if value is valid
//! \returns -1 if value is invalid
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
//! help function which parses for on or off
//! \returns 0 if 'off', 1 if 'on', -1 if invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_on_off(char *word)
{
  if( strcmp(word, "on") == 0 )
    return 1;

  if( strcmp(word, "off") == 0 )
    return 0;

  return -1;
}


/////////////////////////////////////////////////////////////////////////////
//! Parser
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_Parse(mios32_midi_port_t port, char byte)
{
  // temporary change debug port (will be restored at the end of this function)
  mios32_midi_port_t prev_debug_port = MIOS32_MIDI_DebugPortGet();
  MIOS32_MIDI_DebugPortSet(port);

  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    MUTEX_MIDIOUT_TAKE;
    TERMINAL_ParseLine(line_buffer, MIOS32_MIDI_SendDebugMessage);
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
//! Parser for Filebrowser
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_ParseFilebrowser(mios32_midi_port_t port, char byte)
{
  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    // for the auto-load function
    FILE_BrowserUploadCallback_Init(TERMINAL_BrowserUploadCallback);

    MUTEX_MIDIOUT_TAKE;
    MUTEX_SDCARD_TAKE;
    FILE_BrowserHandler(port, line_buffer);
    MUTEX_SDCARD_GIVE;
    MUTEX_MIDIOUT_GIVE;
    line_ix = 0;
    line_buffer[line_ix] = 0;
  } else if( line_ix < (STRING_MAX-1) ) {
    line_buffer[line_ix++] = byte;
    line_buffer[line_ix] = 0;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! For the auto-load function
/////////////////////////////////////////////////////////////////////////////
static s32 TERMINAL_BrowserUploadCallback(char *filename)
{
  if( filename ) {
    // invalidate autoload file
    autoload_ngc_file[0] = 0;

    // check for .NGC file in root dir

    if( filename[0] == '/' )
      ++filename;

    int len = strlen(filename);

    if( len < 5 || len > 12 || 
	(strcasecmp((char *)&filename[len-4], ".ngc") != 0 &&
	 strcasecmp((char *)&filename[len-4], ".ngl") != 0 &&
	 strcasecmp((char *)&filename[len-4], ".ngr") != 0) )
      return 0; // no .NGC file

    int i;
    for(i=0; i<len; ++i) {
      if( filename[i] == '/' )
	return 0; // not in root dir
    }

    strncpy(autoload_ngc_file, filename, len-4);
    autoload_ngc_file[len-4] = 0;
  } else {
    if( autoload_enabled && autoload_ngc_file[0] ) {
      DEBUG_MSG("AUTOLOAD '%s'\n", autoload_ngc_file);

      s32 status = MBNG_PATCH_Load(autoload_ngc_file);
      if( status >= 0 ) {
	DEBUG_MSG("Patch '%s' loaded from SD Card!", autoload_ngc_file);
      } else {
	DEBUG_MSG("ERROR: failed to load patch '%s' on SD Card (status %d)!", autoload_ngc_file, status);
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Parser for a complete line - also used by shell.c for telnet
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_ParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

  if( UIP_TERMINAL_ParseLine(input, _output_function) > 0 )
    return 0; // command parsed by UIP Terminal

  if( KEYBOARD_TerminalParseLine(input, _output_function) > 0 )
    return 0; // command parsed by Keyboard Terminal

  if( MIDIMON_TerminalParseLine(input, _output_function) > 0 )
    return 0; // command parsed

  if( MIDI_ROUTER_TerminalParseLine(input, _output_function) > 0 )
    return 0; // command parsed

#if !defined(MIOS32_FAMILY_EMULATION)
  if( AOUT_TerminalParseLine(input, _output_function) >= 1 )
    return 0; // command parsed
#endif

#ifdef MIOS32_LCD_universal
  if( APP_LCD_TerminalParseLine(input, _output_function) >= 1 )
    return 0; // command parsed
#endif

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcmp(parameter, "help") == 0 ) {
      out("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
      out("Following commands are available:");
      out("  system:                           print system info");
      out("  memory:                           print memory allocation info\n");
      out("  sdcard:                           print SD Card info\n");
      out("  sdcard_format:                    formats the SD Card (you will be asked for confirmation)\n");
      UIP_TERMINAL_Help(_output_function);
      KEYBOARD_TerminalHelp(_output_function);
      MIDIMON_TerminalHelp(_output_function);
      MIDI_ROUTER_TerminalHelp(_output_function);
      AOUT_TerminalHelp(_output_function);
#ifdef MIOS32_LCD_universal
      APP_LCD_TerminalHelp(_output_function);
#endif
      out("  set dout <pin> <0|1>:             directly sets DOUT (all or 0..%d) to given level (1 or 0)", MIOS32_SRIO_NUM_SR*8 - 1);
      out("  set debug <on|off>:               enables debug messages (current: %s)", debug_verbose_level ? "on" : "off");
      out("  set autoload <on|off>:            enables autoload after filebrowser upload (current: %s)", autoload_enabled ? "on" : "off");
      out("  save <name>:                      stores current config on SD Card");
      out("  load <name>:                      restores config from SD Card");
      out("  show file:                        shows the current configuration file");
      out("  show pool:                        shows the items of the event pool");
      out("  show poolbin:                     shows the event pool in binary format");
      out("  show id <element>:<id>            shows informations about the given element id (e.g. BUTTON:1)");
      out("  show hw_id <element>:<hw_id>      shows informations about the given element hw_id (e.g. BUTTON:1)");
      out("  lcd <string>:                     directly prints a string on LCD (can be formatted!)");
      out("  run [<section>] [<value>]:        executes the .NGR script with the optional section number and value");
      out("  ngr_value:                        value used for 'run' (without parameter) and 'ngr' (is: %d)", ngr_value);
      out("  ngr_section:                      section used for 'run' (without parameter) and 'ngr' (is: %d)", ngr_section);
      out("  ngr <command>:                    directly executes a NGR command");
      out("  ngc <command>:                    directly executes a NGC command");
      out("  msd <on|off>:                     enables Mass Storage Device driver");
      out("  reset:                            resets the MIDIbox (!)\n");
      out("  help:                             this page");
      out("  exit:                             (telnet only) exits the terminal");
    } else if( strcmp(parameter, "system") == 0 ) {
      TERMINAL_PrintSystem(_output_function);
    } else if( strcmp(parameter, "memory") == 0 ) {
      TERMINAL_PrintMemoryInfo(out);
    } else if( strcmp(parameter, "sdcard") == 0 ) {
      TERMINAL_PrintSdCardInfo(out);
    } else if( strcmp(parameter, "sdcard_format") == 0 ) {
      if( !brkt || strcasecmp(brkt, "yes, I'm sure") != 0 ) {
	out("ATTENTION: this command will format your SD Card!!!");
	out("           ALL DATA WILL BE DELETED FOREVER!!!");
	out("           Check the current content with the 'sdcard' command");
	out("           Create a backup on your computer if necessary!");
	out("To start formatting, please enter: sdcard_format yes, I'm sure");
	if( brkt ) {
	  out("('%s' wasn't the right \"password\")", brkt);
	}
      } else {
	MUTEX_SDCARD_TAKE;
	out("Formatting SD Card...");
	FRESULT res;
	if( (res=f_mkfs(0,0,0)) != FR_OK ) {
	  out("Formatting failed with error code: %d!", res);
	} else {
	  out("...with success!");
	  MBNG_FILE_UnloadAllFiles();
	  MBNG_FILE_CreateDefaultFiles();
	}
	MUTEX_SDCARD_GIVE;
      }
    } else if( strcmp(parameter, "msd") == 0 ) {
      char *arg = NULL;
      if( (arg = strtok_r(NULL, separators, &brkt)) ) {
	if( strcmp(arg, "on") == 0 ) {
	  if( TASK_MSD_EnableGet() ) {
	    out("Mass Storage Device Mode already activated!\n");
	  } else {
	    out("Mass Storage Device Mode activated - USB MIDI will be disabled!!!\n");
	    // wait a second to ensure that this message is print in MIOS Terminal
	    int d;
	    for(d=0; d<1000; ++d)
	      MIOS32_DELAY_Wait_uS(1000);
	    // activate MSD mode
	    TASK_MSD_EnableSet(1);
	  }
	} else if( strcmp(arg, "off") == 0 ) {
	  if( !TASK_MSD_EnableGet() ) {
	    out("Mass Storage Device Mode already deactivated!\n");
	  } else {
	    out("Mass Storage Device Mode deactivated - USB MIDI will be available again\n");
	    TASK_MSD_EnableSet(0);
	  }
	} else
	  arg = NULL;
      }
      if( arg == NULL ) {
	out("Please enter 'msd on' or 'msd off'\n");
      }      
    } else if( strcmp(parameter, "lcd") == 0 ) {
      if( !brkt || !strlen(brkt) ) {
	out("Please specify string (can be optionally formatted)");
      } else {
	out("Print '%s'", brkt);

	MUTEX_LCD_TAKE;

	// print from a dummy item
	mbng_event_item_t item;
	MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_DISABLED);
	item.label = brkt;
	MBNG_LCD_PrintItemLabel(&item, NULL, 0);

	MUTEX_LCD_GIVE;
      }
    } else if( strcmp(parameter, "run") == 0 ) {
      s32 section, value;
      if( (parameter = strtok_r(NULL, separators, &brkt)) ) {
	if( (section=get_dec(parameter)) < 0 || section >= 256 ) {
	  out("Section number should be between 0..255!");
	} else {
	  ngr_section = section;
	}

	if( (parameter = strtok_r(NULL, separators, &brkt)) ) {
	  if( (value=get_dec(parameter)) < -16384 || value >= 16383 ) {
	    out("Value should be between -16384..16383!");
	  } else {
	    ngr_value = value;
	  }
	}
      }

      if( !MBNG_FILE_R_Valid() ) {
	out("ERROR: can't execute - missing %s.NGR file!", mbng_file_r_script_name);
      } else {
	out("Executing %s.NGR with ^section==%d ^value==%d", mbng_file_r_script_name, ngr_section, ngr_value);
	MBNG_FILE_R_ReadRequest(NULL, ngr_section, ngr_value, 1);
      }
    } else if( strcmp(parameter, "runstop") == 0 ) {
      if( MBNG_FILE_R_RunStop() > 0 ) {
	out("Stopped the execution of %s.NGR", mbng_file_r_script_name);
      } else {
	out("%s.NGR script not running.", mbng_file_r_script_name);
      }
    } else if( strcasecmp(parameter, "ngr_section") == 0 ) {
      s32 section;
      if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	out("Please specify <ngr-section>! Current value: %d", ngr_section);
      } else if( (section=get_dec(parameter)) < 0 || section >= 256 ) {
	out("Section number should be between 0..255!");
      } else {
	ngr_section = section;
	out(".NGR section set to %d", ngr_section);
      }
    } else if( strcasecmp(parameter, "ngr_value") == 0 ) {
      s32 value;

      if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	out("Please specify <ngr-value>! Current value: %d", ngr_value);
      } else if( (value=get_dec(parameter)) < -16384 || value >= 16383 ) {
	out("Value should be between -16384..16383!");
      } else {
	ngr_value = value;
	out(".NGR value set to %d", ngr_value);
      }
    } else if( strcasecmp(parameter, "ngr") == 0 ) {
      MBNG_FILE_R_Parser(0, brkt, NULL, NULL, ngr_section, ngr_value);
      out("Executed command with ^section==%d ^value==%d", ngr_section, ngr_value);
    } else if( strcasecmp(parameter, "ngc") == 0 ) {
      u8 got_first_event_item = 0;
      MBNG_FILE_C_Parser(0, brkt, &got_first_event_item);
      out("Executed command.");
    } else if( strcmp(parameter, "save") == 0 ) {
      if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	out("ERROR: please specify filename for patch (up to 8 characters)!");
      } else {
	if( strlen(parameter) > 8 ) {
	  out("ERROR: 8 characters maximum!");
	} else {
	  s32 status = MBNG_PATCH_Store(parameter);
	  if( status >= 0 ) {
	    out("Patch '%s' stored on SD Card!", parameter);
	  } else {
	    out("ERROR: failed to store patch '%s' on SD Card (status %d)!", parameter, status);
	  }
	}
      }
    } else if( strcmp(parameter, "load") == 0 ) {
      if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	out("ERROR: please specify filename for patch (up to 8 characters)!");
      } else {
	if( strlen(parameter) > 8 ) {
	  out("ERROR: 8 characters maximum!");
	} else {
	  s32 status = MBNG_PATCH_Load(parameter);
	  if( status >= 0 ) {
	    out("Patch '%s' loaded from SD Card!", parameter);
	  } else {
	    out("ERROR: failed to load patch '%s' on SD Card (status %d)!", parameter, status);
	  }
	}
      }
    } else if( strcmp(parameter, "show") == 0 ) {
      if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	out("ERROR: please specify the item which should be displayed!");
      } else {
	if( strcmp(parameter, "file") == 0 ) {
	  MBNG_FILE_C_Debug();
	} else if( strcmp(parameter, "poolbin") == 0 ) {
	  MBNG_EVENT_PoolPrint();
	} else if( strcmp(parameter, "pool") == 0 ) {
	  MBNG_EVENT_PoolItemsPrint();
	  MBNG_EVENT_PoolMapsPrint();
	} else if( strcmp(parameter, "id") == 0 || strcmp(parameter, "hw_id") == 0 ) {
	  u8 search_hw_id = strcmp(parameter, "hw_id") == 0;
	  const char *separator_colon = ":";

	  char *id_str = brkt;
	  if( id_str == NULL || !strlen(id_str) ) {
	    out("Please specify <element>:<id> (e.g. LED:1)!");
	  } else {
	    char *values_str;
	    mbng_event_item_id_t id;
	    if( !(values_str = strtok_r(NULL, separator_colon, &brkt)) ||
		(id=MBNG_EVENT_ItemIdFromControllerStrGet(values_str)) == MBNG_EVENT_CONTROLLER_DISABLED ) {
	      out("Invalid element name '%s'!", id_str);
	    } else {
	      char *id_lower_str = brkt;
	      int id_lower = 0;
	      if( !(values_str = strtok_r(NULL, separator_colon, &brkt)) ||
		  (id_lower=get_dec(values_str)) < 1 || id_lower > 0xfff ) {
		out("Invalid element %s '%s:%s' (expecting %s:1 .. %s:4095)!", search_hw_id ? "hw_id" : "id", id_str, id_lower_str, id_str, id_str);
	      } else {
		id = id | id_lower;

		if( search_hw_id ) {
		  u8 num = MBNG_EVENT_ItemSearchByHwIdAndPrint(id);
		  if( num < 1 ) {
		    out("No items found which are assigned to this hw_id!");
		  }
		} else {
		  u8 num = MBNG_EVENT_ItemSearchByIdAndPrint(id);
		  if( num < 1 ) {
		    out("No items found which are assigned to this id!");
		  }
		}
	      }
	    }
	  }
	} else {
	  out("ERROR: invalid item which should be showed - see 'show pool' for available items!");
	}
      }
    } else if( strcmp(parameter, "reset") == 0 ) {
      MIOS32_SYS_Reset();
    } else if( strcmp(parameter, "set") == 0 ) {
      if( (parameter = strtok_r(NULL, separators, &brkt)) ) {
	if( strcmp(parameter, "dout") == 0 ) {
	  s32 pin = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) ) {
	    if( strcmp(parameter, "all") == 0 ) {
	      pin = -42;
	    } else {
	      pin = get_dec(parameter);
	    }
	  }

	  if( (pin < 0 && pin != -42) || pin >= (MIOS32_SRIO_NUM_SR*8) ) {
	    out("Pin number should be between 0..%d", MIOS32_SRIO_NUM_SR*8 - 1);
	  } else {
	    s32 value = -1;
	    if( (parameter = strtok_r(NULL, separators, &brkt)) )
	      value = get_dec(parameter);

	    if( value < 0 || value > 1 ) {
	      out("Expecting value 1 or 0 for DOUT pin %d", pin);
	    } else {
	      if( pin == -42 ) {
		for(pin=0; pin<(MIOS32_SRIO_NUM_SR*8); ++pin)
		  MIOS32_DOUT_PinSet(pin, value);
		out("All DOUT pins set to %d", value);
	      } else {
		MIOS32_DOUT_PinSet(pin, value);
		out("DOUT Pin %d (SR#%d.D%d) set to %d", pin, (pin/8)+1, 7-(pin%8), value);
	      }
	    }
	  }

	} else if( strcmp(parameter, "debug") == 0 ) {
	  int on_off = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    on_off = get_on_off(parameter);

	  if( on_off < 0 ) {
	    out("Expecting 'on' or 'off'");
	  } else {
	    debug_verbose_level = on_off ? DEBUG_VERBOSE_LEVEL_INFO : DEBUG_VERBOSE_LEVEL_ERROR;
	    out("Debug mode turned %s", on_off ? "on" : "off");
	  }

	} else if( strcmp(parameter, "autoload") == 0 ) {
	  int on_off = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    on_off = get_on_off(parameter);

	  if( on_off < 0 ) {
	    out("Expecting 'on' or 'off'");
	  } else {
	    autoload_enabled = on_off;
	    out("Autoload of .NGC file after filebrowser upload %s", on_off ? "on" : "off");
	  }
	} else {
	  out("Unknown set parameter: '%s'!", parameter);
	}
      } else {
	out("Missing parameter after 'set'!");
      }
    } else {
      out("Unknown command - type 'help' to list available commands!");
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! System Informations
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_PrintSystem(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("Application: " MIOS32_LCD_BOOT_MSG_LINE1);

  MIDIMON_TerminalPrintConfig(out);

  AOUT_TerminalPrintConfig(out);

  APP_LCD_TerminalPrintConfig(out);

  out("Event Pool Number of Items: %d", MBNG_EVENT_PoolNumItemsGet());
  u32 pool_size = MBNG_EVENT_PoolSizeGet();
  u32 pool_max_size = MBNG_EVENT_PoolMaxSizeGet();
  out("Event Pool Allocation: %d of %d bytes (%d%%)",
      pool_size, pool_max_size, (100*pool_size)/pool_max_size);

#if !defined(MIOS32_FAMILY_EMULATION) && (configGENERATE_RUN_TIME_STATS || configUSE_TRACE_FACILITY)
  // send Run Time Stats to MIOS terminal
  out("FreeRTOS Task RunTime Stats:\n");
  FREERTOS_UTILS_RunTimeStats();
#endif

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Memory allocation Informations
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_PrintMemoryInfo(void *_output_function)
{
  //void (*out)(char *format, ...) = _output_function;
  // TODO: umm_info doesn't allow to define output function

#if !defined(MIOS32_FAMILY_EMULATION)
  umm_info( NULL, 1 );
#endif

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! SDCard Informations
/////////////////////////////////////////////////////////////////////////////
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

s32 TERMINAL_PrintSdCardInfo(void *_output_function)
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

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}


//! \}
