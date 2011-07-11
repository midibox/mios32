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

#include "tasks.h"

#include "synth.h"
#include "synth_file.h"
#include "synth_file_b.h"


/////////////////////////////////////////////////////////////////////////////
// Local parameter variables
/////////////////////////////////////////////////////////////////////////////

static u8 selectedPatch;
static u8 selectedBank;
static u8 selectedPhrase;
static u8 selectedPhonemeIx;

/////////////////////////////////////////////////////////////////////////////
// String Conversion Functions
/////////////////////////////////////////////////////////////////////////////
static void stringEmpty(u32 ix, u16 value, char *label)  { sprintf(label, "    "); }
static void stringDec(u32 ix, u16 value, char *label)    { sprintf(label, "%3d ", value); }
static void stringDecP1(u32 ix, u16 value, char *label)  { sprintf(label, "%3d ", value+1); }
static void stringDecPM(u32 ix, u16 value, char *label)  { sprintf(label, "%3d ", (int)value - 64); }
static void stringDec000(u32 ix, u16 value, char *label) { sprintf(label, "%03d ", value); }
static void stringPlay(u32 ix, u16 value, char *label)   { sprintf(label, " %c  ", SYNTH_PhraseIsPlayed(selectedPhrase) ? '*' : 'o'); }


/////////////////////////////////////////////////////////////////////////////
// Parameter Selection Functions
/////////////////////////////////////////////////////////////////////////////
static u16  selectNOP (u32 ix, u16 value)    { return value; }
static u16  selectPLAY(u32 ix, u16 value)    { return value ? 0 : 1; } // toggle

static u16  selectSAVE(u32 ix, u16 value)
{
  s32 status;
  u8 sourceGroup = 0;
  u8 rename_if_empty_name = 1;
  MUTEX_SDCARD_TAKE;
  if( (status=SYNTH_FILE_B_PatchWrite(synth_file_session_name, selectedBank, selectedPatch, sourceGroup, rename_if_empty_name)) < 0 ) {
    DEBUG_MSG("Failed to write patch #%d into bank #%d (status: %d)\n", selectedPatch+1, selectedBank+1, status);
  } else {
    DEBUG_MSG("Patch #%d written into bank #%d\n", selectedPatch+1, selectedBank+1);
  }
  MUTEX_SDCARD_GIVE;
  return 0;
}

static u16  selectLOAD(u32 ix, u16 value)
{
  s32 status;
  u8 targetGroup = 0;
  MUTEX_SDCARD_TAKE;
  if( (status=SYNTH_FILE_B_PatchRead(selectedBank, selectedPatch, targetGroup)) < 0 ) {
    DEBUG_MSG("Failed to read patch #%d from bank #%d (status: %d)\n", selectedPatch+1, selectedBank+1, status);
  } else {
    DEBUG_MSG("Patch #%d read from bank #%d\n", selectedPatch+1, selectedBank+1);
  }
  MUTEX_SDCARD_GIVE;
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Parameter Access Functions
/////////////////////////////////////////////////////////////////////////////
static u16  dummyGet(u32 ix)              { return 0; }
static void dummySet(u32 ix, u16 value)   { }

static u16  phraseGet(u32 ix)              { return selectedPhrase; }
static void phraseSet(u32 ix, u16 value)   { selectedPhrase = value; }

static u16  phonemeIxGet(u32 ix)              { return selectedPhonemeIx; }
static void phonemeIxSet(u32 ix, u16 value)   { selectedPhonemeIx = value; }

static u16  phonemeParGet(u32 ix)            { return SYNTH_PhonemeParGet(selectedPhrase, selectedPhonemeIx, ix); }
static void phonemeParSet(u32 ix, u16 value) { SYNTH_PhonemeParSet(selectedPhrase, selectedPhonemeIx, ix, value); }

static u16  phraseParGet(u32 ix)             { return SYNTH_PhraseParGet(selectedPhrase, ix); }
static void phraseParSet(u32 ix, u16 value)  { SYNTH_PhraseParSet(selectedPhrase, ix, value); }

static u16  globalGet(u32 ix)            { return SYNTH_GlobalParGet(ix); }
static void globalSet(u32 ix, u16 value) { SYNTH_GlobalParSet(ix, value); }

static u16  playGet(u32 ix)              { return SYNTH_PhraseIsPlayed(selectedPhrase); }
static void playSet(u32 ix, u16 value)
{
  if( value )
    SYNTH_PhrasePlay(selectedPhrase, 0x7f);
  else
    SYNTH_PhraseStop(selectedPhrase);
}

static u16  selBankGet(u32 ix)            { return selectedBank; }
static void selBankSet(u32 ix, u16 value) { selectedBank = value; }

static u16  selPatchGet(u32 ix)            { return selectedPatch; }
static void selPatchSet(u32 ix, u16 value) { selectedPatch = value; }


/////////////////////////////////////////////////////////////////////////////
// Menu Structure
/////////////////////////////////////////////////////////////////////////////

const scs_menu_item_t pageGlb[] = {
  SCS_ITEM("DwS ", SYNTH_GLOBAL_PAR_DOWNSAMPLING_FACTOR,   63, globalGet, globalSet, selectNOP, stringDec, NULL),
  SCS_ITEM("Res ", SYNTH_GLOBAL_PAR_RESOLUTION,            16, globalGet, globalSet, selectNOP, stringDec, NULL),
  SCS_ITEM("XOR ", SYNTH_GLOBAL_PAR_XOR,                 0x7f, globalGet, globalSet, selectNOP, stringDec, NULL),
};

const scs_menu_item_t pagePhr[] = {
  SCS_ITEM("PLY ", 0, 1,                          playGet,          playSet,          selectPLAY,stringPlay,  NULL),
  SCS_ITEM("Phr ", 0, SYNTH_NUM_PHRASES-1,       phraseGet,       phraseSet,       selectNOP, stringDecP1,  NULL),
  SCS_ITEM("Len ", SYNTH_PHRASE_PAR_LENGTH, SYNTH_PHRASE_MAX_LENGTH-1, phraseParGet, phraseParSet, selectNOP, stringDecP1,  NULL),
  SCS_ITEM("Tun ", SYNTH_PHRASE_PAR_TUNE,     0x7f,  phraseParGet,     phraseParSet,     selectNOP, stringDecPM,  NULL),
  SCS_ITEM("Ix  ", 0, SYNTH_PHRASE_MAX_LENGTH-1,     phonemeIxGet,     phonemeIxSet,     selectNOP, stringDecP1,  NULL),
  SCS_ITEM("Ph  ", SYNTH_PHONEME_PAR_PH,      0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Env ", SYNTH_PHONEME_PAR_ENV,     0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Ton ", SYNTH_PHONEME_PAR_TONE,    0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Typ ", SYNTH_PHONEME_PAR_TYPE,    0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("PrP ", SYNTH_PHONEME_PAR_PREPAUSE, 0x7f, phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Amp ", SYNTH_PHONEME_PAR_AMP,     0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("New ", SYNTH_PHONEME_PAR_NEWWORD, 0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Flg ", SYNTH_PHONEME_PAR_FLAGS,   0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Len ", SYNTH_PHONEME_PAR_LENGTH,  0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Pt1 ", SYNTH_PHONEME_PAR_PITCH1,  0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Pt2 ", SYNTH_PHONEME_PAR_PITCH2,  0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("SIx ", SYNTH_PHONEME_PAR_SOURCE_IX, SYNTH_PHRASE_MAX_LENGTH-1, phonemeParGet, phonemeParSet, selectNOP, stringDec, NULL),
};

const scs_menu_item_t pageDsk[] = {
  SCS_ITEM("Bnk ", 0, 3,  selBankGet,  selBankSet,  selectNOP, stringDecP1, NULL),
  SCS_ITEM("Pat ", 0, 63, selPatchGet, selPatchSet, selectNOP, stringDecP1, NULL),
  SCS_ITEM("Save", 0, 0, dummyGet, dummySet, selectSAVE, stringEmpty, NULL),
  SCS_ITEM("Load", 0, 0, dummyGet, dummySet, selectLOAD, stringEmpty, NULL),
};

const scs_menu_page_t rootMode0[] = {
  SCS_PAGE("Dsk ", pageDsk),
  SCS_PAGE("Glb ", pageGlb),
  SCS_PAGE("Phr ", pagePhr),
};


/////////////////////////////////////////////////////////////////////////////
// This function returns the two lines of the main page (2x20 chars)
/////////////////////////////////////////////////////////////////////////////
static s32 getStringMainPage(char *line1, char *line2)
{
  strcpy(line1, "Main Page");
  strcpy(line2, "Press soft button");

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
  // here the root table could be changed depending on soft button

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the upper line on the page selection (20 chars)
/////////////////////////////////////////////////////////////////////////////
static s32 getStringPageSelection(char *line1)
{
  strcpy(line1, "Select Page:");

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
