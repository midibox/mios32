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

#include <aout.h>
#include <seq_midi_out.h>
#include <seq_bpm.h>
#include <blm.h>
#include <blm_x.h>

#include "tasks.h"

#include "app.h"

#include "seq_hwcfg.h"

#include "seq_core.h"
#include "seq_led.h"
#include "seq_ui.h"
#include "seq_pattern.h"
#include "seq_mixer.h"
#include "seq_song.h"
#include "seq_label.h"

#include "seq_midply.h"

#include "seq_midi_port.h"
#include "seq_midi_in.h"
#include "seq_midi_router.h"
#include "seq_midi_sysex.h"

#include "seq_file.h"
#include "seq_file_b.h"
#include "seq_file_m.h"
#include "seq_file_s.h"
#include "seq_file_g.h"
#include "seq_file_c.h"
#include "seq_file_hw.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0


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

  // initialize hardware soft-config
  SEQ_HWCFG_Init(0);

  // initialize second CLCD
  MIOS32_LCD_DeviceSet(1);
  MIOS32_LCD_Init(0);
  MIOS32_LCD_DeviceSet(0);

  // init BLMs
  BLM_Init(0);
  BLM_X_Init();

  // initialize AOUT driver
  AOUT_Init(0);

  // initialize MIDI handlers
  SEQ_MIDI_PORT_Init(0);
  SEQ_MIDI_IN_Init(0);
  SEQ_MIDI_SYSEX_Init(0);
  SEQ_MIDI_OUT_Init(0);
  SEQ_MIDI_ROUTER_Init(0);

  // init mixer page
  SEQ_MIXER_Init(0);

  // init sequencer core
  SEQ_CORE_Init(0);

  // init user interface
  SEQ_LABEL_Init(0);
  SEQ_CC_LABELS_Init(0);
  SEQ_LED_Init(0);
  SEQ_UI_Init(0);

  // initial load of filesystem
  SEQ_FILE_Init(0);

  // start tasks (differs between MIOS32 and MacOS)
  TASKS_Init(0);

  // install MIDI Rx/Tx callback functions
  MIOS32_MIDI_DirectRxCallback_Init(NOTIFY_MIDI_Rx);
  MIOS32_MIDI_DirectTxCallback_Init(NOTIFY_MIDI_Tx);

  // install timeout callback function
  MIOS32_MIDI_TimeOutCallback_Init(NOTIFY_MIDI_TimeOut);
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
  SEQ_UI_INFO_Idle();
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
    // returns > 0 if byte has been used for remote function
    if( SEQ_UI_REMOTE_MIDI_Receive(port, midi_package) < 1 ) {
      // forward to router
      SEQ_MIDI_ROUTER_Receive(port, midi_package);
      // forward to transposer/arpeggiator/CC parser/etc...
      SEQ_MIDI_IN_Receive(port, midi_package);
    }
  }

  // forward to port handler (used for MIDI monitor function)
  SEQ_MIDI_PORT_NotifyMIDIRx(port, midi_package);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
  if( seq_hwcfg_blm.enabled ) {
    // prepare DOUT registers of BLM to drive the column
    BLM_PrepareCol();
  }

  if( seq_hwcfg_blm8x8.enabled ) {
    // prepare DOUT registers of 8x8 BLM to drive the row
    BLM_X_PrepareRow();
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
  if( seq_hwcfg_blm.enabled ) {
    // call the BL_GetRow function after scan is finished to capture the read DIN values
    BLM_GetRow();
  }

  if( seq_hwcfg_blm8x8.enabled ) {
    // call the BL_X_GetRow function after scan is finished to capture the read DIN values
    BLM_X_GetRow();
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("Pin %3d (SR%d:D%d) = %d\n", pin, (pin>>3)+1, pin&7, pin_value);
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
  u8 row = pin / 16;
  u8 pin_of_row = pin % 16;
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("BLM Pin %3d (Row%d:D%d) = %d\n", pin, row, pin_of_row, pin_value);
#endif

  // forward to UI BLM button handler
  SEQ_UI_BLM_Button_Handler(row, pin_of_row, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a BLM_X button has been toggled
// (see also SEQ_TASK_Period1mS)
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_BLM_X_NotifyToggle(u32 pin, u32 pin_value)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("BLM8x8 Pin %3d (SR%d:D%d) = %d\n", pin, (pin>>3)+1, pin&7, pin_value);
#endif

  // forward to UI button handler
  SEQ_UI_Button_Handler(pin + 128, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("Enc %2d = %d\n", encoder, incrementer);
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
  // update high-prio LED functions
  SEQ_UI_LED_Handler_Periodic();

  // for menu handling (e.g. flashing cursor, doubleclick counter, etc...)
  SEQ_UI_MENU_Handler_Periodic();

  // update BPM
  SEQ_CORE_BPM_SweepHandler();

  // Button handlers
  if( seq_hwcfg_blm.enabled ) {
    // check for BLM pin changes, call button handler of sequencer on each toggled pin
    BLM_ButtonHandler(APP_BLM_NotifyToggle);
  }

  if( seq_hwcfg_blm8x8.enabled ) {
    // check for BLM_X pin changes, call button handler of sequencer on each toggled pin
    BLM_X_BtnHandler(APP_BLM_X_NotifyToggle);
  }
}


/////////////////////////////////////////////////////////////////////////////
// This task is called each mS with lowest priority
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_Period1mS_LowPrio(void)
{
  // call LCD Handler
  SEQ_UI_LCD_Handler();

  // update LEDs
  SEQ_UI_LED_Handler();

  // MIDI In/Out monitor
  SEQ_MIDI_PORT_Period1mS();

  // if remote client active: timeout handling
  if( seq_ui_remote_active_mode == SEQ_UI_REMOTE_MODE_CLIENT ) {
    ++seq_ui_remote_client_timeout_ctr;

    // request refresh from server each second
    if( (seq_ui_remote_client_timeout_ctr % 1000) == 999 ) {
      SEQ_MIDI_SYSEX_REMOTE_SendRefresh();
    } else if( seq_ui_remote_client_timeout_ctr >= 5000 ) {
      // no reply from server after 5 seconds: leave client mode
      seq_ui_remote_active_mode = SEQ_UI_REMOTE_MODE_AUTO;
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "No response from", "Remote Server!");
    }
  }

}

/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each second
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_Period1S(void)
{
  static u8 wait_boot_ctr = 2; // wait 2 seconds before loading from SD Card - this is to increase the time where the boot screen is print!
  u8 load_sd_content = 0;

  // don't check for SD Card if MSD enabled
  if( TASK_MSD_EnableGet() > 0 )
    return;

  // poll for IIC modules so long HW config hasn't been locked (read from SD card)
  // TODO: use proper mutex handling here
#ifndef MIOS32_FAMILY_EMULATION
  if( !SEQ_FILE_HW_ConfigLocked() ) {
    MIOS32_IIC_MIDI_ScanInterfaces();
  }
#endif  

  // boot phase of 2 seconds finished?
  if( wait_boot_ctr ) {
    --wait_boot_ctr;
    return;
  }

  // check if SD Card connected
  MUTEX_SDCARD_TAKE;

  s32 status = SEQ_FILE_CheckSDCard();

  if( status == 1 ) {
    char str[21];
    sprintf(str, "Label: %s", SEQ_FILE_VolumeLabel());
    SEQ_UI_Msg(SEQ_UI_MSG_SDCARD, 2000, " SD Card connected", "        :-D");
  } else if( status == 2 ) {
    SEQ_UI_Msg(SEQ_UI_MSG_SDCARD, 2000, "SD Card disconnected", "        :-/");
  } else if( status == 3 ) {
    if( !SEQ_FILE_SDCardAvailable() ) {
      SEQ_UI_Msg(SEQ_UI_MSG_SDCARD, 2000, "  No SD Card found  ", "        :-(");
      SEQ_FILE_HW_LockConfig(); // lock configuration
    } else if( !SEQ_FILE_VolumeAvailable() ) {
      SEQ_UI_Msg(SEQ_UI_MSG_SDCARD, 2000, "!! SD Card Error !!!", "!! Invalid FAT !!!!!");
      SEQ_FILE_HW_LockConfig(); // lock configuration
    } else {
      char str1[21];
      sprintf(str1, "Banks: ........");
      u8 bank;
      for(bank=0; bank<8; ++bank)
	str1[7+bank] = SEQ_FILE_B_NumPatterns(bank) ? ('1'+bank) : '-';
      char str2[21];
      sprintf(str2, 
	      "M:%d S:%d G:%d C:%d HW:%d", 
	      SEQ_FILE_M_NumMaps() ? 1 : 0, 
	      SEQ_FILE_S_NumSongs() ? 1 : 0, 
	      SEQ_FILE_G_Valid(),
	      SEQ_FILE_C_Valid(),
	      SEQ_FILE_HW_Valid());
      SEQ_UI_Msg(SEQ_UI_MSG_SDCARD, 2000, str1, str2);

      // request to load content of SD card
      load_sd_content = 1;
    }
  } else if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
  }

  // check for format request
  // this is running with low priority, so that LCD is updated in parallel!
  if( seq_ui_format_req ) {
    // note: request should be cleared at the end of this process to avoid double-triggers!
    if( (status = SEQ_FILE_Format()) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    else
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Files formatted", "successfully!");

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
      switch( status ) {
        case SEQ_FILE_ERR_NO_BACKUP_DIR:
	  SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "Please create", "backup/ dirs!");
	  break;

        case SEQ_FILE_ERR_NO_BACKUP_SUBDIR:
	  SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "Please create", "backup/ subdirs!");
	  break;

        case SEQ_FILE_ERR_NEED_MORE_BACKUP_SUBDIRS:
	  SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "Please create more", "backup/ subdirs!");
	  break;

        case SEQ_FILE_ERR_COPY:
	  SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "COPY FAILED!", "ERROR :-(");
	  break;

        default:
	  SEQ_UI_SDCardErrMsg(2000, status);
      }
    }
    else
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Backup created", "successfully!");

    // finally clear request
    seq_ui_backup_req = 0;
  }

  MUTEX_SDCARD_GIVE;

  // load content of SD card if requested ((re-)connection detected)
  if( load_sd_content && !SEQ_FILE_FormattingRequired() ) {
    // TODO: should we load the patterns when SD Card has been detected?
    // disadvantage: currently edited patterns are destroyed - this could be fatal during a live session if there is a bad contact!

    if( SEQ_MIXER_Load(SEQ_MIXER_NumGet()) < 0 ) // function prints error message on error
      return;

    if( SEQ_SONG_Load(SEQ_SONG_NumGet()) < 0 ) // function prints error message on error
      return;
  }
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS as well
// it handles sequencer and MIDI events
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_MIDI(void)
{
  static u8 last_gates = 0;
  static u8 last_start_stop = 0;

  MUTEX_MIDIOUT_TAKE;

  // execute sequencer handler
  SEQ_CORE_Handler();

  // send timestamped MIDI events
  SEQ_MIDI_OUT_Handler();

  // Start/Stop at J5C.A9
  u8 start_stop = SEQ_BPM_IsRunning();
  if( start_stop != last_start_stop ) {
    last_start_stop = start_stop;
    MIOS32_BOARD_J5_PinSet(9, start_stop);
  }

  // DIN Sync Pulse at J5C.A8
  if( seq_core_din_sync_pulse_ctr > 1 ) {
    MIOS32_BOARD_J5_PinSet(8, 1);
    --seq_core_din_sync_pulse_ctr;
  } else if( seq_core_din_sync_pulse_ctr == 1 ) {
    MIOS32_BOARD_J5_PinSet(8, 0);
    seq_core_din_sync_pulse_ctr = 0;
  }

  // update AOUTs
  AOUT_Update();

  // update J5 Outputs (forwarding AOUT digital pins for modules which don't support gates)
  // The MIOS32_BOARD_* function won't forward pin states if J5_ENABLED was set to 0
  u8 gates = AOUT_DigitalPinsGet();
  if( gates != last_gates ) {
    int i;

    last_gates = gates;
    for(i=0; i<8; ++i) {
      MIOS32_BOARD_J5_PinSet(i, gates & 1);
      gates >>= 1;
    }
  }

  MUTEX_MIDIOUT_GIVE;
}


/////////////////////////////////////////////////////////////////////////////
// This task is triggered whenever a pattern has to be loaded
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_Pattern(void)
{
  SEQ_PATTERN_Handler();
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

  // print message on screen
  SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "MIDI Protocol", "TIMEOUT !!!");

  return 0;
}


