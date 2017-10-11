// $Id$
/*
 * MIDIbox SEQ V4
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
#include <stdarg.h>

#include <seq_midi_out.h>
#include <seq_bpm.h>

#ifndef MBSEQV4L
#include <blm.h>
#else
#include <blm_cheapo.h>
#endif
#include <blm_scalar_master.h>
#include <ws2812.h>

#include "tasks.h"

#include "app.h"
#include "seq_lcd.h"
#include "seq_lcd_logo.h"

#include "seq_hwcfg.h"

#include "seq_core.h"
#include "seq_led.h"
#include "seq_blm8x8.h"
#include "seq_tpd.h"
#include "seq_ui.h"
#include "seq_pattern.h"
#include "seq_mixer.h"
#include "seq_song.h"
#include "seq_label.h"
#include "seq_cc_labels.h"
#include "seq_midply.h"


#include "seq_cv.h"
#include "seq_midi_port.h"
#include "seq_midi_in.h"
#include "seq_midi_router.h"
#include "seq_midi_sysex.h"
#include "seq_blm.h"
#include "seq_terminal.h"
#include "seq_statistics.h"

#include "file.h"
#include "seq_file.h"
#include "seq_file_b.h"
#include "seq_file_g.h"
#include "seq_file_c.h"
#include "seq_file_gc.h"
#include "seq_file_hw.h"
#include "seq_file_m.h"
#include "seq_file_s.h"
#include "seq_file_bm.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// global variables
/////////////////////////////////////////////////////////////////////////////
u8 app_din_testmode;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 byte);
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package);
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // disable DIN test mode by default
  app_din_testmode = 0;

#ifdef MBSEQV4L
  // MBSEQV4L: set default port to 0xc0: multiple outputs
  MIOS32_MIDI_DefaultPortSet(0xc0);
#endif

#ifndef MBSEQV4L
  // initialize CLCDs
  SEQ_LCD_Init(0);
#endif

  // init BLMs
#ifndef MBSEQV4L
  BLM_Init(0);
#else
  BLM_CHEAPO_Init(0);
#endif
  SEQ_BLM8X8_Init(0);

  WS2812_Init(0);

  // initialize hardware soft-config
  SEQ_HWCFG_Init(0);

  SEQ_TPD_Init(0);

  // initialize CV
  SEQ_CV_Init(0);

  // initialize MIDI handlers
  SEQ_MIDI_PORT_Init(0);
  SEQ_MIDI_IN_Init(0);
  SEQ_MIDI_SYSEX_Init(0);
  SEQ_BLM_Init(0);
  SEQ_MIDI_OUT_Init(0);
  SEQ_MIDI_ROUTER_Init(0);
  SEQ_TERMINAL_Init(0);

  // init mixer page
  SEQ_MIXER_Init(0);

  // init sequencer core
  SEQ_CORE_Init(0);

  // init user interface
#ifndef MBSEQV4L
  SEQ_LABEL_Init(0);
  SEQ_CC_LABELS_Init(0);
#endif
  SEQ_LED_Init(0);
  SEQ_UI_Init(0);

  // initial load of filesystem
  SEQ_FILE_Init(0);

  // start tasks (differs between MIOS32 and MacOS)
  TASKS_Init(0);

  // install MIDI Rx/Tx callback functions
  MIOS32_MIDI_DirectRxCallback_Init(&NOTIFY_MIDI_Rx);
  MIOS32_MIDI_DirectTxCallback_Init(&NOTIFY_MIDI_Tx);

  // install SysEx callback
  MIOS32_MIDI_SysExCallback_Init(APP_SYSEX_Parser);

  // install timeout callback function
  MIOS32_MIDI_TimeOutCallback_Init(&NOTIFY_MIDI_TimeOut);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // toggle the state of all LEDs (allows to measure the execution speed with a scope)
#if 0
  MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());
#endif

  // for idle time measurements
  SEQ_STATISTICS_Idle();
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  if( midi_package.evnt0 >= 0xf8 ) {
    // disabled: MIDI Clock always sent from sequencer, even in slave mode
#if 0
    // forward to router
    mios32_midi_package_t p;
    p.ALL = 0;
    p.type = 0x5; // Single-byte system common message
    p.evnt0 = sysex_byte;
    SEQ_MIDI_ROUTER_Receive(port, p);
#endif
  } else {
    if( port == BLM_SCALAR_MASTER_MIDI_PortGet(0) ) {
      BLM_SCALAR_MASTER_MIDI_Receive(port, midi_package);
    } else {
      // returns > 0 if byte has been used for remote function
      if( SEQ_UI_REMOTE_MIDI_Receive(port, midi_package) < 1 ) {
	// forward to router
	SEQ_MIDI_ROUTER_Receive(port, midi_package);
	// forward to transposer/arpeggiator/CC parser/etc...
	SEQ_MIDI_IN_Receive(port, midi_package);
      }
    }
  }

  // forward to port handler (used for MIDI monitor function)
  SEQ_MIDI_PORT_NotifyMIDIRx(port, midi_package);
}


/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MIOS32 commands
/////////////////////////////////////////////////////////////////////////////
s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // forward event to MIDI router
  SEQ_MIDI_ROUTER_ReceiveSysEx(port, midi_in);

  // forward event to BLM as well
  BLM_SCALAR_MASTER_SYSEX_Parser(port, midi_in);

  // forward to common SysEx handler
  SEQ_MIDI_SYSEX_Parser(port, midi_in);

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
  static u8 led_digit_ctr = 0;
  if( ++led_digit_ctr >= 7 )
    led_digit_ctr = 0;

#ifndef MBSEQV4L
  if( seq_hwcfg_blm.enabled ) {
    // prepare DOUT registers of BLM to drive the column
    BLM_PrepareCol();
  }
#else
  BLM_CHEAPO_PrepareCol();
#endif

  if( seq_hwcfg_blm8x8.enabled ) {
    // prepare DOUT registers of 8x8 BLM to drive the row
    SEQ_BLM8X8_PrepareRow();
  }

  // TK: using MIOS32_DOUT_SRSet/PinSet instead of SEQ_LED_SRSet/PinSet to ensure compatibility with MBSEQV4L
  if( seq_hwcfg_bpm_digits.enabled ) {
    // invert for common anodes
    u8 inversion_mask = (seq_hwcfg_bpm_digits.enabled == 2) ? 0xff : 0x00;
    u8 common_enable = (seq_hwcfg_bpm_digits.enabled == 2) ? 1 : 0;

    float bpm = SEQ_BPM_EffectiveGet();
    if( led_digit_ctr == 0 ) {
      u8 sr_value = SEQ_LED_DigitPatternGet(((int)(bpm*10)) % 10);
      MIOS32_DOUT_SRSet(seq_hwcfg_bpm_digits.segments_sr - 1, sr_value ^ inversion_mask);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common1_pin, common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common2_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common3_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common4_pin, !common_enable);
    } else if( led_digit_ctr == 1 ) {
      u8 sr_value = SEQ_LED_DigitPatternGet((int)bpm % 10) | 0x80; // +dot
      MIOS32_DOUT_SRSet(seq_hwcfg_bpm_digits.segments_sr - 1, sr_value ^ inversion_mask);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common1_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common2_pin, common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common3_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common4_pin, !common_enable);
    } else if( led_digit_ctr == 2 ) {
      u8 sr_value = SEQ_LED_DigitPatternGet(((int)bpm / 10) % 10);
      MIOS32_DOUT_SRSet(seq_hwcfg_bpm_digits.segments_sr - 1, sr_value ^ inversion_mask);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common1_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common2_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common3_pin, common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common4_pin, !common_enable);
    } else if( led_digit_ctr == 3 ) {
      u8 sr_value = SEQ_LED_DigitPatternGet(((int)bpm / 100) % 10);
      MIOS32_DOUT_SRSet(seq_hwcfg_bpm_digits.segments_sr - 1, sr_value ^ inversion_mask);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common1_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common2_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common3_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common4_pin, common_enable);
    }
    else { // not displaying bpm digit in this cycle, disable common pins
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common1_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common2_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common3_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_bpm_digits.common4_pin, !common_enable);
    }
  }

  if( seq_hwcfg_step_digits.enabled ) {
    // invert for common anodes
    u8 inversion_mask = (seq_hwcfg_step_digits.enabled == 2) ? 0xff : 0x00;
    u8 common_enable = (seq_hwcfg_step_digits.enabled == 2) ? 1 : 0;
    
    int step = (int)(SEQ_BPM_IsRunning() ? seq_core_trk[SEQ_UI_VisibleTrackGet()].step : ui_selected_step) + 1;
    if( led_digit_ctr == 4 ) {
      u8 sr_value = SEQ_LED_DigitPatternGet(step % 10);
      MIOS32_DOUT_SRSet(seq_hwcfg_step_digits.segments_sr - 1, sr_value ^ inversion_mask);
      MIOS32_DOUT_PinSet(seq_hwcfg_step_digits.common1_pin, common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_step_digits.common2_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_step_digits.common3_pin, !common_enable);
    } else if( led_digit_ctr == 5 ) {
      u8 sr_value = SEQ_LED_DigitPatternGet((step / 10) % 10);
      MIOS32_DOUT_SRSet(seq_hwcfg_step_digits.segments_sr - 1, sr_value ^ inversion_mask);
      MIOS32_DOUT_PinSet(seq_hwcfg_step_digits.common1_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_step_digits.common2_pin, common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_step_digits.common3_pin, !common_enable);
    } else if( led_digit_ctr == 6 ) {
      u8 sr_value = SEQ_LED_DigitPatternGet((step / 100) % 10);
      MIOS32_DOUT_SRSet(seq_hwcfg_step_digits.segments_sr - 1, sr_value ^ inversion_mask);
      MIOS32_DOUT_PinSet(seq_hwcfg_step_digits.common1_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_step_digits.common2_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_step_digits.common3_pin, common_enable);
    }
    else { // not displaying step digit in this cycle, disable common pins
      MIOS32_DOUT_PinSet(seq_hwcfg_step_digits.common1_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_step_digits.common2_pin, !common_enable);
      MIOS32_DOUT_PinSet(seq_hwcfg_step_digits.common3_pin, !common_enable);
    }    
  }
  
  SEQ_TPD_LED_Update();     
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
#ifndef MBSEQV4L
  if( seq_hwcfg_blm.enabled ) {
    // call the BL_GetRow function after scan is finished to capture the read DIN values
    BLM_GetRow();
  }
#else
  BLM_CHEAPO_GetRow();
#endif

  if( seq_hwcfg_blm8x8.enabled ) {
    // call the BL_X_GetRow function after scan is finished to capture the read DIN values
    SEQ_BLM8X8_GetRow();
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
  if( app_din_testmode ) {
    DEBUG_MSG("[DIN_TESTMODE] Pin %3d (SR%d:D%d) %s\n", pin, (pin>>3)+1, pin&7, pin_value ? "depressed" : "pressed");
  }

#ifndef MBSEQV4L
  SEQ_LCD_LOGO_ScreenSaver_Disable();
#endif

  // forward to UI button handler
  SEQ_UI_Button_Handler(pin, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a BLM button has been toggled
// (see also SEQ_TASK_Period1mS)
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_BLM_NotifyToggle(u32 pin, u32 pin_value)
{
#ifndef MBSEQV4L
  u8 row = pin / 16;
  u8 pin_of_row = pin % 16;
  if( app_din_testmode ) {
    DEBUG_MSG("[DIN_TESTMODE] BLM Pin M%d D%d %s\n", row+1, pin_of_row, pin_value ? "depressed" : "pressed");
  }

  // forward to UI BLM button handler
  SEQ_UI_BLM_Button_Handler(row, pin_of_row, pin_value);
#else
  if( app_din_testmode ) {
    MIOS32_MIDI_SendDebugMessage("[DIN_TESTMODE] SR%d Pin D%d %s\n", (pin>>3)+1, pin & 7, pin_value ? "depressed" : "pressed");
  }

  // forward to UI button handler
  SEQ_UI_Button_Handler(pin, pin_value);

#ifndef MBSEQV4L
  SEQ_LCD_LOGO_ScreenSaver_Disable();
#endif
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SEQ_BLM8X8 button has been toggled
// (see also SEQ_TASK_Period1mS)
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_SEQ_BLM8X8_NotifyToggle(u8 blm, u32 pin, u32 pin_value)
{
  if( app_din_testmode ) {
    DEBUG_MSG("[DIN_TESTMODE] BLM8x8%c Pin M%d D%d %s\n", 'A'+blm, (pin>>3)+1, pin&7, pin_value ? "depressed" : "pressed");
  }

#ifndef MBSEQV4L
  SEQ_LCD_LOGO_ScreenSaver_Disable();
#endif

  // extra: fake DIN pin as long as SD Card not read (dirty workaround to simplify usage of V4L)
  if( !SEQ_FILE_HW_Valid() ) {
    if( pin >= 32 ) {
      // ignore for GP pins since they will be reversed again in SEQ_UI_Button_Handler()... really dirty workaround ;-)
      // actually shows, that it would be much better to simplify the HW than adding SW based options
      if( pin < 48 || pin >= 56 ) {
	pin = (pin & 0xf8) | (7-(pin & 7));
      }
    }
    SEQ_UI_Button_Handler(pin, pin_value);
  } else {
    SEQ_UI_Button_Handler(pin + 184, pin_value);
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
  if( app_din_testmode ) {
    DEBUG_MSG("Enc %2d = %d\n", encoder, incrementer);
  }

#ifndef MBSEQV4L
  SEQ_LCD_LOGO_ScreenSaver_Disable();
#endif

  // forward to UI encoder handler
  SEQ_UI_Encoder_Handler(encoder, incrementer);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}

/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_Period1mS(void)
{
#if MEASURE_IDLE_CTR == 0
  // pattern switching if not in sync mode
  if( !seq_core_options.SYNCHED_PATTERN_CHANGE || SEQ_SONG_ActiveGet() ) {
    SEQ_PATTERN_Handler();
  }

  // update high-prio LED functions
  SEQ_UI_LED_Handler_Periodic();

  // for menu handling (e.g. flashing cursor, doubleclick counter, etc...)
  SEQ_UI_MENU_Handler_Periodic();

  // update BPM
  SEQ_CORE_BPM_SweepHandler();

#ifndef MBSEQV4L
  // Button handlers
  if( seq_hwcfg_blm.enabled ) {
    // check for BLM pin changes, call button handler of sequencer on each toggled pin
    BLM_ButtonHandler(APP_BLM_NotifyToggle);
  }
#else
  // -> BLM_CHEAPO driver
  BLM_CHEAPO_ButtonHandler(APP_BLM_NotifyToggle);
#endif

  if( seq_hwcfg_blm8x8.enabled ) {
    // check for SEQ_BLM8X8 pin changes, call button handler of sequencer on each toggled pin
    SEQ_BLM8X8_ButtonHandler(APP_SEQ_BLM8X8_NotifyToggle);
  }
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This task is called each mS with lowest priority
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_Period1mS_LowPrio(void)
{
#if MEASURE_IDLE_CTR == 0
  // call LCD Handler
  SEQ_UI_LCD_Handler();

  // update LEDs
  SEQ_UI_LED_Handler();

  // update TPD
  SEQ_TPD_Handler();

  // MIDI In/Out monitor
  SEQ_MIDI_PORT_Period1mS();

  // if remote client active: timeout handling
  if( seq_midi_sysex_remote_active_mode == SEQ_MIDI_SYSEX_REMOTE_MODE_CLIENT ) {
    ++seq_midi_sysex_remote_client_timeout_ctr;

    // request refresh from server each second
    if( (seq_midi_sysex_remote_client_timeout_ctr % 1000) == 999 ) {
      SEQ_MIDI_SYSEX_REMOTE_SendRefresh();
    } else if( seq_midi_sysex_remote_client_timeout_ctr >= 5000 ) {
      // no reply from server after 5 seconds: leave client mode
      seq_midi_sysex_remote_active_mode = SEQ_MIDI_SYSEX_REMOTE_MODE_AUTO;
#ifndef MBSEQV4L
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "No response from", "Remote Server!");
#endif
    }
  }
#endif
}

/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each second
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_Period1S(void)
{
#if MEASURE_IDLE_CTR == 0
  static s8 wait_boot_ctr = 3; // wait 3 seconds before loading from SD Card - this is to increase the time where the boot screen is print!
  u8 load_sd_content = 0;

  // poll for IIC modules as long as HW config hasn't been locked (read from SD card)
  // TODO: use proper mutex handling here
#ifndef MIOS32_FAMILY_EMULATION
  if( !SEQ_FILE_HW_ConfigLocked() ) {
    MIOS32_IIC_MIDI_ScanInterfaces();
  }
#endif  

  // boot phase of 2 seconds finished?
  if( wait_boot_ctr > 0 ) {
    --wait_boot_ctr;
    if( wait_boot_ctr )
      return;
  }

  // check if SD Card connected
  MUTEX_SDCARD_TAKE;

  s32 status = FILE_CheckSDCard();

  if( status == 1 ) {
    if( wait_boot_ctr != 0 ) { // don't print message if we just booted
      char str[21];
      sprintf(str, "Label: %s", FILE_VolumeLabel());
#ifndef MBSEQV4L
      SEQ_UI_Msg(SEQ_UI_MSG_SDCARD, 2000, " SD Card connected", "        :-D");
#endif
      DEBUG_MSG("SD Card connected: %s\n", FILE_VolumeLabel());
    }
    SEQ_FILE_LoadSessionName();
    DEBUG_MSG("Loading session %s\n", seq_file_session_name);
    SEQ_FILE_LoadAllFiles(1);
  } else if( status == 2 ) {
#ifndef MBSEQV4L
    SEQ_UI_Msg(SEQ_UI_MSG_SDCARD, 2000, "SD Card disconnected", "        :-/");
#endif
    DEBUG_MSG("SD Card disconnected\n");
    SEQ_FILE_UnloadAllFiles();
    wait_boot_ctr = -1;
  } else if( status == 3 ) {
    if( !FILE_SDCardAvailable() ) {
#ifndef MBSEQV4L
      SEQ_UI_Msg(SEQ_UI_MSG_SDCARD, 2000, "  No SD Card found  ", "        :-(");
#endif
      DEBUG_MSG("SD Card not found\n");
      SEQ_FILE_HW_LockConfig(); // lock configuration
      wait_boot_ctr = -1;
    } else if( !FILE_VolumeAvailable() ) {
#ifndef MBSEQV4L
      SEQ_UI_Msg(SEQ_UI_MSG_SDCARD, 2000, "!! SD Card Error !!!", "!! Invalid FAT !!!!!");
#endif
      DEBUG_MSG("ERROR: SD Card contains invalid FAT!\n");
      SEQ_FILE_HW_LockConfig(); // lock configuration
      wait_boot_ctr = -1;
    } else {
#ifndef MBSEQV4L
      if( wait_boot_ctr != 0 ) { // don't print message if we just booted
	char str1[30];
	sprintf(str1, "Banks: ....");
	u8 bank;
	for(bank=0; bank<4; ++bank)
	  str1[7+bank] = SEQ_FILE_B_NumPatterns(bank) ? ('1'+bank) : '-';
	char str2[30];
	sprintf(str2, 
		"M:%c S:%c G:%c C:%c%c HW:%c", 
		SEQ_FILE_M_NumMaps() ? '*':'-',
		SEQ_FILE_S_NumSongs() ? '*':'-',
		SEQ_FILE_G_Valid() ? '*':'-',
		SEQ_FILE_C_Valid() ? 'S':'-',
		SEQ_FILE_GC_Valid() ? 'G':'-',
		SEQ_FILE_HW_Valid() ? '*':'-');
	SEQ_UI_Msg(SEQ_UI_MSG_SDCARD, 2000, str1, str2);
      }
#endif

#if MBSEQV4L
      // auto-format
      // check if formatting is required
      if( SEQ_FILE_FormattingRequired() ) {
	strcpy(seq_file_new_session_name, "DEF_V4L");
	DEBUG_MSG("Creating initial session '%s'... this can take some seconds!\n", seq_file_new_session_name);

	if( (status=SEQ_FILE_Format()) < 0 ) {
	  DEBUG_MSG("Failed to create session! (status: %d)\n", status);
	} else {
	  SEQ_FILE_StoreSessionName();
	  DEBUG_MSG("Done!\n");
	}
      }
#endif

      // request to load content of SD card
      load_sd_content = 1;

      // notify that boot finished
      wait_boot_ctr = -1;
    }
  } else if( status < 0 ) {
    wait_boot_ctr = -1;
#ifndef MBSEQV4L
    SEQ_UI_SDCardErrMsg(2000, status);
#endif
    DEBUG_MSG("ERROR: SD Card Error %d (FatFs: D%3d)\n", status, file_dfs_errno);
  }

  // check for format request
  // this is running with low priority, so that LCD is updated in parallel!
  if( seq_ui_format_req ) {
    // note: request should be cleared at the end of this process to avoid double-triggers!
    if( (status = SEQ_FILE_Format()) < 0 ) {
#ifndef MBSEQV4L
      SEQ_UI_SDCardErrMsg(2000, status);
#endif
      DEBUG_MSG("ERROR: SD Card Error %d (FatFs: D%3d)\n", status, file_dfs_errno);
    } else {
#ifndef MBSEQV4L
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Files created", "successfully!");
#endif
      DEBUG_MSG("Files created successfully!\n");

      // store session name
      status |= SEQ_FILE_StoreSessionName();
    }

    // request to load content of SD card
    load_sd_content = 1;

    // finally clear request
    seq_ui_format_req = 0;
  }

  // check for backup request
  // this is running with low priority, so that LCD is updated in parallel!
  if( seq_ui_backup_req ) {
    // note: request should be cleared at the end of this process to avoid double-triggers!
    status = SEQ_FILE_CreateBackup();
      
    if( status < 0 ) {
      if( status == FILE_ERR_COPY ) {
#ifndef MBSEQV4L
	SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "COPY FAILED!", "ERROR :-(");
#endif
	DEBUG_MSG("ERROR: copy failed!\n");
      } else {
#ifndef MBSEQV4L
	SEQ_UI_SDCardErrMsg(2000, status);
#endif
	DEBUG_MSG("ERROR: SD Card Error %d (FatFs: D%3d)\n", status, file_dfs_errno);
      }
    }
    else {
#ifndef MBSEQV4L
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Files copied", "successfully!");
#endif
      DEBUG_MSG("Files copied successfully!\n");

      // store session name
      status |= SEQ_FILE_StoreSessionName();
    }

    // finally clear request
    seq_ui_backup_req = 0;
  }

  // check for save all request
  // this is running with low priority, so that LCD is updated in parallel!
  if( seq_ui_saveall_req ) {
    s32 status = 0;

    // store all patterns
    int group;
    for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group)
      status |= SEQ_FILE_B_PatternWrite(seq_file_session_name, seq_pattern[group].bank, seq_pattern[group].pattern, group, 1);

    // store config (e.g. to store current song/mixermap/pattern numbers
    SEQ_FILE_C_Write(seq_file_session_name);

    // store global config
    SEQ_FILE_GC_Write();

    // store mixer map
    SEQ_MIXER_Save(SEQ_MIXER_NumGet());

    // store session name
    if( status >= 0 )
      status |= SEQ_FILE_StoreSessionName();

    if( status < 0 ) {
#ifndef MBSEQV4L
      SEQ_UI_SDCardErrMsg(2000, status);
#endif
      DEBUG_MSG("ERROR: SD Card Error %d (FatFs: D%3d)\n", status, file_dfs_errno);
    }

    // finally clear request
    seq_ui_saveall_req = 0;
  }

  MUTEX_SDCARD_GIVE;

  // load content of SD card if requested ((re-)connection detected)
  if( load_sd_content && !SEQ_FILE_FormattingRequired() ) {
    // send layout request to MBHP_BLM_SCALAR
    BLM_SCALAR_MASTER_SendRequest(0, 0x00);

    // TODO: should we load the patterns when SD Card has been detected?
    // disadvantage: current edit patterns are destroyed - this could be fatal during a live session if there is a bad contact!

    SEQ_MIXER_Load(SEQ_MIXER_NumGet());
    SEQ_SONG_Load(SEQ_SONG_NumGet());
  }

#ifndef MBSEQV4L
  SEQ_LCD_LOGO_ScreenSaver_Period1S();
#endif
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS as well
// it handles sequencer and MIDI events
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_MIDI(void)
{
#if MEASURE_IDLE_CTR == 0
  MUTEX_MIDIOUT_TAKE;

  // execute sequencer handler
  SEQ_CORE_Handler();

  // send timestamped MIDI events
  SEQ_MIDI_OUT_Handler();

  // update CV and gates
  SEQ_CV_Update();

  MUTEX_MIDIOUT_GIVE;
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectRxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 midi_byte)
{
  // filter MIDI In port which controls the MIDI clock
  if( SEQ_MIDI_ROUTER_MIDIClockInGet(port) == 1 )
    SEQ_BPM_NotifyMIDIRx(midi_byte);

  return 0; // no error, no filtering
}


/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectTxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package)
{
  return SEQ_MIDI_PORT_NotifyMIDITx(port, package);
}


/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_TimeoutCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port)
{  
  // forward to SysEx parser
  SEQ_MIDI_SYSEX_TimeOut(port);

  // forward timeout to BLM MASTER
  BLM_SCALAR_MASTER_MIDI_TimeOut(port);

#ifndef MBSEQV4L
  // print message on screen
  SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "MIDI Protocol", "TIMEOUT !!!");
#endif

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Send a Mutex protected debug messages (referenced by DEBUG_MSG macro)
/////////////////////////////////////////////////////////////////////////////
void APP_SendDebugMessage(char *format, ...)
{
  MUTEX_MIDIOUT_TAKE;

  {
    char str[128]; // 128 chars allowed
    va_list args;

    // failsave: if format string is longer than 100 chars, break here
    // note that this is a weak protection: if %s is used, or a lot of other format tokens,
    // the resulting string could still lead to a buffer overflow
    // other the other hand we don't want to allocate too many byte for buffer[] to save stack
    if( strlen(format) > 100 ) {
      // exit with less costly message
      MIOS32_MIDI_SendDebugString("(ERROR: string passed to MIOS32_MIDI_SendDebugMessage() is longer than 100 chars!\n");
    } else {
      // transform formatted string into string
      va_start(args, format);
      vsprintf(str, format, args);
    }

    u32 len = strlen(str);
    u8 *str_ptr = (u8 *)str;
    int i;
    for(i=0; i<len; ++i) {
      *str_ptr++ &= 0x7f; // ensure that MIDI protocol won't be violated
    }

    MIOS32_MIDI_SendDebugString(str);
  }

  MUTEX_MIDIOUT_GIVE;
}

