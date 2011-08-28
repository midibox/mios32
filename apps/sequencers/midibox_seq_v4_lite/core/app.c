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

#include <seq_midi_out.h>
#include <seq_bpm.h>

#include <blm_cheapo.h>

#include "tasks.h"

#include "app.h"

#include "seq_hwcfg.h"

#include "seq_core.h"
#include "seq_ui.h"
#include "seq_pattern.h"

#include "seq_cv.h"
#include "seq_midi_port.h"
#include "seq_midi_in.h"
#include "seq_midi_router.h"
#include "seq_midi_sysex.h"
#include "seq_terminal.h"
#include "seq_statistics.h"

#include "file.h"
#include "seq_file.h"
#include "seq_file_b.h"
#include "seq_file_g.h"
#include "seq_file_gc.h"
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

  // initialize cheapo BLM
  BLM_CHEAPO_Init(0);

  // initialize hardware soft-config
  SEQ_HWCFG_Init(0);

  // initialize CV
  SEQ_CV_Init(0);

  // initialize MIDI handlers
  SEQ_MIDI_PORT_Init(0);
  SEQ_MIDI_IN_Init(0);
  SEQ_MIDI_SYSEX_Init(0);
  SEQ_MIDI_OUT_Init(0);
  SEQ_MIDI_ROUTER_Init(0);
  SEQ_TERMINAL_Init(0);

  // init sequencer core
  SEQ_CORE_Init(0);

  // init user interface
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
    // forward to router
    SEQ_MIDI_ROUTER_Receive(port, midi_package);
    // forward to transposer/arpeggiator/CC parser/etc...
    SEQ_MIDI_IN_Receive(port, midi_package);
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

  // forward to common SysEx handler
  SEQ_MIDI_SYSEX_Parser(port, midi_in);

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
  // ignore as long as hardware config hasn't been read
  if( !SEQ_FILE_HW_ConfigLocked() )
    return;

  // -> BLM_CHEAPO driver
  BLM_CHEAPO_PrepareCol();
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
  // ignore as long as hardware config hasn't been read
  if( !SEQ_FILE_HW_ConfigLocked() )
    return;

  // -> BLM_CHEAPO driver
  BLM_CHEAPO_GetRow();
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
// This task handles the buttons
/////////////////////////////////////////////////////////////////////////////
void APP_BLM_CHEAPO_NotifyToggle(u32 pin, u32 pin_value)
{
#if 0
  MIOS32_MIDI_SendDebugMessage("SR: %d  Pin:%d  Value:%d\n", (pin>>3)+1, pin & 7, pin_value);
#endif

  // forward to UI button handler
  SEQ_UI_Button_Handler(pin, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
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
  // -> BLM_CHEAPO driver
  BLM_CHEAPO_ButtonHandler(APP_BLM_CHEAPO_NotifyToggle);

  // update high-prio LED functions
  SEQ_UI_LED_Handler_Periodic();

  // for menu handling (e.g. flashing cursor, doubleclick counter, etc...)
  SEQ_UI_MENU_Handler_Periodic();

  // update BPM
  SEQ_CORE_BPM_SweepHandler();
}


/////////////////////////////////////////////////////////////////////////////
// This task is called each mS with lowest priority
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_Period1mS_LowPrio(void)
{
  // update LEDs
  SEQ_UI_LED_Handler();

  // MIDI In/Out monitor
  SEQ_MIDI_PORT_Period1mS();
}

/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each second
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_Period1S(void)
{
  // don't check for SD Card if MSD enabled
  if( TASK_MSD_EnableGet() > 0 )
    return;

  MUTEX_SDCARD_TAKE;
  s32 status = FILE_CheckSDCard();

  if( status == 1 ) {
    DEBUG_MSG("SD Card connected: %s\n", FILE_VolumeLabel());
    // load all file infos
    SEQ_FILE_LoadAllFiles(1); // including HW info
  } else if( status == 2 ) {
    DEBUG_MSG("SD Card disconnected\n");
    // invalidate all file infos
    SEQ_FILE_UnloadAllFiles();
  } else if( status == 3 ) {
    if( !FILE_SDCardAvailable() ) {
      DEBUG_MSG("SD Card not found\n");
      SEQ_FILE_HW_LockConfig(); // lock configuration
    } else if( !FILE_VolumeAvailable() ) {
      DEBUG_MSG("ERROR: SD Card contains invalid FAT!\n");
      SEQ_FILE_HW_LockConfig(); // lock configuration
    } else {
#if 0
      // check if patch file exists
      if( !MIDIO_FILE_P_Valid() ) {
	// create new one
	DEBUG_MSG("Creating initial DEFAULT.MIO file\n");
	    
	if( (status=MIDIO_FILE_P_Write("DEFAULT")) < 0 ) {
	  DEBUG_MSG("Failed to create file! (status: %d)\n", status);
	}
      }
#endif

      DEBUG_MSG("SD Card found\n");
    }
  }

  MUTEX_SDCARD_GIVE;
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS as well
// it handles sequencer and MIDI events
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_MIDI(void)
{
  MUTEX_MIDIOUT_TAKE;

  // execute sequencer handler
  SEQ_CORE_Handler();

  // send timestamped MIDI events
  SEQ_MIDI_OUT_Handler();

  // update CV and gates
  SEQ_CV_Update();

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

  return 0;
}
