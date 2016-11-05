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
#include <glcd_font.h>
#include "tasks.h"
#include "app.h"
#include "MbCvEnvironment.h"

#include <file.h>

#include <scs.h>
#include <scs_lcd.h>
#include "scs_config.h"

#include <midi_port.h>
#include <midi_router.h>
#include <midimon.h>

#include <app_lcd.h>

#include "terminal.h"
#include "mbcv_hwcfg.h"
#include "mbcv_sysex.h"
#include "mbcv_patch.h"
#include "mbcv_map.h"
#include "mbcv_button.h"
#include "mbcv_lre.h"
#include "mbcv_rgb.h"
#include "mbcv_file.h"
#include "mbcv_file_p.h"
#include "mbcv_file_b.h"
#include "mbcv_file_hw.h"
#include "uip_task.h"
#include "osc_client.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 2

// measure performance with the stopwatch
// 0: off
// 1: CV processing only
// 2: CV processing + mapping
//#define STOPWATCH_PERFORMANCE_MEASURING 2
#define STOPWATCH_PERFORMANCE_MEASURING 0


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////
static u32 stopwatch_value;
static u32 stopwatch_value_min;
static u32 stopwatch_value_max;
static u32 stopwatch_value_accumulated;
static u32 stopwatch_value_accumulated_ctr;

static u8  cv_se_speed_factor;
static u8  cv_se_overloaded;
static u32 cv_se_overload_check_ctr = 0;
static u32 cv_se_last_mios32_timestamp = 0;


/////////////////////////////////////////////////////////////////////////////
// C++ objects
/////////////////////////////////////////////////////////////////////////////
MbCvEnvironment mbCvEnvironment;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
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

  // init hardware config
  MBCV_HWCFG_Init(0);

  // init Stopwatch
  APP_StopwatchInit();

  // initialize all J10 pins as inputs with internal Pull-Up
  int pin;
  for(pin=0; pin<8; ++pin)
    MIOS32_BOARD_J10_PinInit(pin, MIOS32_BOARD_PIN_MODE_INPUT_PU);

  // initialize OLED pins at Alternative LCD port (LPC17: J5/J28, STM32F4: J10B)
  {
    APP_SelectScopeLCDs();

    APP_LCD_Init(0);

    int lcd;
    for(lcd=0; lcd<4; ++lcd) {
      MIOS32_LCD_DeviceSet(lcd);
      MIOS32_LCD_Clear();
    }

    APP_SelectMainLCD();
  }

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
  MBCV_BUTTON_Init(0);
  MBCV_LRE_Init(0);
  MBCV_RGB_Init(0);
  UIP_TASK_Init(0);

  SCS_Init(0);
  SCS_CONFIG_Init(0);

  if( MIOS32_LCD_TypeIsGLCD() && mios32_lcd_parameters.height >= 64 ) {
    SCS_LCD_OffsetYSet(6);
    mbCvEnvironment.mbCvScope[0].setShowOnMainScreen(true);

    // print warning message on alt screen, that it isn't used anymore!
    APP_SelectScopeLCDs();
    MIOS32_LCD_DeviceSet(0);
    MIOS32_LCD_FontInit((u8 *)GLCD_FONT_SMALL);
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintFormattedString("Display not used!");
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintFormattedString("Content visible on main screen!");
    MIOS32_LCD_CursorSet(0, 2);
    MIOS32_LCD_PrintFormattedString("Try another CS line");
    MIOS32_LCD_CursorSet(0, 3);
    MIOS32_LCD_PrintFormattedString("to display other scopes!");
    MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
    APP_SelectMainLCD();
  }

  TERMINAL_Init(0);
  MIDIMON_Init(0);
  MBCV_FILE_Init(0);

  // initialize tasks
  TASKS_Init(0);

  // initialize MbCvEnvironment
  APP_CvUpdateRateFactorSet(5);

#if MIOS32_DONT_SERVICE_SRIO_SCAN
  //MIOS32_SRIO_ScanNumSet(4);

  // standard SRIO scan has been disabled in programming_models/traditional/main.c via MIOS32_DONT_SERVICE_SRIO_SCAN in mios32_config.h
  // start the scan here - and retrigger it whenever it's finished
  APP_SRIO_ServicePrepare();
  MIOS32_SRIO_ScanStart(APP_SRIO_ServiceFinish);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_Background(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the main task which also handles DIN, ENC
// and AIN events. You could add more jobs here, but they shouldn't consume
// more than 300 uS to ensure the responsiveness of buttons, encoders, pots.
// Alternatively you could create a dedicated task for application specific
// jobs as explained in $MIOS32_PATH/apps/tutorials/006_rtos_tasks
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_Tick(void)
{
  // PWM modulate the status LED (this is a sign of life)
  u32 timestamp = MIOS32_TIMESTAMP_Get();
  MIOS32_BOARD_LED_Set(1, (timestamp % 20) <= ((timestamp / 100) % 10));
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the MIDI task which checks for incoming
// MIDI events. You could add more MIDI related jobs here, but they shouldn't
// consume more than 300 uS to ensure the responsiveness of incoming MIDI.
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_MIDI_Tick(void)
{
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
  MIDIMON_Receive(port, midi_package, filter_sysex_message);
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
#if MIOS32_DONT_SERVICE_SRIO_SCAN
  // update encoder states
  MIOS32_ENC_UpdateStates();

  // standard SRIO scan has been disabled in programming_models/traditional/main.c via MIOS32_DONT_SERVICE_SRIO_SCAN in mios32_config.h
  // start the scan here - and retrigger it whenever it's finished
  APP_SRIO_ServicePrepare();
  MIOS32_SRIO_ScanStart(APP_SRIO_ServiceFinish);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
  // -> MBCV Button handler
  MBCV_BUTTON_Handler(pin, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
  // pass encoder event to SCS handler
  if( encoder == SCS_ENC_MENU_ID ) // == 0 (assigned by SCS)
    SCS_ENC_MENU_NotifyChange(incrementer);
  else {
    // -> ENC Handler
    MBCV_LRE_NotifyChange(encoder-1, incrementer);
  }
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
  // ignore as long as hardware config hasn't been read
  if( !MBCV_FILE_HW_ConfigLocked() ) {
    cv_se_last_mios32_timestamp = MIOS32_TIMESTAMP_Get();
    return;
  }

  // check MIOS32 timestamp each second
  if( ++cv_se_overload_check_ctr >= (cv_se_speed_factor*500) ) {
    u32 timestamp = MIOS32_TIMESTAMP_Get();
    u32 delay = timestamp - cv_se_last_mios32_timestamp;
    cv_se_overload_check_ctr = 0;
    cv_se_last_mios32_timestamp = timestamp;

    // actually 1000, allow some margin
    cv_se_overloaded = (delay < 950 );
  }

  // don't process CV if engine overloaded
  if( cv_se_overloaded )
    return;

#if STOPWATCH_PERFORMANCE_MEASURING == 1 || STOPWATCH_PERFORMANCE_MEASURING == 2
  APP_StopwatchReset();
#endif

  if( !mbCvEnvironment.tick() ) {
    return; // no update required
  }

#if STOPWATCH_PERFORMANCE_MEASURING == 1
  APP_StopwatchCapture();
#endif

  // update AOUTs
  MBCV_MAP_Update();

  // start ADC conversions
  MIOS32_AIN_StartConversions();

#if STOPWATCH_PERFORMANCE_MEASURING == 2
  APP_StopwatchCapture();
#endif
}




/////////////////////////////////////////////////////////////////////////////
// This task handles the control surface
/////////////////////////////////////////////////////////////////////////////
void APP_TASK_Period_1mS_LP(void)
{
  static u8 clear_lcd = 1;
  static u32 performance_print_timestamp = 0;

  MUTEX_LCD_TAKE;

  if( clear_lcd ) {
    clear_lcd = 0;
    MIOS32_LCD_Clear();
  }

  // call SCS handler
  SCS_Tick();

  MUTEX_LCD_GIVE;

  // MIDI In/Out monitor
  MIDI_PORT_Period1mS();

  // LED rings and RGB LEDs
  MBCV_LRE_UpdateAllLedRings();
  MBCV_RGB_UpdateAllLeds();

  // output and reset current stopwatch max value each second
  if( MIOS32_TIMESTAMP_GetDelay(performance_print_timestamp) > 1000 ) {
    performance_print_timestamp = MIOS32_TIMESTAMP_Get();

    MUTEX_MIDIOUT_TAKE;
    MIOS32_IRQ_Disable();
    u32 min_value = stopwatch_value_min;
    stopwatch_value_min = ~0;
    u32 max_value = stopwatch_value_max;
    stopwatch_value_max = 0;
    u32 acc_value = stopwatch_value_accumulated;
    stopwatch_value_accumulated = 0;
    u32 acc_ctr = stopwatch_value_accumulated_ctr;
    stopwatch_value_accumulated_ctr = 0;
    MIOS32_IRQ_Enable();
#if DEBUG_VERBOSE_LEVEL >= 2
    if( acc_value || max_value )
      DEBUG_MSG("%d uS (min: %d uS, max: %d uS)\n", acc_value / acc_ctr, min_value, max_value);
#endif
    MUTEX_MIDIOUT_GIVE;
  }

}


/////////////////////////////////////////////////////////////////////////////
// This task handles the scope displays with very low priority
/////////////////////////////////////////////////////////////////////////////
void APP_TASK_Period_1mS_LP2(void)
{
  {
    // CV Scopes (note: mutex is taken inside function)
    mbCvEnvironment.tickScopes();
  }

#if 0
  // CV Bars (currently only for SSD1306)
  {
    MUTEX_LCD_TAKE;

    if( mios32_lcd_parameters.lcd_type == MIOS32_LCD_TYPE_GLCD_SSD1306 ) {

      MIOS32_LCD_DeviceSet(0);
      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_METER_ICONS_V); // memo: 28 icons, 14 used, icon size: 8x32

      u16 *outMeter = mbCvEnvironment.cvOutMeter.first();
      for(int cv=0; cv<CV_SE_NUM; ++cv, ++outMeter) {
	MIOS32_LCD_CursorSet(0 + 2*cv, 4);
	MIOS32_LCD_PrintChar((*outMeter * 13) / 65535);
      }

      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);    
    }

    MUTEX_LCD_GIVE;
  }
#endif
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

    if( status == 1 ) {
      DEBUG_MSG("SD Card connected: %s\n", FILE_VolumeLabel());
      // load all file infos
      MBCV_FILE_LoadAllFiles(1); // including HW info

      // load A001
      mbCvEnvironment.bankLoad(0, 0);
    } else if( status == 2 ) {
      DEBUG_MSG("SD Card disconnected\n");
      // invalidate all file infos
      MBCV_FILE_UnloadAllFiles();

      // change status
      MBCV_FILE_StatusMsgSet("No SD Card");
    } else if( status == 3 ) {
      if( !FILE_SDCardAvailable() ) {
	DEBUG_MSG("SD Card not found\n");
	MBCV_FILE_HW_LockConfig(); // lock configuration
	MBCV_FILE_StatusMsgSet("No SD Card");
      } else if( !FILE_VolumeAvailable() ) {
	DEBUG_MSG("ERROR: SD Card contains invalid FAT!\n");
	MBCV_FILE_HW_LockConfig(); // lock configuration
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
}


/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectRxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 midi_byte)
{
  // filter MIDI In port which controls the MIDI clock
  if( MIDI_ROUTER_MIDIClockInGet(port) == 1 ) {
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
// Switches to the scope LCDs
/////////////////////////////////////////////////////////////////////////////
s32 APP_SelectScopeLCDs(void)
{
  // select the alternative pinning via J5A/J28
  APP_LCD_AltPinningSet(1);

  // select GLCD type and dimensions
  mios32_lcd_parameters.lcd_type = MIOS32_LCD_TYPE_GLCD_SSD1306; // TODO: get this from a configuration file
  mios32_lcd_parameters.num_x = 4;
  mios32_lcd_parameters.num_y = 1;
  mios32_lcd_parameters.width = 128;
  mios32_lcd_parameters.height = 64;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Switches to the original LCD
/////////////////////////////////////////////////////////////////////////////
s32 APP_SelectMainLCD(void)
{
  // ensure that first LCD is selected
  MIOS32_LCD_DeviceSet(0);

  // select the original pinning
  APP_LCD_AltPinningSet(0);

  // retrieve original LCD parameters
  MIOS32_LCD_ParametersFetchFromBslInfoRange();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Speed Factor (engine will be updated at factor * 500 Hz)
/////////////////////////////////////////////////////////////////////////////
s32 APP_CvUpdateRateFactorSet(u8 factor)
{
  if( factor < 1 || factor > APP_CV_UPDATE_RATE_FACTOR_MAX )
    return -1; // invalid factor

  MIOS32_IRQ_Disable();

  cv_se_speed_factor = factor;
  mbCvEnvironment.updateSpeedFactorSet(cv_se_speed_factor);

  // start timer
  MIOS32_TIMER_Init(2, 2000 / cv_se_speed_factor, CV_TIMER_SE_Update, MIOS32_IRQ_PRIO_MID);

  // clear overload indicators
  cv_se_overloaded = 0;
  cv_se_overload_check_ctr = 0;
  cv_se_last_mios32_timestamp = MIOS32_TIMESTAMP_Get();

  MIOS32_IRQ_Enable();

  return 0; // no error
}

s32 APP_CvUpdateRateFactorGet(void)
{
  return cv_se_speed_factor;
}

/////////////////////////////////////////////////////////////////////////////
// Returns 1 if engine overloaded
/////////////////////////////////////////////////////////////////////////////
s32 APP_CvUpdateOverloadStatusGet(void)
{
  return cv_se_overloaded;
}


/////////////////////////////////////////////////////////////////////////////
// For performance Measurements
/////////////////////////////////////////////////////////////////////////////
s32 APP_StopwatchInit(void)
{
  stopwatch_value = 0;
  stopwatch_value_min = ~0;
  stopwatch_value_max = 0;
  stopwatch_value_accumulated = 0;
  stopwatch_value_accumulated_ctr = 0;
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
  if( value < stopwatch_value_min )
    stopwatch_value_min = value;
  if( value > stopwatch_value_max )
    stopwatch_value_max = value;
  stopwatch_value_accumulated += value;
  ++stopwatch_value_accumulated_ctr;
  MIOS32_IRQ_Enable();

  return 0; // no error
}

