// $Id$
/*
 * MIDIbox CV V2
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
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
#include "tasks.h"
#include "app.h"
#include "MbCvEnvironment.h"

#include <file.h>

// include source of the SCS
#include <scs.h>
#include "scs_config.h"

#include <seq_bpm.h>
#include <seq_midi_out.h>

#include <midi_port.h>
#include <midi_router.h>
#include <midimon.h>

// quick&dirty to simplify re-use of C modules without changing header files
extern "C" {
#include "mbcv_sysex.h"
#include "mbcv_patch.h"
#include "mbcv_map.h"

#include "mbcv_file.h"
#include "mbcv_file_p.h"
#include "mbcv_file_b.h"

#include "terminal.h"

#include "uip_task.h"
#include "osc_client.h"
}

/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 2

// measure performance with the stopwatch
//#define STOPWATCH_PERFORMANCE_MEASURING 2
#define STOPWATCH_PERFORMANCE_MEASURING 1


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////
static u8 hw_enabled;

static u32 ms_counter;

static u32 stopwatch_value;
static u32 stopwatch_value_max;
static u32 stopwatch_value_accumulated;

static u32 cv_se_speed_factor;

/////////////////////////////////////////////////////////////////////////////
// C++ objects
/////////////////////////////////////////////////////////////////////////////
MbCvEnvironment mbCvEnvironment;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void APP_Periodic_100uS(void);
extern void CV_TIMER_SE_Update(void);
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 byte);
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package);
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port);



/////////////////////////////////////////////////////////////////////////////
// returns pointer to MbCv objects
/////////////////////////////////////////////////////////////////////////////
MbCvEnvironment *APP_GetEnv()
{
  return &mbCvEnvironment;
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // hardware will be enabled once configuration has been loaded from SD Card
  // (resp. no SD Card is available)
  hw_enabled = 0;

  // init Stopwatch
  APP_StopwatchInit();

  // initialize all J10 pins as inputs with internal Pull-Up
  int pin;
  for(pin=0; pin<8; ++pin)
    MIOS32_BOARD_J10_PinInit(pin, MIOS32_BOARD_PIN_MODE_INPUT_PU);


  // install SysEx callback
  MIOS32_MIDI_SysExCallback_Init(APP_SYSEX_Parser);

  // install MIDI Rx/Tx callback functions
  MIOS32_MIDI_DirectRxCallback_Init(&NOTIFY_MIDI_Rx);
  MIOS32_MIDI_DirectTxCallback_Init(&NOTIFY_MIDI_Tx);

  // install timeout callback function
  MIOS32_MIDI_TimeOutCallback_Init(&NOTIFY_MIDI_TimeOut);

  // initialize AOUT module
  AOUT_Init(0);

  // configure interface (will be changed again once config file has been loaded from SD Card, or via SCS)
  aout_config_t config;
  config = AOUT_ConfigGet();
  config.if_type = AOUT_IF_MAX525;
  config.if_option = 0;
  config.num_channels = 8;
  config.chn_inverted = 0;
  AOUT_ConfigSet(config);
  AOUT_IF_Init(0);

  // initialize code modules
  MIDI_PORT_Init(0);
  MBCV_SYSEX_Init(0);
  MBCV_MAP_Init(0);
  MIDI_ROUTER_Init(0);
  MBCV_PATCH_Init(0);
  UIP_TASK_Init(0);
  SCS_Init(0);
  SCS_CONFIG_Init(0);
  TERMINAL_Init(0);
  MIDIMON_Init(0);
  MBCV_FILE_Init(0);
  SEQ_MIDI_OUT_Init(0);

  // install timer function which is called each 100 uS
  MIOS32_TIMER_Init(1, 100, APP_Periodic_100uS, MIOS32_IRQ_PRIO_MID);

  // initialize MbCvEnvironment
  cv_se_speed_factor = 10;
  mbCvEnvironment.updateSpeedFactorSet(cv_se_speed_factor);

  // initialize tasks
  TASKS_Init(0);

  // start timer
  // TODO: increase  once performance has been evaluated
  MIOS32_TIMER_Init(2, 2000 / cv_se_speed_factor, CV_TIMER_SE_Update, MIOS32_IRQ_PRIO_MID);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_Background(void)
{
  // endless loop
  while( 1 ) {
    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // -> CV MIDI handler
  mbCvEnvironment.midiReceive(port, midi_package);

  // -> MIDI Router
  MIDI_ROUTER_Receive(port, midi_package);

  // -> MIDI Port Handler (used for MIDI monitor function)
  MIDI_PORT_NotifyMIDIRx(port, midi_package);

  // forward to MIDI Monitor
  // SysEx messages have to be filtered for USB0 and UART0 to avoid data corruption
  // (the SysEx stream would interfere with monitor messages)
  u8 filter_sysex_message = (port == USB0) || (port == UART0);
  MIDIMON_Receive(port, midi_package, ms_counter, filter_sysex_message);
}

/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MIOS32 commands
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // -> MBCV
  MBCV_SYSEX_Parser(port, midi_in);

  // -> MIDI Router
  MIDI_ROUTER_ReceiveSysEx(port, midi_in);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_SRIO_ServicePrepare(void)
{
  // pass current pin state of J10
  // only available for LPC17xx, all others (like STM32) default to SRIO
  SCS_AllPinsSet(0xff00 | MIOS32_BOARD_J10_Get());

  // update encoders/buttons of SCS
  SCS_EncButtonUpdate_Tick();
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
  // pass encoder event to SCS handler
  if( encoder == SCS_ENC_MENU_ID )
    SCS_ENC_MENU_NotifyChange(incrementer);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}



/////////////////////////////////////////////////////////////////////////////
// This timer interrupt periodically calls the sound engine update
/////////////////////////////////////////////////////////////////////////////
extern void CV_TIMER_SE_Update(void)
{
  if( !hw_enabled )
    return;

#if STOPWATCH_PERFORMANCE_MEASURING >= 1
  APP_StopwatchReset();
#endif

  if( !mbCvEnvironment.tick() )
    return; // no update required

#if STOPWATCH_PERFORMANCE_MEASURING == 1
  APP_StopwatchCapture();
#endif

  // update AOUTs
  MBCV_MAP_Update();

#if STOPWATCH_PERFORMANCE_MEASURING == 2
  APP_StopwatchCapture();
#endif
}




/////////////////////////////////////////////////////////////////////////////
// This task handles the control surface
/////////////////////////////////////////////////////////////////////////////
void APP_TASK_Period_1mS_LP(void)
{
  static u16 performance_print_ctr = 0;

  // call SCS handler
  SCS_Tick();

  // MIDI In/Out monitor
  MIDI_PORT_Period1mS();

  // output and reset current stopwatch max value each second
  if( ++performance_print_ctr >= 1000 ) {
    performance_print_ctr = 0;

    MUTEX_MIDIOUT_TAKE;
    MIOS32_IRQ_Disable();
    u32 max_value = stopwatch_value_max;
    stopwatch_value_max = 0;
    u32 acc_value = stopwatch_value_accumulated;
    stopwatch_value_accumulated = 0;
    MIOS32_IRQ_Enable();
#if DEBUG_VERBOSE_LEVEL >= 2
    if( acc_value || max_value )
      DEBUG_MSG("%d uS (max: %d uS)\n", acc_value / (1000000 / (2000/cv_se_speed_factor)), max_value);
#endif
    MUTEX_MIDIOUT_GIVE;
  }

}


/////////////////////////////////////////////////////////////////////////////
// This task handles the SD Card
/////////////////////////////////////////////////////////////////////////////
void APP_TASK_Period_1mS_SD(void)
{
  static u16 sdcard_check_ctr = 0;

  // each second: check if SD Card (still) available
  if( ++sdcard_check_ctr >= 1000 ) {
    sdcard_check_ctr = 0;

    MUTEX_SDCARD_TAKE;
    s32 status = FILE_CheckSDCard();

    hw_enabled = 1; // enable hardware after first read...

    if( status == 1 ) {
      DEBUG_MSG("SD Card connected: %s\n", FILE_VolumeLabel());
      // load all file infos
      MBCV_FILE_LoadAllFiles(1); // including HW info
    } else if( status == 2 ) {
      DEBUG_MSG("SD Card disconnected\n");
      // invalidate all file infos
      MBCV_FILE_UnloadAllFiles();

      // change status
      MBCV_FILE_StatusMsgSet("No SD Card");
    } else if( status == 3 ) {
      if( !FILE_SDCardAvailable() ) {
	DEBUG_MSG("SD Card not found\n");
	MBCV_FILE_StatusMsgSet("No SD Card");
      } else if( !FILE_VolumeAvailable() ) {
	DEBUG_MSG("ERROR: SD Card contains invalid FAT!\n");
	MBCV_FILE_StatusMsgSet("No FAT");
      } else {
	// create the default files if they don't exist on SD Card
	MBCV_FILE_CreateDefaultFiles();

	// disable status message and print patch
	MBCV_FILE_StatusMsgSet(NULL);
      }
    }
    
    MUTEX_SDCARD_GIVE;
  }
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS
/////////////////////////////////////////////////////////////////////////////
void APP_TASK_Period_1mS(void)
{
  // e.g. for synched pattern changes
  mbCvEnvironment.tick_1mS();

  if( hw_enabled ) {
    // nothing to do... yet
  }
}


/////////////////////////////////////////////////////////////////////////////
// This timer function is periodically called each 100 uS
/////////////////////////////////////////////////////////////////////////////
static void APP_Periodic_100uS(void)
{
  static u8 pre_ctr = 0;

  // increment the microsecond counter each 10th tick
  if( ++pre_ctr >= 10 ) {
    pre_ctr = 0;
    ++ms_counter;
  }

  // here we could do some additional high-prio jobs
  // (e.g. PWM LEDs)
}

/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectRxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 midi_byte)
{
  // filter MIDI In port which controls the MIDI clock
  if( MIDI_ROUTER_MIDIClockInGet(port) == 1 )
    SEQ_BPM_NotifyMIDIRx(midi_byte);

  // TODO: better port filtering!
  if( port < USB1 || port >= UART0 ) {
    if( midi_byte >= 0xf8 ) {
      mbCvEnvironment.midiReceiveRealTimeEvent(port, midi_byte);
    }
  }

  return 0; // no error, no filtering
}

/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectTxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package)
{
  return MIDI_PORT_NotifyMIDITx(port, package);
}

/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_TimeoutCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port)
{  
  // forward to SysEx parser
  MBCV_SYSEX_TimeOut(port);

  // print message on screen
  SCS_Msg(SCS_MSG_L, 2000, "MIDI Protocol", "TIMEOUT !!!");

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// For performance Measurements
/////////////////////////////////////////////////////////////////////////////
s32 APP_StopwatchInit(void)
{
  stopwatch_value = 0;
  stopwatch_value_max = 0;
  stopwatch_value_accumulated = 0;
  return MIOS32_STOPWATCH_Init(1); // 1 uS resolution
}

s32 APP_StopwatchReset(void)
{
  return MIOS32_STOPWATCH_Reset();
}

s32 APP_StopwatchCapture(void)
{
  u32 value = MIOS32_STOPWATCH_ValueGet();
  MIOS32_IRQ_Disable();
  stopwatch_value = value;
  if( value > stopwatch_value_max )
    stopwatch_value_max = value;
  stopwatch_value_accumulated += value;
  MIOS32_IRQ_Enable();

  return 0; // no error
}
