// $Id$
//! \defgroup MBNG_PATCH
//! Patch Layer for MIDIbox NG
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
//! Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "tasks.h"
#include <scs.h>
#include <scs_lcd.h>
#include <keyboard.h>
#include <ainser.h>
#include <aout.h>
#include <midi_router.h>

#include "mbng_patch.h"
#include "mbng_dout.h"
#include "mbng_file.h"
#include "mbng_file_c.h"
#include "mbng_file_l.h"
#include "mbng_file_s.h"
#include "mbng_file_r.h"


/////////////////////////////////////////////////////////////////////////////
//! Preset patch
/////////////////////////////////////////////////////////////////////////////

mbng_patch_matrix_din_entry_t mbng_patch_matrix_din[MBNG_PATCH_NUM_MATRIX_DIN];
mbng_patch_matrix_dout_entry_t mbng_patch_matrix_dout[MBNG_PATCH_NUM_MATRIX_DOUT];

mbng_patch_ain_entry_t mbng_patch_ain;

mbng_patch_ainser_entry_t mbng_patch_ainser[MBNG_PATCH_NUM_AINSER_MODULES];

mbng_patch_mf_entry_t mbng_patch_mf[MBNG_PATCH_NUM_MF_MODULES];

char mbng_patch_aout_spi_rc_pin;
char mbng_patch_max72xx_spi_rc_pin;

mbng_patch_scs_t mbng_patch_scs;
static const mbng_patch_scs_t mbng_patch_scs_default = {
  .button_emu_id = {0, 0, 0, 0, 0},
  .enc_emu_id = 0,
};

mbng_patch_cfg_t mbng_patch_cfg;
static const mbng_patch_cfg_t mbng_patch_cfg_default = {
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

#if MBNG_PATCH_NUM_DIO > 0
mbng_patch_dio_cfg_t mbng_patch_dio_cfg[MBNG_PATCH_NUM_DIO];
#endif


/////////////////////////////////////////////////////////////////////////////
//! This function initializes the patch structure
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_PATCH_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  mbng_patch_cfg = mbng_patch_cfg_default;
  mbng_patch_scs = mbng_patch_scs_default;

  SCS_LCD_DeviceSet(0);
  SCS_LCD_OffsetXSet(0);
  SCS_LCD_OffsetYSet(0);
  SCS_NumMenuItemsSet(4); // for 2x20 LCD

  // MIDI router: disable all MIDI IN clocks by default to avoid feedback loops
  midi_router_mclk_in = 0;

  {
    int matrix;
    mbng_patch_matrix_din_entry_t *m = (mbng_patch_matrix_din_entry_t *)&mbng_patch_matrix_din[0];
    for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DIN; ++matrix, ++m) {
      m->num_rows = 8;
      m->flags.ALL = 0;
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
      m->flags.ALL = 0;
      m->led_emu_id_offset = 0;
      m->sr_dout_sel1  = 0;
      m->sr_dout_sel2 = 0;
      m->sr_dout_r1 = 0;
      m->sr_dout_r2 = 0;
      m->sr_dout_g1 = 0;
      m->sr_dout_g2 = 0;
      m->sr_dout_r1 = 0;
      m->sr_dout_r2 = 0;
      m->lc_meter_port = 0;
    }
  }

  {
    mbng_patch_ain.enable_mask = 0x0000;

    MIOS32_AIN_DeadbandSet(31); // matches with 7bit

    int pin;
    for(pin=0; pin<MBNG_PATCH_NUM_AIN; ++pin) {
      mbng_patch_ain.cali[pin].min = 0;
      mbng_patch_ain.cali[pin].max = MBNG_PATCH_AIN_MAX_VALUE;
      mbng_patch_ain.cali[pin].spread_center = 0;
    }
  }

  {
    int module;
    mbng_patch_ainser_entry_t *ainser = (mbng_patch_ainser_entry_t *)&mbng_patch_ainser[0];
    for(module=0; module<MBNG_PATCH_NUM_AINSER_MODULES; ++module, ++ainser) {
      ainser->flags.cs = module;
      ainser->flags.resolution = 7; // bit

      AINSER_EnabledSet(module, 0);
      AINSER_MuxedSet(module, 1);
      AINSER_DeadbandSet(module, 31); // matches with 7bit
      AINSER_NumPinsSet(module, AINSER_NUM_PINS);

      int pin;
      for(pin=0; pin<AINSER_NUM_PINS; ++pin) {
	ainser->cali[pin].min = 0;
	ainser->cali[pin].max = MBNG_PATCH_AINSER_MAX_VALUE;
	ainser->cali[pin].spread_center = 0;
      }
    }

    // MAX72xx CS pin
    mbng_patch_max72xx_spi_rc_pin = 0;
  }

  {
    // AOUT CS pin
    mbng_patch_aout_spi_rc_pin = 0;

    // disable module by default, only enable 8 channels by default
    aout_config_t config;
    config = AOUT_ConfigGet();
    config.if_type = AOUT_IF_NONE;
    config.num_channels = 8;
    AOUT_ConfigSet(config);
    AOUT_IF_Init(0);
  }

  {
    KEYBOARD_Init(0);

    // disable keyboard SR assignments by default
    int kb;
    keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[0];
    for(kb=0; kb<KEYBOARD_NUM; ++kb, ++kc) {
      kc->num_rows = 0;
      kc->dout_sr1 = 0;
      kc->dout_sr2 = 0;
      kc->din_sr1 = 0;
      kc->din_sr2 = 0;

      // due to slower scan rate:
      kc->delay_fastest = 5;
      kc->delay_fastest_black_keys = 0; // if 0, we take delay_fastest, otherwise we take the dedicated value for the black keys
      kc->delay_slowest = 100;
    }

    KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
  }

#if MBNG_PATCH_NUM_DIO > 0
  {
    int dio;

    for(dio=0; dio<MBNG_PATCH_NUM_DIO; ++dio) {
      mbng_patch_dio_cfg[dio].mode = MBNG_PATCH_DIO_CFG_MODE_Off;
      mbng_patch_dio_cfg[dio].emu_sr = 0;
    }
  }
#endif

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function loads the patch from SD Card
//! \return != 0 if Load failed
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_PATCH_Load(char *filename)
{
  s32 status = 0;

  // ensure that .NCR script isn't executed anymore
  MBNG_FILE_R_RunStop();

  MUTEX_SDCARD_TAKE;
  status |= MBNG_FILE_C_Read(filename);
#if 0
  status |= MBNG_FILE_L_Read(filename);
#else
  // we need a more sophisticated file handling to determine if file *must* exist before error message is print out
  MBNG_FILE_L_Read(filename);
#endif
  MBNG_FILE_S_Read(filename, -1);
  MBNG_FILE_R_ReadRequest(filename, 0, 0, 0);
  MUTEX_SDCARD_GIVE;

  // refresh the elements (will update the screen!)
  MBNG_EVENT_Refresh();

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! This function stores the patch on SD Card
//! \return != 0 if Store failed
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_PATCH_Store(char *filename)
{
  MUTEX_SDCARD_TAKE;
  s32 status = MBNG_FILE_C_Write(filename);
  MUTEX_SDCARD_GIVE;

  return status;
}


//! \}
