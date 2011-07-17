// $Id$
/*
 * EmbSpeech Demo for MIOS32
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
#include "app.h"
#include "synth.h"
#include "tasks.h"

#include <notestack.h>

// include source of the SCS
#include <scs.h>
#include "scs_config.h"

#include "file.h"
#include "synth_file.h"
#include "synth_file_b.h"

// define priority level for task:
// use same priority as MIOS32 specific tasks (3)
#define PRIORITY_TASK_PERIODIC_1MS   ( tskIDLE_PRIORITY + 3 )

// local prototype of the task function
static void TASK_Periodic_1mS(void *pvParameters);

// define priority level for control surface handler
// use lower priority as MIOS32 specific tasks (2), so that slow LCDs don't affect overall performance
#define PRIORITY_TASK_PERIOD_1mS_LP ( tskIDLE_PRIORITY + 2 )

// local prototype of the task function
static void TASK_Period_1mS_LP(void *pvParameters);

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NOTESTACK_SIZE 16

// C-2
#define PHRASE_NOTE_OFFSET 0x30

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static notestack_t notestack;
static notestack_item_t notestack_items[NOTESTACK_SIZE];


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

// for mutual exclusive SD Card access between different tasks
// The mutex is handled with MUTEX_SDCARD_TAKE and MUTEX_SDCARD_GIVE
// macros inside the application, which contain a different implementation 
// for emulation
xSemaphoreHandle xSDCardSemaphore;


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // create semaphores
  xSDCardSemaphore = xSemaphoreCreateRecursiveMutex();

  // initialize the Notestack
  NOTESTACK_Init(&notestack, NOTESTACK_MODE_PUSH_TOP, &notestack_items[0], NOTESTACK_SIZE);

  // init Synth
  SYNTH_Init(0);

  // initialize all J10 pins as inputs with internal Pull-Up
  int pin;
  for(pin=0; pin<8; ++pin)
    MIOS32_BOARD_J10_PinInit(pin, MIOS32_BOARD_PIN_MODE_INPUT_PU);

  // initialize Standard Control Surface
  SCS_Init(0);

  // initialize local SCS configuration
  SCS_CONFIG_Init(0);

  // initialize file system
  SYNTH_FILE_Init(0);

  // start task
  xTaskCreate(TASK_Periodic_1mS, (signed portCHAR *)"Periodic_1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIODIC_1MS, NULL);
  xTaskCreate(TASK_Period_1mS_LP, (signed portCHAR *)"1mS_LP", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS_LP, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // if note event over MIDI channel #1 controls note of both oscillators
  // Note On received?
  if( midi_package.chn == Chn1 && 
      (midi_package.type == NoteOn || midi_package.type == NoteOff) ) {

    // branch depending on Note On/Off event
    if( midi_package.event == NoteOn && midi_package.velocity > 0 ) {
      // push note into note stack
      NOTESTACK_Push(&notestack, midi_package.note, midi_package.velocity);
    } else {
      // remove note from note stack
      NOTESTACK_Pop(&notestack, midi_package.note);
    }


    // still a note in stack?
    if( notestack.len ) {
      // take first note of stack
      u8 note = notestack_items[0].note;
      u8 velocity = notestack_items[0].tag;

#if 0
      // set frequency for both oscillators
      int chn;
      for(chn=0; chn<2; ++chn) {
	SYNTH_FrequencySet(chn, frqtab[note]);
	SYNTH_VelocitySet(chn, velocity);
      }
#endif

      // play note
      int phrase_num = note - PHRASE_NOTE_OFFSET;
      while( phrase_num < 0)
	phrase_num += SYNTH_NUM_PHRASES;
      while( phrase_num > SYNTH_NUM_PHRASES )
	phrase_num -= SYNTH_NUM_PHRASES;

      // legato...
      if( notestack.len == 1 )
	SYNTH_PhraseStop(phrase_num);

      SYNTH_PhrasePlay(phrase_num, velocity);
      
      // set board LED
      MIOS32_BOARD_LED_Set(1, 1);
    } else {
      // turn off LED (can also be used as a gate output!)
      MIOS32_BOARD_LED_Set(1, 0);

#if 0
      // set velocity to 0 for all oscillators
      int chn;
      for(chn=0; chn<2; ++chn)
	SYNTH_VelocitySet(chn, 0x00);
#endif
    }

#if 0
    // optional debug messages
    NOTESTACK_SendDebugMessage(&notestack);
#endif
  } else if( midi_package.type == CC ) {
    u8 phrase_num = 0;
    u8 phoneme_ix = midi_package.chn;
    u32 value = midi_package.value;

    if( midi_package.cc_number < 16 ) {
    } else if( midi_package.cc_number < 32 ) {
      u8 phoneme_par = midi_package.cc_number - 16;
      SYNTH_PhonemeParSet(phrase_num, phoneme_ix, phoneme_par, value);
    } else if( midi_package.cc_number < 48 ) {
      u8 phrase_par = midi_package.cc_number - 32;
      SYNTH_PhraseParSet(phrase_num, phrase_par, value);
    } else if( midi_package.cc_number < 64 ) {
      u8 global_par = midi_package.cc_number - 48;
      SYNTH_GlobalParSet(global_par, value);
    }
  }

}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
#ifdef MIOS32_FAMILY_LPC17xx
  // pass current pin state of J10
  // only available for LPC17xx, all others (like STM32) default to SRIO
  SCS_AllPinsSet(0xff00 | MIOS32_BOARD_J10_Get());
#endif

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
  // for STM32 (no J10 available, accordingly encoder/buttons connected to DIN):
  // if DIN pin number between 0..7: pass to SCS
  if( pin < 8 )
    SCS_DIN_NotifyToggle(pin, pin_value);
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
// This task scans J5 pins periodically
/////////////////////////////////////////////////////////////////////////////
static void TASK_Periodic_1mS(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    SYNTH_Tick();
  }
}

/////////////////////////////////////////////////////////////////////////////
// This task handles the control surface
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period_1mS_LP(void *pvParameters)
{
  u16 sdcard_check_ctr = 0;

  MIOS32_LCD_Clear();

  while( 1 ) {
    vTaskDelay(1 / portTICK_RATE_MS);

    // call SCS handler
    SCS_Tick();

    // each second: check if SD Card (still) available
    if( ++sdcard_check_ctr >= 1000 ) {
      sdcard_check_ctr = 0;

      MUTEX_SDCARD_TAKE;
      s32 status = FILE_CheckSDCard();

      if( status == 1 ) {
	DEBUG_MSG("SD Card connected: %s\n", FILE_VolumeLabel());
	// load all file infos
	SYNTH_FILE_LoadAllFiles(1); // including HW info
      } else if( status == 2 ) {
	DEBUG_MSG("SD Card disconnected\n");
	// invalidate all file infos
	SYNTH_FILE_UnloadAllFiles();
      } else if( status == 3 ) {
	if( !FILE_SDCardAvailable() ) {
	  DEBUG_MSG("SD Card not found\n");
	} else if( !FILE_VolumeAvailable() ) {
	  DEBUG_MSG("ERROR: SD Card contains invalid FAT!\n");
	} else {
	  int bank;
	  for(bank=0; bank<SYNTH_FILE_B_NUM_BANKS; ++bank) {
	    u8 numPatches = SYNTH_FILE_B_NumPatches(bank);

	    if( numPatches ) {
	      DEBUG_MSG("Bank #%d contains %d patches\n", bank+1, numPatches);
	    } else {
	      DEBUG_MSG("Bank #%d not found - creating new one\n", bank+1);
	      if( (status=SYNTH_FILE_B_Create(bank)) < 0 ) {
		DEBUG_MSG("Failed to create Bank #%d (status: %d)\n", bank+1, status);
	      } else {
		DEBUG_MSG("Bank #%d successfully created!\n", bank+1);

		int numPatches = SYNTH_FILE_B_NumPatches(bank);
		int patch;
		for(patch=0; patch<numPatches; ++patch) {
		  DEBUG_MSG("Writing Bank %d Patch #%d\n", bank+1, patch+1);
		  MIOS32_LCD_CursorSet(0, 0); // TMP - use message system later
		  MIOS32_LCD_PrintFormattedString("Write Patch %d.%03d  ", bank+1, patch+1);
		  MIOS32_LCD_CursorSet(0, 1); // TMP - use message system later
		  MIOS32_LCD_PrintFormattedString("Please wait...      ");
		  u8 sourceGroup = 0;
		  u8 rename_if_empty_name = 0;
		  if( (status=SYNTH_FILE_B_PatchWrite(bank, patch, sourceGroup, rename_if_empty_name)) < 0 ) {
		    DEBUG_MSG("Failed to write patch #%d into bank #%d (status: %d)\n", patch+1, bank+1, status);
		  }
		}

		SCS_DisplayUpdateRequest();
	      }
	    }
	  }
	  status = SYNTH_FILE_UnloadAllFiles();
	  status = SYNTH_FILE_LoadAllFiles(1);
	  if( status < 0 ) {
	    DEBUG_MSG("Failed to load the newly created files!\n");
	  } else {
	    u8 initialBank = 0;
	    u8 initialPatch = 0;
	    u8 targetGroup = 0;
	    if( (status=SYNTH_FILE_B_PatchRead(initialBank, initialPatch, targetGroup)) < 0 ) {
	      char buffer[100];
	      sprintf(buffer, "Patch %c%03d", 'A'+initialBank, initialPatch+1);
	      SCS_Msg(SCS_MSG_ERROR_L, 1000, "Failed to read", buffer);
	    } else {
	      //    char buffer[100];
	      //    sprintf(buffer, "Patch %c%03d", 'A'+initialBank, initialPatch+1);
	      //    SCS_Msg(SCS_MSG_L, 1000, buffer, "read!");
	    }
	  }
	}
      }

      MUTEX_SDCARD_GIVE;
    }
  }

}

