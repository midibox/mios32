// $Id$
/*
 * MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

// prints execution time on each received event
#define DEBUG_EVENT_HANDLER_PERFORMANCE 0

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <msd.h>
#include "app.h"
#include "tasks.h"

#include <buflcd.h>

#include <ainser.h>
#include <midi_port.h>
#include <midi_router.h>
#include <midimon.h>

#include "mbng_sysex.h"
#include "mbng_patch.h"
#include "mbng_event.h"
#include "mbng_lcd.h"
#include "mbng_din.h"
#include "mbng_dout.h"
#include "mbng_enc.h"
#include "mbng_ain.h"
#include "mbng_matrix.h"

// include source of the SCS
#include <scs.h>
#include "scs_config.h"

#include "file.h"
#include "mbng_file.h"
#include "mbng_file_c.h"
#include "mbng_file_l.h"

#include <seq_bpm.h>
#include <seq_midi_out.h>

#include "terminal.h"

#include "uip_task.h"
#include "osc_client.h"


// define priority level for sequencer
// use same priority as MIOS32 specific tasks
#define PRIORITY_TASK_PERIOD_1mS ( tskIDLE_PRIORITY + 3 )

// local prototype of the task function
static void TASK_Period_1mS(void *pvParameters);

// define priority level for control surface handler
// use lower priority as MIOS32 specific tasks (2), so that slow LCDs don't affect overall performance
#define PRIORITY_TASK_PERIOD_1mS_LP ( tskIDLE_PRIORITY + 2 )

// SD Card with lower priority
#define PRIORITY_TASK_PERIOD_1mS_SD ( tskIDLE_PRIORITY + 2 )

// local prototype of the task function
static void TASK_Period_1mS_LP(void *pvParameters);
static void TASK_Period_1mS_SD(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  MSD_DISABLED,
  MSD_INIT,
  MSD_READY,
  MSD_SHUTDOWN
} msd_state_t;


/////////////////////////////////////////////////////////////////////////////
// global variables
/////////////////////////////////////////////////////////////////////////////
u8 debug_verbose_level;

/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////
static u8 hw_enabled;

// for mutual exclusive SD Card access between different tasks
// The mutex is handled with MUTEX_SDCARD_TAKE and MUTEX_SDCARD_GIVE
// macros inside the application, which contain a different implementation 
// for emulation
xSemaphoreHandle xSDCardSemaphore;

// Mutex for MIDI IN/OUT handler
xSemaphoreHandle xMIDIINSemaphore;
xSemaphoreHandle xMIDIOUTSemaphore;


static u32 ms_counter;

static volatile msd_state_t msd_state;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void APP_Periodic_100uS(void);
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

  // initialize stopwatch for measuring delays
  MIOS32_STOPWATCH_Init(100);

  // only print error messages by default
  debug_verbose_level = DEBUG_VERBOSE_LEVEL_ERROR;

  // disable MSD by default (has to be enabled in SHIFT menu)
  msd_state = MSD_DISABLED;

  // hardware will be enabled once configuration has been loaded from SD Card
  // (resp. no SD Card is available)
  hw_enabled = 0;

  // initialize all J10 pins as inputs with internal Pull-Up
  int pin;
  for(pin=0; pin<8; ++pin)
    MIOS32_BOARD_J10_PinInit(pin, MIOS32_BOARD_PIN_MODE_INPUT_PU);

  // initialize LCDs
  MBNG_LCD_Init(0);

  // initialize the AINSER module(s)
  AINSER_Init(0);

  // create semaphores
  xSDCardSemaphore = xSemaphoreCreateRecursiveMutex();
  xMIDIINSemaphore = xSemaphoreCreateRecursiveMutex();
  xMIDIOUTSemaphore = xSemaphoreCreateRecursiveMutex();

  // install SysEx callback
  MIOS32_MIDI_SysExCallback_Init(APP_SYSEX_Parser);

  // install MIDI Rx/Tx callback functions
  MIOS32_MIDI_DirectRxCallback_Init(&NOTIFY_MIDI_Rx);
  MIOS32_MIDI_DirectTxCallback_Init(&NOTIFY_MIDI_Tx);

  // install timeout callback function
  MIOS32_MIDI_TimeOutCallback_Init(&NOTIFY_MIDI_TimeOut);

  // initialize code modules
  MIDI_PORT_Init(0);
  MBNG_SYSEX_Init(0);
  MIDI_ROUTER_Init(0);
  MBNG_PATCH_Init(0);
  MBNG_EVENT_Init(0);
  MBNG_DIN_Init(0);
  MBNG_DOUT_Init(0);
  MBNG_ENC_Init(0);
  MBNG_AIN_Init(0);
  MBNG_MATRIX_Init(0);
  UIP_TASK_Init(0);
  SCS_Init(0);
  SCS_CONFIG_Init(0);
  TERMINAL_Init(0);
  MIDIMON_Init(0);
  MBNG_FILE_Init(0);
  SEQ_BPM_Init(0);
  SEQ_BPM_Set(120.0);
  SEQ_MIDI_OUT_Init(0);

  // install timer function which is called each 100 uS
  MIOS32_TIMER_Init(1, 100, APP_Periodic_100uS, MIOS32_IRQ_PRIO_MID);

  // start tasks
  xTaskCreate(TASK_Period_1mS, (signed portCHAR *)"1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS, NULL);
  xTaskCreate(TASK_Period_1mS_LP, (signed portCHAR *)"1mS_LP", 2*configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS_LP, NULL);
  xTaskCreate(TASK_Period_1mS_SD, (signed portCHAR *)"1mS_SD", 2*configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS_SD, NULL);

}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
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
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // filter SysEx which is handled by separate parser
  if( !(midi_package.evnt0 < 0xf8 &&
       (midi_package.cin == 0xf ||
       (midi_package.cin >= 0x4 && midi_package.cin <= 0x7))) ) {

#if DEBUG_EVENT_HANDLER_PERFORMANCE
    MIOS32_STOPWATCH_Reset();
#endif

    // -> Event Handler
    MBNG_EVENT_MIDI_NotifyPackage(port, midi_package);

#if DEBUG_EVENT_HANDLER_PERFORMANCE
    u32 cycles = MIOS32_STOPWATCH_ValueGet();
    if( cycles == 0xffffffff )
      DEBUG_MSG("[PERF PCK:%08x] overrun!\n", midi_package.ALL);
    else
      DEBUG_MSG("[PERF PCK:%08x] %5d.%d mS\n", midi_package.ALL, cycles/10, cycles%10);
#endif
  }

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
s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // -> MBNG
  MBNG_SYSEX_Parser(port, midi_in);

#if DEBUG_EVENT_HANDLER_PERFORMANCE
    MIOS32_STOPWATCH_Reset();
#endif

  // -> Event router
  MBNG_EVENT_ReceiveSysEx(port, midi_in);

#if DEBUG_EVENT_HANDLER_PERFORMANCE
  u32 cycles = MIOS32_STOPWATCH_ValueGet();
  if( cycles == 0xffffffff )
    DEBUG_MSG("[PERF SYX:%02x] overrun!\n", midi_in);
  else
    DEBUG_MSG("[PERF SYX:%02x] %5d.%d mS\n", midi_in, cycles/10, cycles%10);
#endif

  // -> MIDI Router
  MIDI_ROUTER_ReceiveSysEx(port, midi_in);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
  // pass current pin state of J10
  // only available for LPC17xx, all others (like STM32) default to SRIO
  SCS_AllPinsSet(0xff00 | MIOS32_BOARD_J10_Get());

  // update encoders/buttons of SCS
  SCS_EncButtonUpdate_Tick();

  // Matrix handler
  MBNG_MATRIX_PrepareCol();
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
  // Matrix handler
  MBNG_MATRIX_GetRow();
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
  // -> MBNG_DIN once enabled
  if( hw_enabled )
    MBNG_DIN_NotifyToggle(pin, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
  // pass encoder event to SCS handler
  if( encoder == SCS_ENC_MENU_ID ) // == 0 (assigned by SCS)
    SCS_ENC_MENU_NotifyChange(incrementer);
  else {
    // -> ENC handler
    MBNG_ENC_NotifyChange(encoder-1, incrementer);
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  // -> MBNG_AIN once enabled
  if( hw_enabled )
    MBNG_AIN_NotifyChange(pin, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an AINSER pot has been moved
/////////////////////////////////////////////////////////////////////////////
static void APP_AINSER_NotifyChange(u32 module, u32 pin, u32 pin_value)
{
  // -> MBNG_AIN once enabled
  if( hw_enabled )
    MBNG_AIN_NotifyChange_SER64(module, pin, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This task handles the control surface
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period_1mS_LP(void *pvParameters)
{
  static u8 isInMainPage = 0;

  SCS_DisplayUpdateInMainPage(0);

  while( 1 ) {
    vTaskDelay(1 / portTICK_RATE_MS);

    // call SCS handler
    SCS_Tick();

    // LCD output in mainpage
    if( SCS_MenuStateGet() == SCS_MENU_STATE_MAINPAGE ) {
      u8 force = isInMainPage == 0;
      if( force ) // page change
	MBNG_LCD_SpecialCharsReInit();

      BUFLCD_Update(force);
      isInMainPage = 1; // static reminder
    } else {
      isInMainPage = 0; // static reminder
    }

    // MIDI In/Out monitor
    MIDI_PORT_Period1mS();

    // call MIDI event tick
    MBNG_EVENT_Tick();
  }

}


/////////////////////////////////////////////////////////////////////////////
// This task handles the SD Card
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period_1mS_SD(void *pvParameters)
{
  const u16 sdcard_check_delay = 1000;
  u16 sdcard_check_ctr = 0;
  u8 lun_available = 0;

  while( 1 ) {
    vTaskDelay(1 / portTICK_RATE_MS);

    // each second: check if SD Card (still) available
    if( msd_state == MSD_DISABLED && ++sdcard_check_ctr >= sdcard_check_delay ) {
      sdcard_check_ctr = 0;

      MUTEX_SDCARD_TAKE;
      s32 status = FILE_CheckSDCard();

      if( status == 1 ) {
	DEBUG_MSG("SD Card connected: %s\n", FILE_VolumeLabel());

	// stop sequencer
	SEQ_BPM_Stop();

	// load all file infos
	MBNG_FILE_LoadAllFiles(1); // including HW info

	// immediately go to next step
	sdcard_check_ctr = sdcard_check_delay;
      } else if( status == 2 ) {
	DEBUG_MSG("SD Card disconnected\n");
	// invalidate all file infos
	MBNG_FILE_UnloadAllFiles();

	// stop sequencer
	SEQ_BPM_Stop();

	// change status
	MBNG_FILE_StatusMsgSet("No SD Card");
      } else if( status == 3 ) {
	if( !FILE_SDCardAvailable() ) {
	  DEBUG_MSG("SD Card not found\n");
	  MBNG_FILE_StatusMsgSet("No SD Card");
	} else if( !FILE_VolumeAvailable() ) {
	  DEBUG_MSG("ERROR: SD Card contains invalid FAT!\n");
	  MBNG_FILE_StatusMsgSet("No FAT");
	} else {
	  MBNG_FILE_StatusMsgSet(NULL);

	  portENTER_CRITICAL();

	  // check if configuration file exists
	  if( !MBNG_FILE_C_Valid() ) {
	    // create new one
	    DEBUG_MSG("Creating initial DEFAULT.NGC file\n");

	    if( (status=MBNG_FILE_C_Write("DEFAULT")) < 0 ) {
	      DEBUG_MSG("Failed to create file! (status: %d)\n", status);
	    }
	  }

	  // check if patch file exists
	  if( !MBNG_FILE_L_Valid() ) {
	    // create new one
	    DEBUG_MSG("Creating initial DEFAULT.NGL file\n");

	    if( (status=MBNG_FILE_L_Write("DEFAULT")) < 0 ) {
	      DEBUG_MSG("Failed to create file! (status: %d)\n", status);
	    }
	  }

	  // select first bank
	  MBNG_PATCH_BankSet(0);

	  portEXIT_CRITICAL();
	}

	hw_enabled = 1; // enable hardware after first read...
      }

      MUTEX_SDCARD_GIVE;
    }

    // MSD driver
    if( msd_state != MSD_DISABLED ) {
      MUTEX_SDCARD_TAKE;

      switch( msd_state ) {
      case MSD_SHUTDOWN:
	// switch back to USB MIDI
	MIOS32_USB_Init(1);
	msd_state = MSD_DISABLED;
	break;

      case MSD_INIT:
	// LUN not mounted yet
	lun_available = 0;

	// enable MSD USB driver
	//MUTEX_J16_TAKE;
	if( MSD_Init(0) >= 0 )
	  msd_state = MSD_READY;
	else
	  msd_state = MSD_SHUTDOWN;
	//MUTEX_J16_GIVE;
	break;

      case MSD_READY:
	// service MSD USB driver
	MSD_Periodic_mS();

	// this mechanism shuts down the MSD driver if SD card has been unmounted by OS
	if( lun_available && !MSD_LUN_AvailableGet(0) )
	  msd_state = MSD_SHUTDOWN;
	else if( !lun_available && MSD_LUN_AvailableGet(0) )
	  lun_available = 1;
	break;
      }

      MUTEX_SDCARD_GIVE;
    }
  }

}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS to handle sequencer requests
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period_1mS(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // skip delay gap if we had to wait for more than 5 ticks to avoid 
    // unnecessary repeats until xLastExecutionTime reached xTaskGetTickCount() again
    portTickType xCurrentTickCount = xTaskGetTickCount();
    if( xLastExecutionTime < (xCurrentTickCount-5) )
      xLastExecutionTime = xCurrentTickCount;

//    // execute sequencer handler
//    MUTEX_SDCARD_TAKE;
//    SEQ_Handler();
//    MUTEX_SDCARD_GIVE;

    // send timestamped MIDI events
    MUTEX_MIDIOUT_TAKE;
    SEQ_MIDI_OUT_Handler();
    MUTEX_MIDIOUT_GIVE;

    // Scan Matrix button handler
    MBNG_MATRIX_ButtonHandler();

    // scan AINSER pins
    AINSER_Handler(APP_AINSER_NotifyChange);
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
  MBNG_SYSEX_TimeOut(port);

  // print message on screen
  SCS_Msg(SCS_MSG_L, 2000, "MIDI Protocol", "TIMEOUT !!!");

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// MSD access functions
/////////////////////////////////////////////////////////////////////////////
s32 TASK_MSD_EnableSet(u8 enable)
{
  MIOS32_IRQ_Disable();
  if( msd_state == MSD_DISABLED && enable ) {
    msd_state = MSD_INIT;
  } else if( msd_state == MSD_READY && !enable )
    msd_state = MSD_SHUTDOWN;
  MIOS32_IRQ_Enable();

  return 0; // no error
}

s32 TASK_MSD_EnableGet()
{
  return (msd_state == MSD_READY) ? 1 : 0;
}

s32 TASK_MSD_FlagStrGet(char str[5])
{
  str[0] = MSD_CheckAvailable() ? 'U' : '-';
  str[1] = MSD_LUN_AvailableGet(0) ? 'M' : '-';
  str[2] = MSD_RdLEDGet(250) ? 'R' : '-';
  str[3] = MSD_WrLEDGet(250) ? 'W' : '-';
  str[4] = 0;

  return 0; // no error
}
