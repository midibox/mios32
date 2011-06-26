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

#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include <notestack.h>

#include "mcu_otherdef.h"
#include "mcu_phoneme.h"
#include "mcu_synthesize.h"


// define priority level for task:
// use same priority as MIOS32 specific tasks (3)
#define PRIORITY_TASK_PERIODIC_1MS   ( tskIDLE_PRIORITY + 3 )

// local prototype of the task function
static void TASK_Periodic_1mS(void *pvParameters);



const char HelloMsg[] = "Circuit Cellar and Luminary Micro are pleased to offer design engineers an incredible contest opportunity called Design Stellaris 2006 And with Circuit Cellar magazine they also have the 1 venue for peer recognition of their winning applications";

volatile char* p_cgen = NULL;
int MCUTranslate(char* src);
int MCUPhoneticString(char* dest);
void MCUCalcPitches(int clause_tone);
void MCUCalcLengths(void);

void MCU_Speak(char* src){
        if(NULL != p_cgen) return;
        p_cgen = (char*) src;
        MCUTranslate((char*) p_cgen);
        MCUCalcPitches(1);
        MCUCalcLengths();
}



/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NOTESTACK_SIZE 16


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static notestack_t notestack;
static notestack_item_t notestack_items[NOTESTACK_SIZE];


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize the Notestack
  NOTESTACK_Init(&notestack, NOTESTACK_MODE_PUSH_TOP, &notestack_items[0], NOTESTACK_SIZE);

  // init speak
  MCU_WavegenInit();
  MCU_SynthesizeInit();
  MCU_Speak((char*) HelloMsg);

  // init Synth
  SYNTH_Init(0);

  // start task
  xTaskCreate(TASK_Periodic_1mS, (signed portCHAR *)"Periodic_1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIODIC_1MS, NULL);
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

      MCU_Speak((char*) HelloMsg);

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
  }
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
// This task scans J5 pins periodically
/////////////////////////////////////////////////////////////////////////////
static void TASK_Periodic_1mS(void *pvParameters)
{
  static int gen_resume = 0;
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    SYNTH_Tick();

    int trans=0;
    gen_resume = MCU_Generate(MCU_phoneme_list,&MCU_n_phoneme_list,gen_resume);
    if(0 == gen_resume){
      if(p_cgen != NULL){
	//p_cgen = MCUTranslate((char*) p_cgen);
	trans = MCUTranslate(NULL);
	MCUCalcPitches(1);
	MCUCalcLengths();
	//num_phoneme = MCU_n_phoneme_list;
	gen_resume = MCU_Generate(MCU_phoneme_list,&MCU_n_phoneme_list,0);
	if( (trans ==0) && (0 == gen_resume) /*&& (get_sound_state() )*/ )
	  p_cgen = NULL;
      }
    }
  }
}
