// $Id$
/*
 * Local SCS Configuration
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
#include "tasks.h"
#include <string.h>

#include <scs.h>
#include "scs_config.h"

#include <seq_bpm.h>
#include "seq.h"
#include "mid_file.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////
#define NUM_KNOBS        10
#define DEMO_STRING_LEN  16

/////////////////////////////////////////////////////////////////////////////
// Local parameter variables
/////////////////////////////////////////////////////////////////////////////

static u8 selectedKnob;

static u8 knobCC[NUM_KNOBS];
static u8 knobChn[NUM_KNOBS];
static u8 knobValue[NUM_KNOBS];

static char demoString[DEMO_STRING_LEN+1]; // +1 char for 0 terminator


/////////////////////////////////////////////////////////////////////////////
// String Conversion Functions
/////////////////////////////////////////////////////////////////////////////
static void stringEmpty(u32 ix, u16 value, char *label)  { label[0] = 0; }
static void stringDec(u32 ix, u16 value, char *label)    { sprintf(label, "%3d ", value); }
static void stringDecP1(u32 ix, u16 value, char *label)  { sprintf(label, "%3d ", value+1); }
static void stringDec000(u32 ix, u16 value, char *label) { sprintf(label, "%03d ", value); }


/////////////////////////////////////////////////////////////////////////////
// Parameter Selection Functions
/////////////////////////////////////////////////////////////////////////////
static u16  selectNOP(u32 ix, u16 value)    { return value; }

// message demos
static u16 selectMsg1(u32 ix, u16 value)
{
  SCS_Msg(SCS_MSG_L, 1000, "Short", "Message");
  return value; // no change on value
}

static u16 selectMsg2(u32 ix, u16 value)
{
  SCS_Msg(SCS_MSG_L, 3000, "Long", "Message");
  return value; // no change on value
}

static u16 selectMsg3(u32 ix, u16 value)
{
  char buffer[100];
  sprintf(buffer, "Knob #%d", selectedKnob+1);
  SCS_Msg(SCS_MSG_L, 1000, "Parameters:", buffer);
  return value; // no change on value
}

static u16 selectMsg4(u32 ix, u16 value)
{
  SCS_Msg(SCS_MSG_R, 1000, "Right", "Aligned");
  return value; // no change on value
}

static u16 selectMsg5(u32 ix, u16 value)
{
  SCS_Msg(SCS_MSG_ERROR_L, 1000, "ERROR", "42");
  return value; // no change on value
}


static void demoCallback(u32 parameter)
{
  char buffer[100];
  sprintf(buffer, "0x%08x", parameter);
  SCS_Msg(SCS_MSG_L, 2000, "Got parameter:", buffer);
}
static u16 selectMsg6(u32 ix, u16 value)
{
  // if upper line empty: it will show the seconds how long a button has to be pressed
  SCS_Msg(SCS_MSG_DELAYED_ACTION_L, 3001, "", "for demo");
  u32 parameter = 0x1234567; // to pass an optional parameter
  SCS_InstallDelayedActionCallback(demoCallback, 3000, parameter);
  return value; // no change on value
}

static void selectString_Callback(char *newString)
{
  // could be the name of a patch which should be stored
  memcpy(demoString, newString, DEMO_STRING_LEN);
  SCS_Msg(SCS_MSG_L, 2000, "You entered:", demoString);
  // call SD Card store function here
}

static u16  selectString(u32 ix, u16 value)
{
  return SCS_InstallEditStringCallback(selectString_Callback, "SAVE", demoString, DEMO_STRING_LEN);
}


/////////////////////////////////////////////////////////////////////////////
// Parameter Access Functions
/////////////////////////////////////////////////////////////////////////////
static u16  dummyGet(u32 ix)              { return 0; }
static void dummySet(u32 ix, u16 value)   { }

static u16  knobGet(u32 ix)              { return selectedKnob; }
static void knobSet(u32 ix, u16 value)   { selectedKnob = value; }

static u16  knobValueGet(u32 ix)               { return knobValue[ix]; }
static void knobValueSet(u32 ix, u16 value)
{
  if( knobValue[ix] != value ) {
    knobValue[ix] = value;
    MIOS32_MIDI_SendCC(USB0,  knobChn[ix], knobCC[ix], knobValue[ix]);
    MIOS32_MIDI_SendCC(UART0, knobChn[ix], knobCC[ix], knobValue[ix]);
  }
}

static u16  selKnobCCGet(u32 ix)               { return knobCC[selectedKnob]; }
static void selKnobCCSet(u32 ix, u16 value)    { knobCC[selectedKnob] = value; }

static u16  selKnobChnGet(u32 ix)              { return knobChn[selectedKnob]; }
static void selKnobChnSet(u32 ix, u16 value)   { knobChn[selectedKnob] = value; }

static u16  selKnobValueGet(u32 ix)            { return knobValue[selectedKnob]; }
static void selKnobValueSet(u32 ix, u16 value) { knobValueSet(selectedKnob, value); } // re-use, will send CC


/////////////////////////////////////////////////////////////////////////////
// Menu Structure
/////////////////////////////////////////////////////////////////////////////

const scs_menu_item_t pageCfg[] = {
  SCS_ITEM("Knb ", 0, NUM_KNOBS-1, knobGet,         knobSet,         selectNOP, stringDecP1,  NULL),
  SCS_ITEM("Chn ", 0, 0x0f,        selKnobChnGet,   selKnobChnSet,   selectNOP, stringDecP1,  NULL),
  SCS_ITEM("Val ", 0, 0x7f,        selKnobValueGet, selKnobValueSet, selectNOP, stringDec,    NULL),
};

const scs_menu_item_t pageKnb[] = {
  SCS_ITEM("K 1 ",  0, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec000, NULL),
  SCS_ITEM("K 2 ",  1, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec000, NULL),
  SCS_ITEM("K 3 ",  2, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec000, NULL),
  SCS_ITEM("K 4 ",  3, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec000, NULL),
  SCS_ITEM("K 5 ",  4, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec000, NULL),
  SCS_ITEM("K 6 ",  5, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec000, NULL),
  SCS_ITEM("K 7 ",  6, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec000, NULL),
  SCS_ITEM("K 8 ",  7, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec000, NULL),
  SCS_ITEM("K 9 ",  8, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec000, NULL),
  SCS_ITEM("K10 ",  9, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec000, NULL),
};

const scs_menu_item_t pageMsg[] = {
  SCS_ITEM(" M1 ", 0, 1, dummyGet, dummySet, selectMsg1, stringEmpty,  NULL),
  SCS_ITEM(" M2 ", 0, 1, dummyGet, dummySet, selectMsg2, stringEmpty,  NULL),
  SCS_ITEM(" M3 ", 0, 1, dummyGet, dummySet, selectMsg3, stringEmpty,  NULL),
  SCS_ITEM(" M4 ", 0, 1, dummyGet, dummySet, selectMsg4, stringEmpty,  NULL),
  SCS_ITEM(" M5 ", 0, 1, dummyGet, dummySet, selectMsg5, stringEmpty,  NULL),
  SCS_ITEM(" M6 ", 0, 1, dummyGet, dummySet, selectMsg6, stringEmpty,  NULL),
  SCS_ITEM("Str ", 0, 1, dummyGet, dummySet, selectString, stringEmpty,  NULL),
};

const scs_menu_page_t rootMode0[] = {
  SCS_PAGE("Knb ", pageKnb),
  SCS_PAGE("Cfg ", pageCfg),
  SCS_PAGE("Msg ", pageMsg),
};


/////////////////////////////////////////////////////////////////////////////
// This function returns the two lines of the main page (2x20 chars)
/////////////////////////////////////////////////////////////////////////////
static s32 getStringMainPage(char *line1, char *line2)
{
  u32 tick = SEQ_BPM_TickGet();
  u32 ticks_per_step = SEQ_BPM_PPQN_Get() / 4;
  u32 ticks_per_measure = ticks_per_step * 16;
  u32 measure = (tick / ticks_per_measure) + 1;
  u32 step = ((tick % ticks_per_measure) / ticks_per_step) + 1;

  sprintf(line1, "%-12s %4u.%2d", MID_FILE_UI_NameGet(), measure, step);
  sprintf(line2, "%s  <   >      MENU", SEQ_BPM_IsRunning() ? "STP" : "PLY");

  // request LCD update - this will lead to fast refresh rate in main screen
  SCS_DisplayUpdateRequest();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called when the rotary encoder is moved in the main page
/////////////////////////////////////////////////////////////////////////////
static s32 encMovedInMainPage(s32 incrementer)
{
  MIOS32_MIDI_SendDebugMessage("Encoder moved in main page: %d\n", incrementer);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called when a soft button is pressed in the main page
// if a value < 0 is returned, the menu won't be entered
/////////////////////////////////////////////////////////////////////////////
static s32 buttonPressedInMainPage(u8 softButton)
{
  switch( softButton ) {
  case 0: // Play/Stop
    if( SEQ_BPM_IsRunning() ) {
      SEQ_BPM_Stop();          // stop sequencer
      seq_pause = 1;
    } else {
      if( seq_pause ) {
	// continue sequencer
	seq_pause = 0;
	SEQ_BPM_Cont();
      } else {
	MUTEX_SDCARD_TAKE;
	// if in auto mode and BPM generator is clocked in slave mode:
	// change to master mode
	SEQ_BPM_CheckAutoMaster();
	// first song
	SEQ_PlayFileReq(0);
	// reset sequencer
	SEQ_Reset(1);
	// start sequencer
	SEQ_BPM_Start();
	MUTEX_SDCARD_GIVE;
      }
    }
    return -1; // don't enter menu

  case 1: { // previous song
    MUTEX_SDCARD_TAKE;
    SEQ_PlayFileReq(-1);
    MUTEX_SDCARD_GIVE;
    return -1; // don't enter menu
  }

  case 2: { // next song
    MUTEX_SDCARD_TAKE;
    SEQ_PlayFileReq(1);
    MUTEX_SDCARD_GIVE;
    return -1; // don't enter menu
  }

  case 3:
    // not assigned yet
    return -1; // don't enter menu
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the upper line on the page selection (20 chars)
/////////////////////////////////////////////////////////////////////////////
static s32 getStringPageSelection(char *line1)
{
  sprintf(line1, "Select Page:");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation of SCS Config
// mode selects the used SCS config (currently only one available selected with 0)
// return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 SCS_CONFIG_Init(u32 mode)
{
  if( mode > 0 )
    return -1;

  switch( mode ) {
  case 0: {
    // default knob assignments
    int i;
    for(i=0; i<NUM_KNOBS; ++i)
      knobCC[i] = 16 + i;

    // default demo string content
    for(i=0; i<DEMO_STRING_LEN; ++i)
      demoString[i] = ' ';
    demoString[DEMO_STRING_LEN] = 0; // 0 terminator

    // install table
    SCS_INSTALL_ROOT(rootMode0);
    SCS_InstallMainPageStringHook(getStringMainPage);
    SCS_InstallPageSelectStringHook(getStringPageSelection);
    SCS_InstallEncMainPageHook(encMovedInMainPage);
    SCS_InstallButtonMainPageHook(buttonPressedInMainPage);
    break;
  }
  default: return -1; // mode not supported
  }

  return 0; // no error
}
