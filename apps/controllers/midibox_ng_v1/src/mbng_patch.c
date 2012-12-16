// $Id$
/*
 * Patch Layer for MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
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

#include "mbng_patch.h"
#include "mbng_dout.h"
#include "mbng_file.h"
#include "mbng_file_c.h"
#include "mbng_file_l.h"


/////////////////////////////////////////////////////////////////////////////
// Preset patch
/////////////////////////////////////////////////////////////////////////////

mbng_patch_matrix_din_entry_t mbng_patch_matrix_din[MBNG_PATCH_NUM_MATRIX_DIN];
mbng_patch_matrix_dout_entry_t mbng_patch_matrix_dout[MBNG_PATCH_NUM_MATRIX_DOUT];

mbng_patch_ain_entry_t mbng_patch_ain;

mbng_patch_ainser_entry_t mbng_patch_ainser[MBNG_PATCH_NUM_AINSER_MODULES];

mbng_patch_mf_entry_t mbng_patch_mf[MBNG_PATCH_NUM_MF_MODULES];

mbng_patch_bank_entry_t mbng_patch_bank[MBNG_PATCH_NUM_BANKS];

mbng_patch_cfg_t mbng_patch_cfg = {
  .debounce_ctr = 20,
  .global_chn = 0,
  .all_notes_off_chn = 0,
  .convert_note_off_to_on0 = 1,
  .sysex_dev = 0,
  .sysex_pat = 0,
  .sysex_bnk = 0,
  .sysex_ins = 0,
  .sysex_chn = 0,
};

/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////
static u8 selected_bank;


/////////////////////////////////////////////////////////////////////////////
// This function initializes the patch structure
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_PATCH_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  selected_bank = 0;

  {
    int matrix;
    mbng_patch_matrix_din_entry_t *m = (mbng_patch_matrix_din_entry_t *)&mbng_patch_matrix_din[0];
    for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DIN; ++matrix, ++m) {
      m->num_rows = 8;
      m->inverted = 0;
      m->button_emu_id_offset = 0;
      m->sr_dout_sel1 = 0;
      m->sr_dout_sel2 = 0;
      m->sr_din1  = 0;
      m->sr_din2  = 0;
    }
  }

  {
    int matrix;
    mbng_patch_matrix_dout_entry_t *m = (mbng_patch_matrix_dout_entry_t *)&mbng_patch_matrix_dout[0];
    for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DOUT; ++matrix, ++m) {
      m->num_rows = 8;
      m->inverted = 0;
      m->led_emu_id_offset = 0;
      m->sr_dout_sel1  = 0;
      m->sr_dout_sel2 = 0;
      m->sr_dout_r1 = 0;
      m->sr_dout_r2 = 0;
      m->sr_dout_g1 = 0;
      m->sr_dout_g2 = 0;
      m->sr_dout_r1 = 0;
      m->sr_dout_r2 = 0;
    }
  }

  mbng_patch_ain.enable_mask = 0x0000;

  {
    int module;
    mbng_patch_ainser_entry_t *ainser = (mbng_patch_ainser_entry_t *)&mbng_patch_ainser[0];
    for(module=0; module<MBNG_PATCH_NUM_AINSER_MODULES; ++module, ++ainser) {
      ainser->flags.enabled = 1;
      ainser->flags.cs = module;
    }
  }

  {
    int bank;
    mbng_patch_bank_entry_t *b = (mbng_patch_bank_entry_t *)&mbng_patch_bank[0];
    for(bank=0; bank<MBNG_PATCH_NUM_BANKS; ++bank, ++b) {
      u8 valid = bank == 0;
      MBNG_PATCH_BankEntryInit(b, valid);
    }
  }
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function loads the patch from SD Card
// Returns != 0 if Load failed
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_PATCH_Load(char *filename)
{
  s32 status = 0;

  MUTEX_SDCARD_TAKE;
  status |= MBNG_FILE_C_Read(filename);
#if 0
  status |= MBNG_FILE_L_Read(filename);
#else
  // we need a more sophisticated file handling to determine if file *must* exist before error message is print out
  MBNG_FILE_L_Read(filename);
#endif
  MUTEX_SDCARD_GIVE;

  // select first bank
  MBNG_PATCH_BankSet(0);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// This function stores the patch on SD Card
// Returns != 0 if Store failed
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_PATCH_Store(char *filename)
{
  MUTEX_SDCARD_TAKE;
  s32 status = MBNG_FILE_C_Write(filename);
  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Bank Handling
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_PATCH_BankEntryInit(mbng_patch_bank_entry_t *b, u8 valid)
{
  if( valid ) {
    b->valid = 1;

    b->button.first_n = 1;
    b->button.num = MBNG_PATCH_NUM_DIN;
    b->button.first_id = MBNG_EVENT_CONTROLLER_BUTTON + 1;

    b->led.first_n = 1;
    b->led.num = MBNG_PATCH_NUM_DOUT;
    b->led.first_id = MBNG_EVENT_CONTROLLER_LED + 1;

    b->enc.first_n = 1;
    b->enc.num = MBNG_PATCH_NUM_ENC;
    b->enc.first_id = MBNG_EVENT_CONTROLLER_ENC + 1;

    b->ain.first_n = 1;
    b->ain.num = MBNG_PATCH_NUM_AIN;
    b->ain.first_id = MBNG_EVENT_CONTROLLER_AIN + 1;

    b->ainser.first_n = 1;
    b->ainser.num = MBNG_PATCH_NUM_AINSER_MODULES*64;
    b->ainser.first_id = MBNG_EVENT_CONTROLLER_AINSER + 1;

    b->mf.first_n = 1;
    b->mf.num = MBNG_PATCH_NUM_MF_MODULES*8;
    b->mf.first_id = MBNG_EVENT_CONTROLLER_MF + 1;
  } else {
    b->valid = 0;

    b->button.first_n = 0;
    b->button.num = 0;
    b->button.first_id = 0;

    b->led.first_n = 0;
    b->led.num = 0;
    b->led.first_id = 0;

    b->enc.first_n = 0;
    b->enc.num = 0;
    b->enc.first_id = 0;

    b->ain.first_n = 0;
    b->ain.num = 0;
    b->ain.first_id = 0;

    b->ainser.first_n = 0;
    b->ainser.num = 0;
    b->ainser.first_id = 0;

    b->mf.first_n = 0;
    b->mf.num = 0;
    b->mf.first_id = 0;
  }

  return 0;
}

s32 MBNG_PATCH_BankSet(u8 new_bank)
{
  if( new_bank < MBNG_PATCH_NUM_BANKS ) {
    selected_bank = new_bank;
  }

  // refresh all controllers
  MBNG_EVENT_Refresh();

  return 0;
}

s32 MBNG_PATCH_BankGet(void)
{
  return selected_bank;
}

s32 MBNG_PATCH_NumBanksGet(void)
{
  int bank;

  mbng_patch_bank_entry_t *b = (mbng_patch_bank_entry_t *)&mbng_patch_bank[0];
  for(bank=0; bank<MBNG_PATCH_NUM_BANKS && b->valid; ++bank, ++b);

  return bank;
}

/////////////////////////////////////////////////////////////////////////////
// IN: element ix and pointer to id
// OUT: modified id if bank entry is valid
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_PATCH_BankCtrlIdGet(u32 ix, mbng_event_item_id_t* id)
{
  mbng_patch_bank_entry_t *b = (mbng_patch_bank_entry_t *)&mbng_patch_bank[selected_bank];


  mbng_patch_bank_ctrl_t *bc = NULL;
  switch( *id & 0xf000 ) {
  case MBNG_EVENT_CONTROLLER_BUTTON: bc = &b->button; break;
  case MBNG_EVENT_CONTROLLER_LED: bc = &b->led; break;
  case MBNG_EVENT_CONTROLLER_ENC: bc = &b->enc; break;
  case MBNG_EVENT_CONTROLLER_AIN: bc = &b->ain; break;
  case MBNG_EVENT_CONTROLLER_AINSER: bc = &b->ainser; break;
  case MBNG_EVENT_CONTROLLER_MF: bc = &b->mf; break;
  }

  if( bc ) {
    if( b->valid && bc->num &&
	ix >= (bc->first_n-1) && ix < (bc->first_n-1 + bc->num) ) {
      u32 offset = (ix - (bc->first_n-1)) % bc->num;
      *id = bc->first_id + offset;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// IN: item
// OUT: 1 if item is in any bank, otherwise 0
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_PATCH_BankCtrlInBank(mbng_event_item_t *item)
{
  // select first ctrl entry:
  mbng_patch_bank_entry_t *b = (mbng_patch_bank_entry_t *)&mbng_patch_bank[0];
  mbng_patch_bank_ctrl_t *bc = NULL;
  switch( item->id & 0xf000 ) {
  case MBNG_EVENT_CONTROLLER_BUTTON: bc = &b->button; break;
  case MBNG_EVENT_CONTROLLER_LED: bc = &b->led; break;
  case MBNG_EVENT_CONTROLLER_ENC: bc = &b->enc; break;
  case MBNG_EVENT_CONTROLLER_AIN: bc = &b->ain; break;
  case MBNG_EVENT_CONTROLLER_AINSER: bc = &b->ainser; break;
  case MBNG_EVENT_CONTROLLER_MF: bc = &b->mf; break;
  }

  if( bc == NULL )
    return 1; // not covered by banks -> always active

  mbng_event_item_id_t item_id = item->id;
  int bank;
  for(bank=0; bank<MBNG_PATCH_NUM_BANKS && b->valid; ++bank, ++b, bc = (mbng_patch_bank_ctrl_t *)((u8 *)bc + sizeof(mbng_patch_bank_entry_t))) {
    if( bc->num && item_id >= bc->first_id && item_id < (bc->first_id + bc->num) ) {
      return 1; // found
    }
  }

  return 0; // not in bank
}


/////////////////////////////////////////////////////////////////////////////
// IN: item
// OUT: 1 if item is selected in the bank, otherwise 0
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_PATCH_BankCtrlIsActive(mbng_event_item_t *item)
{
  // select first ctrl entry:
  mbng_patch_bank_entry_t *b = (mbng_patch_bank_entry_t *)&mbng_patch_bank[selected_bank];
  mbng_patch_bank_ctrl_t *bc = NULL;
  switch( item->id & 0xf000 ) {
  case MBNG_EVENT_CONTROLLER_BUTTON: bc = &b->button; break;
  case MBNG_EVENT_CONTROLLER_LED: bc = &b->led; break;
  case MBNG_EVENT_CONTROLLER_ENC: bc = &b->enc; break;
  case MBNG_EVENT_CONTROLLER_AIN: bc = &b->ain; break;
  case MBNG_EVENT_CONTROLLER_AINSER: bc = &b->ainser; break;
  case MBNG_EVENT_CONTROLLER_MF: bc = &b->mf; break;
  }

  return bc &&
    bc->num &&
    item->id >= bc->first_id &&
    item->id < (bc->first_id + bc->num);
}
