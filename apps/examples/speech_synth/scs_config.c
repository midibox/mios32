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

static u8 extraPage;

static u8 selectedPatch;
static u8 selectedBank;
static u8 selectedPhrase;
static u8 selectedPhonemeIx;


/////////////////////////////////////////////////////////////////////////////
// Generates patch string
/////////////////////////////////////////////////////////////////////////////
static void getPatchString(char *str)
{
  char *patchName = SYNTH_PatchNameGet(0);
  int i;
  u8 stringIsEmpty = 1;
  for(i=0; i<SYNTH_PATCH_NAME_LEN; ++i)
    if( patchName[i] != ' ' ) {
      stringIsEmpty = 0;
      break;
    }

  sprintf(str, "%c%03d %s", 'A' + selectedBank, selectedPatch+1, stringIsEmpty ? "<Unnamed Patch>" : patchName);
}

/////////////////////////////////////////////////////////////////////////////
// String Conversion Functions
/////////////////////////////////////////////////////////////////////////////
static void stringEmpty(u32 ix, u16 value, char *label)  { label[0] = 0; }
static void stringDec(u32 ix, u16 value, char *label)    { sprintf(label, "%3d  ", value); }
static void stringDecP1(u32 ix, u16 value, char *label)  { sprintf(label, "%3d  ", value+1); }
static void stringDecPM(u32 ix, u16 value, char *label)  { sprintf(label, "%3d  ", (int)value - 64); }
static void stringDec03(u32 ix, u16 value, char *label)  { sprintf(label, "%03d  ", value); }
static void stringHex2(u32 ix, u16 value, char *label)    { sprintf(label, " %02x  ", value); }
static void stringPlay(u32 ix, u16 value, char *label)   { sprintf(label, " [%c] ", SYNTH_PhraseIsPlayed(selectedPhrase) ? '*' : 'o'); }

/////////////////////////////////////////////////////////////////////////////
// Parameter Selection Functions
/////////////////////////////////////////////////////////////////////////////
static u16  selectNOP (u32 ix, u16 value)    { return value; }
static u16  selectPLAY(u32 ix, u16 value)    { return value ? 0 : 1; } // toggle

static void selectSAVE_Callback(char *newString)
{
  s32 status;
  u8 sourceGroup = 0;
  u8 rename_if_empty_name = 1;

  SYNTH_PatchNameSet(sourceGroup, newString);

  MUTEX_SDCARD_TAKE;
  if( (status=SYNTH_FILE_B_PatchWrite(selectedBank, selectedPatch, sourceGroup, rename_if_empty_name)) < 0 ) {
    char buffer[100];
    sprintf(buffer, "Patch %c%03d", 'A'+selectedBank, selectedPatch+1);
    SCS_Msg(SCS_MSG_ERROR_L, 1000, "Failed to store", buffer);
  } else {
    char buffer[100];
    sprintf(buffer, "Patch %c%03d", 'A'+selectedBank, selectedPatch+1);
    SCS_Msg(SCS_MSG_L, 1000, buffer, "stored!");
  }
  MUTEX_SDCARD_GIVE;
}

static u16  selectSAVE(u32 ix, u16 value)
{
  return SCS_InstallEditStringCallback(selectSAVE_Callback, "SAVE", SYNTH_PatchNameGet(0), SYNTH_PATCH_NAME_LEN);
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
  SCS_ITEM("DwnS ", SYNTH_GLOBAL_PAR_DOWNSAMPLING_FACTOR,   63, globalGet, globalSet, selectNOP, stringDec, NULL),
  SCS_ITEM("Res. ", SYNTH_GLOBAL_PAR_RESOLUTION,            16, globalGet, globalSet, selectNOP, stringDec, NULL),
  SCS_ITEM("XOR  ", SYNTH_GLOBAL_PAR_XOR,                 0x7f, globalGet, globalSet, selectNOP, stringDec, NULL),
};

const scs_menu_item_t pagePhr[] = {
  SCS_ITEM("PLAY ", 0, 1,                          playGet,          playSet,          selectPLAY,stringPlay,  NULL),
  SCS_ITEM("Phrs ", 0, SYNTH_NUM_PHRASES-1,       phraseGet,       phraseSet,       selectNOP, stringDecP1,  NULL),
  SCS_ITEM("Len. ", SYNTH_PHRASE_PAR_LENGTH, SYNTH_PHRASE_MAX_LENGTH-1, phraseParGet, phraseParSet, selectNOP, stringDecP1,  NULL),
  SCS_ITEM("Tune ", SYNTH_PHRASE_PAR_TUNE,     0x7f,  phraseParGet,     phraseParSet,     selectNOP, stringDecPM,  NULL),
  SCS_ITEM(" Ix  ", 0, SYNTH_PHRASE_MAX_LENGTH-1,     phonemeIxGet,     phonemeIxSet,     selectNOP, stringDecP1,  NULL),
  SCS_ITEM("Phr  ", SYNTH_PHONEME_PAR_PH,      0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Env  ", SYNTH_PHONEME_PAR_ENV,     0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Tone ", SYNTH_PHONEME_PAR_TONE,    0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Type ", SYNTH_PHONEME_PAR_TYPE,    0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("PrP  ", SYNTH_PHONEME_PAR_PREPAUSE, 0x7f, phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Amp. ", SYNTH_PHONEME_PAR_AMP,     0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("New  ", SYNTH_PHONEME_PAR_NEWWORD, 0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Flag ", SYNTH_PHONEME_PAR_FLAGS,   0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Len. ", SYNTH_PHONEME_PAR_LENGTH,  0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Pt1  ", SYNTH_PHONEME_PAR_PITCH1,  0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("Pt2  ", SYNTH_PHONEME_PAR_PITCH2,  0x7f,  phonemeParGet,    phonemeParSet,    selectNOP, stringDec,    NULL),
  SCS_ITEM("SIx  ", SYNTH_PHONEME_PAR_SOURCE_IX, SYNTH_PHRASE_MAX_LENGTH-1, phonemeParGet, phonemeParSet, selectNOP, stringDec, NULL),
};

const scs_menu_item_t pageDsk[] = {
  SCS_ITEM("Bank ", 0, 3,  selBankGet,  selBankSet,  selectNOP, stringDecP1, NULL),
  SCS_ITEM("Pat. ", 0, 63, selPatchGet, selPatchSet, selectNOP, stringDecP1, NULL),
  SCS_ITEM("Save ", 0, 0, dummyGet, dummySet, selectSAVE, stringEmpty, NULL),
};

const scs_menu_page_t rootMode0[] = {
  SCS_PAGE("Disk ", pageDsk),
  SCS_PAGE("Glb. ", pageGlb),
  SCS_PAGE("Phr. ", pagePhr),
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
    getPatchString(line1);
    sprintf(line2, "Press soft button");
    return 1;
  } else if( SCS_MenuStateGet() == SCS_MENU_STATE_SELECT_PAGE ) {
    getPatchString(line1);
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
    int newPatch = selectedPatch + incrementer;
    if( newPatch < 0 )
      newPatch = 0;
    else if( newPatch > 63 )
      newPatch = 63;

    selectedPatch = newPatch;

    s32 status;
    u8 targetGroup = 0;
    MUTEX_SDCARD_TAKE;
    if( (status=SYNTH_FILE_B_PatchRead(selectedBank, selectedPatch, targetGroup)) < 0 ) {
      char buffer[100];
      sprintf(buffer, "Patch %c%03d", 'A'+selectedBank, selectedPatch+1);
      SCS_Msg(SCS_MSG_ERROR_L, 1000, "Failed to read", buffer);
    } else {
      //    char buffer[100];
      //    sprintf(buffer, "Patch %c%03d", 'A'+selectedBank, selectedPatch+1);
      //    SCS_Msg(SCS_MSG_L, 1000, buffer, "read!");
    }
    MUTEX_SDCARD_GIVE;
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
	case SCS_PIN_SOFT2: SCS_Msg(SCS_MSG_L, 1000, "Soft Button", "#2 pressed"); break;
	case SCS_PIN_SOFT3: SCS_Msg(SCS_MSG_L, 1000, "Soft Button", "#3 pressed"); break;
	case SCS_PIN_SOFT4: SCS_Msg(SCS_MSG_L, 1000, "Soft Button", "#4 pressed"); break;
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
