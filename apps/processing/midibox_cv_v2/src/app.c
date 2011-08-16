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
#include <msd.h>
#include <aout.h>
#include "app.h"
#include "tasks.h"

#include "mbcv_sysex.h"
#include "mbcv_patch.h"
#include "mbcv_router.h"
#include "mbcv_port.h"

// include source of the SCS
#include <scs.h>
#include "scs_config.h"

#include "file.h"
#include "mbcv_file.h"
#include "mbcv_file_p.h"

#include <seq_bpm.h>
#include <seq_midi_out.h>

#include "terminal.h"
#include "midimon.h"

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

static msd_state_t msd_state;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void APP_Periodic_100uS(void);
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 byte);



/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // disable MSD by default (has to be enabled in SEQ_UI_FILE menu)
  msd_state = MSD_DISABLED;

  // hardware will be enabled once configuration has been loaded from SD Card
  // (resp. no SD Card is available)
  hw_enabled = 0;

  // initialize all J10 pins as inputs with internal Pull-Up
  int pin;
  for(pin=0; pin<8; ++pin)
    MIOS32_BOARD_J10_PinInit(pin, MIOS32_BOARD_PIN_MODE_INPUT_PU);


  // create semaphores
  xSDCardSemaphore = xSemaphoreCreateRecursiveMutex();
  xMIDIINSemaphore = xSemaphoreCreateRecursiveMutex();
  xMIDIOUTSemaphore = xSemaphoreCreateRecursiveMutex();

  // install SysEx callback
  MIOS32_MIDI_SysExCallback_Init(APP_SYSEX_Parser);

  // install MIDI Rx callback function
  MIOS32_MIDI_DirectRxCallback_Init(NOTIFY_MIDI_Rx);

  // initialize code modules
  MBCV_PORT_Init(0);
  MBCV_SYSEX_Init(0);
  MBCV_ROUTER_Init(0);
  MBCV_PATCH_Init(0);
  UIP_TASK_Init(0);
  SCS_Init(0);
  SCS_CONFIG_Init(0);
  TERMINAL_Init(0);
  MIDIMON_Init(0);
  MBCV_FILE_Init(0);
  SEQ_MIDI_OUT_Init(0);

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
  // -> MIDI Router
  MBCV_ROUTER_Receive(port, midi_package);

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
  // -> MBCV
  MBCV_SYSEX_Parser(port, midi_in);

  // -> MIDI Router
  MBCV_ROUTER_ReceiveSysEx(port, midi_in);

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
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
  // pass encoder event to SCS handler
  if( encoder == SCS_ENC_MENU_ID )
    SCS_ENC_MENU_NotifyChange(incrementer);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}




/////////////////////////////////////////////////////////////////////////////
// This task handles the control surface
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period_1mS_LP(void *pvParameters)
{
  MIOS32_LCD_Clear();

  while( 1 ) {
    vTaskDelay(1 / portTICK_RATE_MS);

    // call SCS handler
    SCS_Tick();

  }

}


/////////////////////////////////////////////////////////////////////////////
// This task handles the SD Card
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period_1mS_SD(void *pvParameters)
{
  u16 sdcard_check_ctr = 0;
  u8 lun_available = 0;

  while( 1 ) {
    vTaskDelay(1 / portTICK_RATE_MS);

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
	  // check if patch file exists
	  if( !MBCV_FILE_P_Valid() ) {
	    // create new one
	    DEBUG_MSG("Creating initial DEFAULT.CV2 file\n");

	    if( (status=MBCV_FILE_P_Write("DEFAULT")) < 0 ) {
	      DEBUG_MSG("Failed to create file! (status: %d)\n", status);
	    }
	  }

	  // disable status message and print patch
	  MBCV_FILE_StatusMsgSet(NULL);
	}
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
// This task is called periodically each mS to handle AOUTs
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

    if( hw_enabled ) {
      // update AOUT channels
      AOUT_Update();
    }
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
  // here we could filter a certain port
  // The BPM generator will deliver inaccurate results if MIDI clock 
  // is received from multiple ports
  SEQ_BPM_NotifyMIDIRx(midi_byte);

  return 0; // no error, no filtering
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
