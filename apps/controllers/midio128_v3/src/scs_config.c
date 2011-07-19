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

#include "midio_file.h"
#include "midio_file_p.h"
#include "midio_patch.h"



/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local parameter variables
/////////////////////////////////////////////////////////////////////////////


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

static void selectSAVE_Callback(char *newString)
{
  s32 status;

  if( (status=MIDIO_PATCH_Store(newString)) < 0 ) {
    char buffer[100];
    sprintf(buffer, "Patch '%s'", newString);
    SCS_Msg(SCS_MSG_ERROR_L, 1000, "Failed to store", buffer);
  } else {
    char buffer[100];
    sprintf(buffer, "Patch %s", newString);
    SCS_Msg(SCS_MSG_L, 1000, buffer, "stored!");
  }
}
static u16  selectSAVE(u32 ix, u16 value)
{
  return SCS_InstallEditStringCallback(selectSAVE_Callback, "SAVE", midio_file_p_patch_name, MIDIO_FILE_P_FILENAME_LEN);
}

static void selectLOAD_Callback(char *newString)
{
  s32 status;

  if( (status=MIDIO_PATCH_Load(newString)) < 0 ) {
    char buffer[100];
    sprintf(buffer, "Patch %s", newString);
    SCS_Msg(SCS_MSG_ERROR_L, 1000, "Failed to load", buffer);
  } else {
    char buffer[100];
    sprintf(buffer, "Patch '%s'", newString);
    SCS_Msg(SCS_MSG_L, 1000, buffer, "loaded!");
  }
}
static u16  selectLOAD(u32 ix, u16 value)
{
  return SCS_InstallEditStringCallback(selectLOAD_Callback, "LOAD", midio_file_p_patch_name, MIDIO_FILE_P_FILENAME_LEN);
}


/////////////////////////////////////////////////////////////////////////////
// Parameter Access Functions
/////////////////////////////////////////////////////////////////////////////
static u16  dummyGet(u32 ix)              { return 0; }
static void dummySet(u32 ix, u16 value)   { }



/////////////////////////////////////////////////////////////////////////////
// Special output functions for Disk page
/////////////////////////////////////////////////////////////////////////////
void pageDskStringLine1(u8 editMode, char *line)
{
  sprintf(line, "Patch: %s", midio_file_p_patch_name);
}

void pageDskStringLine2(u8 editMode, char *line)
{
  sprintf(line, "Load Save");
}

/////////////////////////////////////////////////////////////////////////////
// Menu Structure
/////////////////////////////////////////////////////////////////////////////

const scs_menu_item_t pageDsk[] = {
  SCS_ITEM("Load", 0, 0,           dummyGet,        dummySet,        selectLOAD, stringEmpty, NULL),
  SCS_ITEM("Save", 0, 0,           dummyGet,        dummySet,        selectSAVE, stringEmpty, NULL),
};

const scs_menu_page_t rootMode0[] = {
  SCS_PAGE("Dsk ", pageDsk, pageDskStringLine1, pageDskStringLine2),
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
  sprintf(line1, "Patch: %s", midio_file_p_patch_name);

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
