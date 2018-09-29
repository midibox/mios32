// $Id: app.c 1223 2011-06-23 21:26:52Z tk $
/*
 * Demo application for SSD1322 GLCD
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
#include <msd.h>
#include "app.h"
#include "tasks.h"
#include "hardware.h"

#include <app_lcd.h>

#include <midi_port.h>
#include <midi_router.h>
#include <midimon.h>

#include "file.h"

#include <seq_bpm.h>
#include <seq_midi_out.h>

#include "loopa.h"
#include "terminal.h"
#include "screen.h"


// #define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


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
// global variables
/////////////////////////////////////////////////////////////////////////////
u8 hw_enabled;    // After startup/config loaded, enable local hardware

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

// for mutual exclusive SD Card access between different tasks
// The mutex is handled with MUTEX_SDCARD_TAKE and MUTEX_SDCARD_GIVE
// macros inside the application, which contain a different implementation
// for emulation
xSemaphoreHandle xSDCardSemaphore;

// Mutex for MIDI IN/OUT handler
xSemaphoreHandle xMIDIINSemaphore;
xSemaphoreHandle xMIDIOUTSemaphore;

// Mutex for digital out (e.g. GP LED) updates
xSemaphoreHandle xDigitalOutSemaphore;

static volatile msd_state_t msd_state;

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
   MIOS32_BOARD_LED_Init(0xffffffff); // initialize all LEDs

   MIOS32_MIDI_SendDebugMessage("=============================================================");
   MIOS32_MIDI_SendDebugMessage("Starting LoopA");

   // enable MSD by default (has to be enabled in SHIFT menu)
   msd_state = MSD_DISABLED;

   // hardware will be enabled once configuration has been loaded from SD Card
   // (resp. no SD Card is available)
   hw_enabled = 0;

   // create semaphores
   xSDCardSemaphore = xSemaphoreCreateRecursiveMutex();
   xMIDIINSemaphore = xSemaphoreCreateRecursiveMutex();
   xMIDIOUTSemaphore = xSemaphoreCreateRecursiveMutex();
   xDigitalOutSemaphore = xSemaphoreCreateRecursiveMutex();

   // install SysEx callback
   MIOS32_MIDI_SysExCallback_Init(APP_SYSEX_Parser);

   // install MIDI Rx/Tx callback functions
   MIOS32_MIDI_DirectRxCallback_Init(&NOTIFY_MIDI_Rx);
   MIOS32_MIDI_DirectTxCallback_Init(&NOTIFY_MIDI_Tx);

   // install timeout callback function
   MIOS32_MIDI_TimeOutCallback_Init(&NOTIFY_MIDI_TimeOut);

   // initialize code modules
   MIDI_PORT_Init(0);
   MIDI_ROUTER_Init(0);
   TERMINAL_Init(0);
   MIDIMON_Init(0);
   FILE_Init(0);
   SEQ_MIDI_OUT_Init(0);
   seqInit(0);

   // install four encoders...
   mios32_enc_config_t enc_config = MIOS32_ENC_ConfigGet(enc_scene_id);
   enc_config.cfg.type = DETENTED3;
   enc_config.cfg.sr = enc_scene / 8 + 1;
   enc_config.cfg.pos = enc_scene % 8;
   enc_config.cfg.speed = NORMAL;
   enc_config.cfg.speed_par = 0;
   MIOS32_ENC_ConfigSet(enc_scene_id, enc_config);

   enc_config = MIOS32_ENC_ConfigGet(enc_track_id);
   enc_config.cfg.type = DETENTED3;
   enc_config.cfg.sr = enc_track / 8 + 1;
   enc_config.cfg.pos = enc_track % 8;
   enc_config.cfg.speed = NORMAL;
   enc_config.cfg.speed_par = 0;
   MIOS32_ENC_ConfigSet(enc_track_id, enc_config);

   enc_config = MIOS32_ENC_ConfigGet(enc_page_id);
   enc_config.cfg.type = DETENTED3;
   enc_config.cfg.sr = enc_page / 8 + 1;
   enc_config.cfg.pos = enc_page % 8;
   enc_config.cfg.speed = NORMAL;
   enc_config.cfg.speed_par = 0;
   MIOS32_ENC_ConfigSet(enc_page_id, enc_config);

   enc_config = MIOS32_ENC_ConfigGet(enc_data_id);
   enc_config.cfg.type = DETENTED3;
   enc_config.cfg.sr = enc_data / 8 + 1;
   enc_config.cfg.pos = enc_data % 8;
   enc_config.cfg.speed = NORMAL;
   enc_config.cfg.speed_par = 0;
   MIOS32_ENC_ConfigSet(enc_data_id, enc_config);

   // start tasks
   xTaskCreate(TASK_Period_1mS, (const char * const)"1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS, NULL);
   xTaskCreate(TASK_Period_1mS_LP, (const char * const)"1mS_LP", 2*configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS_LP, NULL);
   xTaskCreate(TASK_Period_1mS_SD, (const char * const)"1mS_SD", 2*configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS_SD, NULL);

   loopaStartup();
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the MIDI task which checks for incoming
// MIDI events. You could add more MIDI related jobs here, but they shouldn't
// consume more than 300 uS to ensure the responsiveness of incoming MIDI.
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_Tick(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // -> MIDI Router
  MIDI_ROUTER_Receive(port, midi_package);

  // -> MIDI Port Handler (used for MIDI monitor function)
  MIDI_PORT_NotifyMIDIRx(port, midi_package);

  /// -> MIDI file recorder
  /// MID_FILE_Receive(port, midi_package);

  // Record midi event in loopa
  loopaRecord(port, midi_package);

  // forward to MIDI Monitor
  // SysEx messages have to be filtered for USB0 and UART0 to avoid data corruption
  // (the SysEx stream would interfere with monitor messages)
  u8 filter_sysex_message = (port == USB0) || (port == UART0);
  MIDIMON_Receive(port, midi_package, filter_sysex_message);

   /*
  // Draw notes to voxel space
  if( midi_package.event == NoteOn && midi_package.velocity > 0)
     voxelNoteOn(midi_package.note, midi_package.velocity);

  if( midi_package.event == NoteOff || (midi_package.event == NoteOn && midi_package.velocity == 0) )
     voxelNoteOff(midi_package.note);
     */
}


/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MIOS32 commands
/////////////////////////////////////////////////////////////////////////////
s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
   // -> MIDI Router
   MIDI_ROUTER_ReceiveSysEx(port, midi_in);

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
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
   // MIOS32_MIDI_SendDebugMessage("PIN %d toggled - value %d", pin, pin_value);

   if (pin_value == 0)
      loopaButtonPressed(pin);
   else
      loopaButtonReleased(pin);
}

/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
   //MIOS32_MIDI_SendDebugMessage("Encoder %d turned - increment %d", encoder, incrementer);

   loopaEncoderTurned(encoder, incrementer);
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
   u16 taskCtr = 0;

   while( 1 )
   {
      vTaskDelay(1 / portTICK_RATE_MS);
      taskCtr++;

      // call SCS handler
      // SCS_Tick(); Call our own

      // MIDI In/Out monitor
      MIDI_PORT_Period1mS();

      if (taskCtr % 20 == 0)
      {
         display();
         //updateGPLeds();
         updateLEDs();
      }
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

  while (1)
  {
    vTaskDelay(1 / portTICK_RATE_MS);

    // each second: check if SD Card (still) available

    if (msd_state == MSD_DISABLED && ++sdcard_check_ctr >= sdcard_check_delay)
    {
       sdcard_check_ctr = 0;

       MUTEX_SDCARD_TAKE;
       s32 status = FILE_CheckSDCard();

       if (status == 1)
       {
          DEBUG_MSG("SD Card connected: %s\n", FILE_VolumeLabel());

          // stop sequencer
          SEQ_BPM_Stop();

          // load all file infos
          /// MIDIO_FILE_LoadAllFiles(1); // including HW info

          // immediately go to next step
          sdcard_check_ctr = sdcard_check_delay;
       }
       else if (status == 2)
       {
          DEBUG_MSG("SD Card disconnected\n");
          // invalidate all file infos
          /// MIDIO_FILE_UnloadAllFiles();

          // stop sequencer
          SEQ_BPM_Stop();

          // change status
          /// MIDIO_FILE_StatusMsgSet("No SD Card");
       }
       else if (status == 3)
       {
          if (!FILE_SDCardAvailable())
          {
             DEBUG_MSG("SD Card not found\n");
             /// MIDIO_FILE_StatusMsgSet("No SD Card");
          }
          else if (!FILE_VolumeAvailable())
          {
             DEBUG_MSG("ERROR: SD Card contains invalid FAT!\n");
             /// MIDIO_FILE_StatusMsgSet("No FAT");
          }
          else
          {
             // create the default files if they don't exist on SD Card
             /// MIDIO_FILE_CreateDefaultFiles();
             loopaSDCardAvailable();
          }

          hw_enabled = 1; // enable hardware after first read...
       }

       MUTEX_SDCARD_GIVE;
    }

    // MSD driver
    if( msd_state != MSD_DISABLED )
    {
       MUTEX_SDCARD_TAKE;

       switch( msd_state )
       {
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

  while( 1 )
  {
     vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

     // skip delay gap if we had to wait for more than 5 ticks to avoid
     // unnecessary repeats until xLastExecutionTime reached xTaskGetTickCount() again
     portTickType xCurrentTickCount = xTaskGetTickCount();
     if (xLastExecutionTime < xCurrentTickCount-5)
        xLastExecutionTime = xCurrentTickCount;

     // execute sequencer handler
     MUTEX_SDCARD_TAKE;
     seqHandler();
     MUTEX_SDCARD_GIVE;

     // send timestamped MIDI events
     MUTEX_MIDIOUT_TAKE;
     SEQ_MIDI_OUT_Handler();
     MUTEX_MIDIOUT_GIVE;

     // Scan Matrix button handler
     /// MIDIO_MATRIX_ButtonHandler();

     // scan AINSER pins
     /// AINSER_Handler(APP_AINSER_NotifyChange);
  }
}


/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectRxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 midi_byte)
{
  // filter MIDI In port which controls the MIDI clock
  if (MIDI_ROUTER_MIDIClockInGet(port) == 1)
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
   /// MIDIO_SYSEX_TimeOut(port);

   // print message on screen
   /// SCS_Msg(SCS_MSG_L, 2000, "MIDI Protocol", "TIMEOUT !!!");

   return 0;
}


/////////////////////////////////////////////////////////////////////////////
// MSD access functions
/////////////////////////////////////////////////////////////////////////////
s32 TASK_MSD_EnableSet(u8 enable)
{
   MIOS32_IRQ_Disable();
   if( msd_state == MSD_DISABLED && enable )
   {
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


/////////////////////////////////////////////////////////////////////////////
// functions to access MIDI IN/Out Mutex
// see also mios32_config.h
/////////////////////////////////////////////////////////////////////////////
void APP_MUTEX_MIDIOUT_Take(void) { MUTEX_MIDIOUT_TAKE; }
void APP_MUTEX_MIDIOUT_Give(void) { MUTEX_MIDIOUT_GIVE; }
void APP_MUTEX_MIDIIN_Take(void) { MUTEX_MIDIIN_TAKE; }
void APP_MUTEX_MIDIIN_Give(void) { MUTEX_MIDIIN_GIVE; }

