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
#include <string.h>

#include <scs.h>
#include "scs_config.h"
#include "cc_labels.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////
#define NUM_KNOBS        10
#define DEMO_STRING_LEN  16

/////////////////////////////////////////////////////////////////////////////
// Local parameter variables
/////////////////////////////////////////////////////////////////////////////

static u8 extraPage;

static u8 selectedKnob;

static u8 knobCC[NUM_KNOBS];
static u8 knobChn[NUM_KNOBS];
static u8 knobValue[NUM_KNOBS];

static char demoString[DEMO_STRING_LEN+1]; // +1 char for 0 terminator


/////////////////////////////////////////////////////////////////////////////
// String Conversion Functions
/////////////////////////////////////////////////////////////////////////////
static void stringEmpty(u32 ix, u16 value, char *label)  { label[0] = 0; }
static void stringDec(u32 ix, u16 value, char *label)    { sprintf(label, "%3d  ", value); }
static void stringDecP1(u32 ix, u16 value, char *label)  { sprintf(label, "%3d  ", value+1); }
static void stringDecPM(u32 ix, u16 value, char *label)  { sprintf(label, "%3d  ", (int)value - 64); }
static void stringDec03(u32 ix, u16 value, char *label)  { sprintf(label, "%03d  ", value); }
static void stringHex2(u32 ix, u16 value, char *label)    { sprintf(label, " %02x  ", value); }

static void stringCCFull(u32 ix, u16 value, char *line1, char *line2)
{
  // print CC parameter on full screen (2x20)
  sprintf(line1, "CC Assignment Knob#%d", selectedKnob);
  sprintf(line2, "%s    (CC#%03d)", CC_LABELS_Get(value), value);
}


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
  SCS_ITEM("Knob ", 0, NUM_KNOBS-1, knobGet,         knobSet,         selectNOP, stringDecP1,  NULL),
  SCS_ITEM("Chn. ", 0, 0x0f,        selKnobChnGet,   selKnobChnSet,   selectNOP, stringDecP1,  NULL),
  SCS_ITEM("CC # ", 0, 0x7f,        selKnobCCGet,    selKnobCCSet,    selectNOP, stringDec03, stringCCFull),
  SCS_ITEM("Val. ", 0, 0x7f,        selKnobValueGet, selKnobValueSet, selectNOP, stringDec,    NULL),
};

const scs_menu_item_t pageKnb[] = {
  SCS_ITEM(" K 1 ",  0, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec03, NULL),
  SCS_ITEM(" K 2 ",  1, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec03, NULL),
  SCS_ITEM(" K 3 ",  2, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec03, NULL),
  SCS_ITEM(" K 4 ",  3, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec03, NULL),
  SCS_ITEM(" K 5 ",  4, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec03, NULL),
  SCS_ITEM(" K 6 ",  5, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec03, NULL),
  SCS_ITEM(" K 7 ",  6, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec03, NULL),
  SCS_ITEM(" K 8 ",  7, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec03, NULL),
  SCS_ITEM(" K 9 ",  8, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec03, NULL),
  SCS_ITEM(" K10 ",  9, 0x7f,      knobValueGet,    knobValueSet,    selectNOP, stringDec03, NULL),
};

const scs_menu_item_t pageMsg[] = {
  SCS_ITEM("Msg1 ", 0, 1, dummyGet, dummySet, selectMsg1, stringEmpty,  NULL),
  SCS_ITEM("Msg2 ", 0, 1, dummyGet, dummySet, selectMsg2, stringEmpty,  NULL),
  SCS_ITEM("Msg3 ", 0, 1, dummyGet, dummySet, selectMsg3, stringEmpty,  NULL),
  SCS_ITEM("Msg4 ", 0, 1, dummyGet, dummySet, selectMsg4, stringEmpty,  NULL),
  SCS_ITEM("Msg5 ", 0, 1, dummyGet, dummySet, selectMsg5, stringEmpty,  NULL),
  SCS_ITEM("Msg6 ", 0, 1, dummyGet, dummySet, selectMsg6, stringEmpty,  NULL),
  SCS_ITEM("Str. ", 0, 1, dummyGet, dummySet, selectString, stringEmpty,  NULL),
};

const scs_menu_item_t pageSubA[] = {
  SCS_ITEM("Msg1 ", 0, 1, dummyGet, dummySet, selectMsg1, stringEmpty,  NULL),
};

const scs_menu_item_t pageSubB[] = {
  SCS_ITEM("Msg2 ", 0, 1, dummyGet, dummySet, selectMsg2, stringEmpty,  NULL),
};

const scs_menu_item_t pageSubC[] = {
  SCS_ITEM("Msg3 ", 0, 1, dummyGet, dummySet, selectMsg3, stringEmpty,  NULL),
};

const scs_menu_item_t pageSubD[] = {
  SCS_ITEM("Msg4 ", 0, 1, dummyGet, dummySet, selectMsg4, stringEmpty,  NULL),
};

const scs_menu_item_t pageSubE[] = {
  SCS_ITEM("Msg5 ", 0, 1, dummyGet, dummySet, selectMsg5, stringEmpty,  NULL),
};

const scs_menu_item_t pageSubF[] = {
  SCS_ITEM("Msg6 ", 0, 1, dummyGet, dummySet, selectMsg6, stringEmpty,  NULL),
  SCS_ITEM("Str. ", 0, 1, dummyGet, dummySet, selectString, stringEmpty,  NULL),
};

const scs_menu_page_t subpageDeep[] = {
  SCS_PAGE("SubA ", pageSubA),
  SCS_PAGE("SubB ", pageSubB),
  SCS_PAGE("SubC ", pageSubC),
  SCS_PAGE("SubD ", pageSubD),
  SCS_PAGE("SubE ", pageSubE),
  SCS_PAGE("SubF ", pageSubF),
};

const scs_menu_page_t rootMode0[] = {
  SCS_PAGE("Knob ", pageKnb),
  SCS_PAGE("Cfg. ", pageCfg),
  SCS_PAGE("Msg. ", pageMsg),
  SCS_SUBPAGE("Deep ", subpageDeep),
};


/////////////////////////////////////////////////////////////////////////////
// This function can overrule the display output
// If it returns 0, the original SCS output will be print
// If it returns 1, the output copied into line1 and/or line2 will be print
// If a line is not changed (line[0] = 0 or line[1] = 0), the original output
// will be displayed - this allows to overrule only a single line
/////////////////////////////////////////////////////////////////////////////
static s32 displayHook(char *line1, char *line2)
{
  if( extraPage ) {
    sprintf(line1, "Extra Page");
    sprintf(line2, "Msg1 Msg2 Msg3 Msg4 ");
    return 1;
  } else if( SCS_MenuStateGet() == SCS_MENU_STATE_MAINPAGE ) {
    sprintf(line1, "Main Page");
    sprintf(line2, "Press soft button");
    return 1;
  } else if( SCS_MenuStateGet() == SCS_MENU_STATE_SELECT_PAGE ) {
    sprintf(line1, "Select Page:");
    return 1;
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function is called when the rotary encoder is moved
// If it returns 0, the encoder increment will be handled by the SCS
// If it returns 1, the SCS will ignore the encoder
/////////////////////////////////////////////////////////////////////////////
static s32 encHook(s32 incrementer)
{
  if( extraPage ) {
    MIOS32_MIDI_SendDebugMessage("Encoder moved in extra page: %d\n", incrementer);
    return 1;
  } else if( SCS_MenuStateGet() == SCS_MENU_STATE_MAINPAGE ) {
    MIOS32_MIDI_SendDebugMessage("Encoder moved in main page: %d\n", incrementer);
    return 1;
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function is called when a button has been pressed or depressed
// If it returns 0, the button movement will be handled by the SCS
// If it returns 1, the SCS will ignore the button event
/////////////////////////////////////////////////////////////////////////////
static s32 buttonHook(u8 scsButton, u8 depressed)
{
  if( extraPage ) {
    if( scsButton == SCS_PIN_SOFT5 && depressed ) // selects/deselects extra page
      extraPage = 0;
    else {
      if( !depressed )
	switch( scsButton ) {
	case SCS_PIN_SOFT1: SCS_Msg(SCS_MSG_L, 1000, "Soft Button", "#1 pressed"); break;
#if !SCS_MENU_NO_SOFT_BUTTON_MODE
	case SCS_PIN_SOFT2: SCS_Msg(SCS_MSG_L, 1000, "Soft Button", "#2 pressed"); break;
	case SCS_PIN_SOFT3: SCS_Msg(SCS_MSG_L, 1000, "Soft Button", "#3 pressed"); break;
	case SCS_PIN_SOFT4: SCS_Msg(SCS_MSG_L, 1000, "Soft Button", "#4 pressed"); break;
#endif
	}
    }
    return 1;
  } else {
    if( scsButton == SCS_PIN_SOFT5 && !depressed ) { // selects/deselects extra page
      extraPage = 1;
      return 1;
    }
  }

  return 0;
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
    extraPage = 0;

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
    SCS_InstallDisplayHook(displayHook);
    SCS_InstallEncHook(encHook);
    SCS_InstallButtonHook(buttonHook);
    break;
  }
  default: return -1; // mode not supported
  }

  return 0; // no error
}
