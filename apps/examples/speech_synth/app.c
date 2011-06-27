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
MCU_PHONEME_LIST* g_ph_list;
int num_phoneme;
int gen_resume;

int MCUTranslate(char* src);
int MCUPhoneticString(char* dest);
void MCUCalcPitches(int clause_tone);
void MCUCalcLengths(void);

void MCU_Speak(char* src)
{
  if(NULL != p_cgen) return;
  p_cgen = (char*) src;
  MCUTranslate((char*) p_cgen);
  MCUCalcPitches(1);
  MCUCalcLengths();
}

int MCU_SpeakPhoneme(MCU_PHONEME_LIST* ph_list, int num)
{
  num_phoneme = num;
  gen_resume = 0;
  g_ph_list = ph_list;
  return (1);
}

//play list phoneme
const MCU_PHONEME_LIST phoneme_default[43]={
 { 10,   1,   0,   0,   0,  20,   0, 205,    25,   23,   29,    0},
 { 67,   0,   6,   6,  15,  20,   5,   0,   256,    0,    0,14337},
 { 94,   0,   6,   2,   0,  24,   0,   4,   168,   39,   47,    0},
 { 59,   0,   0,   4,  40,  20,   0,   0,     0,    0,    0,    0},
 { 88,   0,   0,   2,   0,  16,   0,   4,   133,   35,   39,    0},
 { 50,   0,   8,   4,  60,  20,   0,   0,     0,    0,    0,    0},
 { 67,   0,   8,   6,  15,  20,   1,   0,   256,    0,    0,12297},
 { 87,   4,   8,   2,   0,  22,   0,   4,   229,   11,   41,    0},
 { 33,   1,   0,   3,   0,  15,   0,   0,   235,   14,   17,    0},
 { 13,   1,   0,   2,   0,  15,   0,   4,   235,   17,   29,    0},
 {  9,   0,  12,   0,   0,   0,   2,   0,   200,    0,    0,    0},
 {  9,   0,  12,   0,   0,   0,   0,   0,     0,    0,    0,    0},
 { 10,   1,   0,   0,   0,  20,   0, 205,    25,   23,   29,    0},
 { 33,   1,   6,   3,   0,  23,   1,   0,   155,   37,   46,16385},
 { 99,   0,   6,   2,   0,  23,   0,   4,   155,   39,   47,    0},
 { 42,   0,   1,   8,   0,  15,   0,   0,    87,   39,   38,    0},
 { 88,   0,   1,   2,   0,  15,   0,   4,    87,   35,   39,    0},
 { 43,   1,   0,   8,   0,  15,   0,   0,   123,   35,   42,    0},
 { 13,   0,   0,   2,   0,  15,   0,   4,   123,   39,   43,    0},
 { 30,   1,   0,   3,   0,  15,   0,   0,   146,   39,   42,    0},
 { 81,   0,   0,   2,   0,  15,   0,   4,   146,   39,   43,    0},
 { 42,   0,   6,   8,  12,  18,   1,   0,   168,   39,   39,10250},
 {102,   0,   6,   2,   0,  23,   0,   4,   168,   32,   40,    0},
 { 59,   0,   0,   4,  40,  20,   0,   0,     0,    0,    0,    0},
 { 30,   1,   0,   3,   0,  15,   0,   0,   200,   22,   31,    0},
 {101,   0,   0,   2,   0,  15,   0,   4,   200,   28,   32,    0},
 {  9,   0,   1,   0,   0,  20,   1,   0,    75,    0,    0, 6160},
 {  9,   0,   1,   0,   0,  20,   0,   0,    75,    0,    0,    0},
 { 85,   0,   1,   2,   0,  16,   0,  20,   108,   32,   36,    0},
 { 43,   0,   6,   8,   0,  16,   0,  17,   172,   24,   32,    0},
 { 49,   0,   6,   5,   0,  20,   0,  16,   128,    0,    0,    0},
 { 59,   0,   6,   4,  60,  20,   1,   0,     0,    0,    0, 8212},
 {103,   0,   6,   2,   0,  24,   0,   4,   182,   25,   33,    0},
 { 35,   0,   8,   3,   0,  16,   0,   1,   121,    5,   25,    0},
 { 59,   0,   8,   4,  60,  20,   1,   0,     0,    0,    0,14361},
 { 90,   0,   8,   2,   0,  22,   0,   4,   182,    8,   34,    0},
 { 43,   0,   0,   8,   0,  16,   0,   1,   182,    0,    8,    0},
 { 50,   0,   0,   4,  40,  20,   0,   0,     0,    0,    0,    0},
 { 87,   0,   0,   2,   0,  16,   0,   4,   165,    9,   13,    0},
 { 67,   0,  12,   6,   0,  20,   0,   0,   200,    0,    0,    0},
 { 50,   0,  12,   4,  20,  20,   0,   0,     0,    0,    0,    0},
 {  9,   0,  12,   0,   0,   0,   2,   0,    10,    0,    0,    0},
 {  9,   0,  12,   0,   0,   0,   0,   0,     0,    0,    0,    0},
};



// modified via CC
// NOTE: at least 3 entries have to be played, we reserve for up to 16 entries
#define PHONEME_CC_MAX 16
static MCU_PHONEME_LIST phoneme_cc[PHONEME_CC_MAX];
static u8 phoneme_cc_num_played;

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


  // init CC phonemes
  int i;
  MCU_PHONEME_LIST *p = (MCU_PHONEME_LIST *)&phoneme_cc[0];
  for(i=0; i<PHONEME_CC_MAX; ++i, ++p) {
    p->ph = 88;
    p->env = 0;
    p->tone = 0;
    p->type = 2;
    p->prepause = 0;
    p->amp = 16;
    p->newword = 0;
    p->synthflags = 4;
    p->length = 133;
    p->pitch1 = 35;
    p->pitch2 = 39;
    p->sourceix = 0;
  }
  phoneme_cc_num_played = 3; // at least 3 entries have to be played

  // init speak
  MCU_WavegenInit();
  MCU_SynthesizeInit();
  // MCU_Speak((char*) HelloMsg);

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

      //MCU_Speak((char*) HelloMsg);
      //MCU_SpeakPhoneme(phoneme_default, 42);
      MCU_SynthesizeInit();
      MCU_SpeakPhoneme(phoneme_cc, phoneme_cc_num_played);
      
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
    int phoneme_ix = midi_package.chn;
    u8 value = midi_package.value;
    if( phoneme_ix >= PHONEME_CC_MAX ) // just to ensure, is 16...
      return;
    MCU_PHONEME_LIST *p = (MCU_PHONEME_LIST *)&phoneme_cc[phoneme_ix];

    switch( midi_package.cc_number ) {
    case 16: p->ph = value; break;
    case 17: p->env = value; break;
    case 18: p->tone = value; break;
    case 19: p->type = value; break;
    case 20: p->prepause = value; break;
    case 21: p->amp = value; break;
    case 22: p->newword = value; break;
    case 23: p->synthflags = value; break;
    case 24: p->length = 64*value; break;
    case 25: p->pitch1 = value; break;
    case 26: p->pitch2 = value; break;

    case 32: phoneme_cc_num_played = (value >= PHONEME_CC_MAX) ? (PHONEME_CC_MAX-1) : value; break;
    case 33: synth_downsampling_factor = value; break;
    case 34: synth_resolution = (value >= 16) ? 16 : value; break;
    case 35: synth_xor = ((u16)value << 9); break;
    }
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
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    SYNTH_Tick();

    if( g_ph_list ) {
      // Local phoneme List
      gen_resume = MCU_Generate(g_ph_list, &num_phoneme, gen_resume);
      if( !gen_resume )
	g_ph_list = NULL;
    } else {
      // Translated phoneme list
      int trans=0;
      gen_resume = MCU_Generate(MCU_phoneme_list, &MCU_n_phoneme_list, gen_resume);
      if( !gen_resume ) {
	if( p_cgen != NULL ) {
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
}
