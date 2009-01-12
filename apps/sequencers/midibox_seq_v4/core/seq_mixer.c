// $Id$
/*
 * Mixer Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
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

#include "seq_mixer.h"
#include "seq_file_m.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// should only be directly accessed by SEQ_FILE_M, remaining functions should
// use SEQ_MIXER_Get/Set
u8 seq_mixer_value[SEQ_MIXER_NUM_CHANNELS][SEQ_MIXER_NUM_PARAMETERS];
char seq_mixer_map_name[21];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIXER_Init(u32 mode)
{
  // clear mixer page
  SEQ_MIXER_Clear();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the name of the mixer map (20 characters)
/////////////////////////////////////////////////////////////////////////////
char *SEQ_MIXER_MapNameGet(void)
{
  return seq_mixer_map_name;
}


/////////////////////////////////////////////////////////////////////////////
// Sets a mixer value
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIXER_Set(u8 chn, seq_mixer_par_t par, u8 value)
{
  if( chn >= SEQ_MIXER_NUM_CHANNELS )
    return -1; // invalid channel number
  if( chn >= SEQ_MIXER_NUM_PARAMETERS )
    return -2; // invalid parameter number

  seq_mixer_value[chn][par] = value;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns a mixer value
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIXER_Get(u8 chn, seq_mixer_par_t par)
{
  if( chn >= SEQ_MIXER_NUM_CHANNELS )
    return -1; // invalid channel number
  if( chn >= SEQ_MIXER_NUM_PARAMETERS )
    return -2; // invalid parameter number

  return seq_mixer_value[chn][par];
}


/////////////////////////////////////////////////////////////////////////////
// Sends a single mixer value
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIXER_Send(u8 chn, seq_mixer_par_t par)
{
  mios32_midi_port_t midi_port = SEQ_MIXER_Get(chn, SEQ_MIXER_PAR_PORT);
  mios32_midi_chn_t  midi_chn = SEQ_MIXER_Get(chn, SEQ_MIXER_PAR_CHANNEL);

  s32 value;
  if( (value=SEQ_MIXER_Get(chn, par)) < 0 )
    return 0; // don't return error, as it could be misinterpreded as a MIDI interface issue

  switch( par ) {
    case SEQ_MIXER_PAR_PRG:
      return value == 0 ? 0 : MIOS32_MIDI_SendProgramChange(midi_port, midi_chn, value-1);
    case SEQ_MIXER_PAR_VOLUME:   
      return value == 0 ? 0 : MIOS32_MIDI_SendCC(midi_port, midi_chn, 7, value-1);
    case SEQ_MIXER_PAR_PANORAMA: 
      return value == 0 ? 0 : MIOS32_MIDI_SendCC(midi_port, midi_chn, 10, value-1);
    case SEQ_MIXER_PAR_REVERB:   
      return value == 0 ? 0 : MIOS32_MIDI_SendCC(midi_port, midi_chn, 91, value-1);
    case SEQ_MIXER_PAR_CHORUS:
      return value == 0 ? 0 : MIOS32_MIDI_SendCC(midi_port, midi_chn, 93, value-1);
    case SEQ_MIXER_PAR_MODWHEEL:
      return value == 0 ? 0 : MIOS32_MIDI_SendCC(midi_port, midi_chn, 1, value-1);

    case SEQ_MIXER_PAR_CC1:
      return value == 0 ? 0 : MIOS32_MIDI_SendCC(midi_port, midi_chn, SEQ_MIXER_Get(chn, SEQ_MIXER_PAR_CC1_NUM), value-1);
    case SEQ_MIXER_PAR_CC2:
      return value == 0 ? 0 : MIOS32_MIDI_SendCC(midi_port, midi_chn, SEQ_MIXER_Get(chn, SEQ_MIXER_PAR_CC2_NUM), value-1);
    case SEQ_MIXER_PAR_CC3:
      return value == 0 ? 0 : MIOS32_MIDI_SendCC(midi_port, midi_chn, SEQ_MIXER_Get(chn, SEQ_MIXER_PAR_CC3_NUM), value-1);
    case SEQ_MIXER_PAR_CC4:
      return value == 0 ? 0 : MIOS32_MIDI_SendCC(midi_port, midi_chn, SEQ_MIXER_Get(chn, SEQ_MIXER_PAR_CC4_NUM), value-1);
  }

  // not supported
  // don't return error, as this could be misinterpreted as a MIDI interface issue
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Sends all mixer values
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIXER_SendAll(void)
{
  u8 chn;
  seq_mixer_par_t par;
  s32 status = 0;

  for(chn=0; chn<SEQ_MIXER_NUM_CHANNELS; ++chn)
    for(par=SEQ_MIXER_PAR_PRG; par<=SEQ_MIXER_PAR_CC4; ++par)
      status |= SEQ_MIXER_Send(chn, par);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Clears a mixer page
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIXER_Clear(void)
{
  // init name
  int i;
  for(i=0; i<20; ++i)
    seq_mixer_map_name[i] = ' ';
  seq_mixer_map_name[i] = 0;

  u8 chn, par;
  // init mixer values
  for(chn=0; chn<SEQ_MIXER_NUM_CHANNELS; ++chn) {
    for(par=0; par<SEQ_MIXER_NUM_PARAMETERS; ++par)
      seq_mixer_value[chn][par] = 0;

    seq_mixer_value[chn][SEQ_MIXER_PAR_CHANNEL] = chn & 0xf;
    seq_mixer_value[chn][SEQ_MIXER_PAR_CC1_NUM] = 16;
    seq_mixer_value[chn][SEQ_MIXER_PAR_CC2_NUM] = 17;
    seq_mixer_value[chn][SEQ_MIXER_PAR_CC3_NUM] = 18;
    seq_mixer_value[chn][SEQ_MIXER_PAR_CC4_NUM] = 19;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads a mixer map
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIXER_Load(u8 map)
{
  s32 status;

  MUTEX_SDCARD_TAKE;
  if( (status=SEQ_FILE_M_MapRead(map)) < 0 )
    SEQ_UI_SDCardErrMsg(2000, status);
  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Stores a mixer map
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIXER_Save(u8 map)
{
  s32 status;

  MUTEX_SDCARD_TAKE;
  if( (status=SEQ_FILE_M_MapWrite(map)) < 0 )
    SEQ_UI_SDCardErrMsg(2000, status);
  MUTEX_SDCARD_GIVE;

  return status;
}
