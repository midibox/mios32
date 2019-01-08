// $Id: mbfm_sequencer.c $
/*
 * Header file for MBHP_MBFM MIDIbox FM V2.0 synth engine control handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2014 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "mbfm_sequencer.h"

#include <opl3.h>

#include "mbfm.h"
#include "mbfm_controlhandler.h"
#include "mbfm_modulation.h"
#include "mbng_event.h"
#include "mbng_lcd.h"

mbfm_step_t sequence[OPL3_COUNT*17*4];
u8 submeas_enabled[4];
u32 seq_tempo; //In 256ths of a ms per tick, with TICKS_PER_STEP
u8 seq_swing;
u32 seq_starttime;
u16 seq_masterpos;
u8 seq_tickinstep;
u8 seq_mastermeas;
u8 seq_measstep;
u8 seq_submeas;
u8 seq_lastsubmeas;
u8 seq_measurelen;
u8 seq_beatlen;
u8 seq_recording;
u8 seq_playing;
mbfm_seqshow_t seq_show;
u8 seq_selsubmeas;
u8 seq_seldrum;
mbfm_drumstate_t drumstates[6*OPL3_COUNT];
u8 drums_soloed;

static const u8 tps_gate_lengths[4] = {
  4, 8, 15, 16
};


void MBFM_Drum_InitPercussionMode(u8 chip){
  u8 voice = (18*chip) + 15, i;
  //Initialize voices for drums
  for(; voice<18*(chip+1); voice++){
    voicedupl[voice].voice = -1;
    voicelink[voice].voice = -1;
    for(i=0; i<18*OPL3_COUNT; i++){
      if(voicedupl[i].voice == voice || voicelink[i].voice == voice){
        voicemidistate[voice].note = 0;
      }
    }
    //Put in default percussion patch
    MBFM_InitPercValues(voice);
    //Set up lack of note handler
    voicemidistate[voice].note = 0;
    voicemidistate[voice].update = 0;
    voiceactualnotes[voice].note = 60; //Middle C, transpose/tune from there
    voiceactualnotes[voice].update = 1; //Of course
    //"Map" voices to ch 10 for volume, pitch bend, etc.
    midichanmapping[voice] = 9;
  }
}

void MBFM_Drum_ReceiveGenericTrigger(u8 drum, u8 state, u8 canrecord){
  if(drum >= 6*OPL3_COUNT) return;
  //MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED, drum, state);
  if(drumstates[drum].state == state) return; //No change
  if(canrecord && seq_recording){
    MBFM_Drum_RecordNow(drum, state);
  }
  drumactivity[drum] = state ? 1 : 0;
  u8 chip = drum / 6;
  u8 chipoffset = 18*chip;
  u32 time = MIOS32_TIMESTAMP_Get();
  u8 subdrum = drum % 6;
  if(subdrum & 2){ //Only HH open/closed
    u8 otherdrum = (!(subdrum-2))+2 + (drum-subdrum); //Global index of other hat
    if(state && (//Turning on and...
          drumstates[otherdrum].state //Other on already
          || drumstates[otherdrum].update)){ //Off but set to refresh (i.e. just turned off)
      //Stop hi-hat now
      OPL3_TriggerHH(chip, 0);
      //Set new states
      drumstates[drum].state = 1;
      drumstates[otherdrum].state = 0;
      //Set hi-hat to retrigger next frame
      drumstates[drum].update = 2;
      drumstates[otherdrum].update = 0;
      //Turn off light for other one
      drumactivity[otherdrum] = 0;
      //Set vari for new hh mode
      modl_varis[16+chipoffset] = (subdrum == 3) ? 127 : 0;
      return;
    }
  }
  if(drumstates[drum].update){
    drumstates[drum].state = state;
    drumstates[drum].update = 2;
    return;
  }
  switch(subdrum){ //TODO XXX
    case 0: OPL3_TriggerBD(chip, state); if(state){StartModulatorsNow(chipoffset + 15, time);} break;
    case 1: OPL3_TriggerSD(chip, state); if(state){StartModulatorsNow(chipoffset + 16, time);} break;
    case 2: OPL3_TriggerHH(chip, state); modl_varis[16+chipoffset] = 0; break;
    case 3: OPL3_TriggerHH(chip, state); modl_varis[16+chipoffset] = 127; break;
    case 4: OPL3_TriggerTT(chip, state); if(state){StartModulatorsNow(chipoffset + 17, time);} break;
    case 5: OPL3_TriggerCY(chip, state); break;
  }
  drumstates[drum].state = state;
  drumstates[drum].update = 1;
  //DEBUG_MSG("Set drum %d chip %d to %d", drum, chip, state);
}


void MBFM_Drum_ReceiveMIDITrigger(u8 drum, u8 velocity){
  if(drum >= 6*OPL3_COUNT) return;
  voiceactualnotes[MBFM_Drum_GetChannel(drum)].velocity = velocity;
  MBFM_Drum_ReceiveGenericTrigger(drum, (velocity > 0), 1);
}

void MBFM_Drum_ReceivePanelTrigger(u8 drum, u8 state){
  if(drum >= 6*OPL3_COUNT) return;
  if(state){
    if(holdmode == MBFM_HOLD_MUTE){
      u8 val = !drumstates[drum].muted;
      drumstates[drum].muted = val;
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED, drum, val);
      return;
    }else if(holdmode == MBFM_HOLD_SOLO){
      u8 val = !drumstates[drum].soloed;
      drumstates[drum].soloed = val;
      if(val){
        drums_soloed++;
      }else{
        drums_soloed--;
      }
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED, drum, val);
      return;
    }else{
      u8 old_seldrum = seq_seldrum;
      seq_seldrum = drum;
      if(old_seldrum != seq_seldrum){
        if(mainmode == MBFM_MAINMODE_MIDI && act_midich == 9){
          MBFM_DrawScreen();
          MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_DISPVALUE, 0, mididrummapping[drum]);
        }else if(mainmode == MBFM_MAINMODE_SEQ){
          MBFM_SEQ_RedrawTrack();
        }
      }
    }
  }
  voiceactualnotes[MBFM_Drum_GetChannel(drum)].velocity = 127; //TODO
  MBFM_Drum_ReceiveGenericTrigger(drum, state, 1);
}

const char *MBFM_Drum_GetName(u8 drum){
  switch(drum){
  case 0:  return "BD A";
  case 1:  return "SD A";
  case 2:  return "HC A";
  case 3:  return "HO A";
  case 4:  return "TT A";
  case 5:  return "CY A";
  case 6:  return "BD B";
  case 7:  return "SD B";
  case 8:  return "HC B";
  case 9:  return "HO B";
  case 10: return "TT B";
  case 11: return "CY B";
  default: return "???";
  }
}

const char *MBFM_CondensedDrum_GetName(u8 drum){
  switch(drum){
  case 0:  return "BD A";
  case 1:  return "PrcA";
  case 2:  return "BD B";
  case 3:  return "PrcB";
  default: return "???";
  }
}

u8 MBFM_Drum_GetChannel(u8 drum){
  u8 ret = 18*(drum/6) + 16;
  switch(drum%6){
  case 0:
    ret -= 1;
    break;
  case 4:
  case 5:
    ret += 1;
    break;
  }
  return ret;
}

void MBFM_Drum_SetVelocity(u8 drum, u8 velocity){
  u8 sub = drum % 6;
  if(sub == 2 || sub == 3 || sub == 5) return;
  voiceactualnotes[MBFM_Drum_GetChannel(drum)].velocity = velocity;
}
void MBFM_Drum_SetVariation(u8 drum, u8 variation){
  u8 sub = drum % 6;
  if(sub != 0 && sub != 4) return;
  modl_varis[MBFM_Drum_GetChannel(drum)] = variation;
}

void MBFM_Drum_OffAll(){
  u8 i;
  //Turn off all notes
  for(i=0; i<6*OPL3_COUNT; i++){
    if(drumstates[i].state){
      MBFM_Drum_ReceiveGenericTrigger(i, 0, 0);
    }
  }
}

u8 MBFM_SEQ_GetSubmeas(u8 measure){
  measure++;
  s8 submeas;
  for(submeas=0; submeas<=3; submeas++){
    if(measure & 1){
      for(; submeas>=0; submeas--){
        if(submeas_enabled[submeas]) return submeas;
      }
      return 0;
    }
    measure >>= 1;
  }
  return 0;
}

u8 MBFM_SEQ_GetSequenceIndexFromSubmeas(u8 submeas, u8 substep){
  return OPL3_COUNT * (substep + ((seq_measurelen+1) * submeas));
}

u8 MBFM_SEQ_GetSequenceIndex(u8 measure, s8 substep){
  if(substep >= seq_measurelen + 1) return 0;
  u8 submeas = MBFM_SEQ_GetSubmeas(measure);
  if(substep >= 0){
    return MBFM_SEQ_GetSequenceIndexFromSubmeas(submeas, substep);
  }
  s8 prev_measure = measure;
  prev_measure--;
  if(prev_measure < 0) prev_measure += 8;
  substep = seq_measurelen;
  return MBFM_SEQ_GetSequenceIndex(prev_measure, substep);
}

u16 MBFM_SEQ_GetTempoBPM(){
  return 60000 * 256 / (TICKS_PER_STEP * seq_tempo * seq_beatlen);
}
void MBFM_SEQ_SetTempoBPM(u16 bpm){
  if(bpm > 300) return;
  if(bpm < 10) return;
  u32 old_tempo = seq_tempo;
  seq_tempo = (60000 * 256) / (bpm * seq_beatlen * TICKS_PER_STEP);
  if(seq_playing){
    //Adjust play start time
    u32 time = MIOS32_TIMESTAMP_Get();
    u32 newstarttime = time - ((seq_tempo * (time - seq_starttime)) / old_tempo);
    seq_starttime = newstarttime;
  }
}

void MBFM_SEQ_Restart(){
  seq_starttime = MIOS32_TIMESTAMP_Get();
  seq_masterpos = 0;
  seq_tickinstep = 0;
  seq_mastermeas = 0;
  seq_measstep = 0;
  seq_submeas = MBFM_SEQ_GetSubmeas(seq_mastermeas);
}


void MBFM_SEQ_RedrawAll(){
  if(mainmode != MBFM_MAINMODE_SEQ) return;
  u8 i;
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISSEQVEL, 0, (seq_show == MBFM_SEQSHOW_VEL));
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISSEQVARI, 0, (seq_show == MBFM_SEQSHOW_VARI));
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISSEQGATE, 0, (seq_show == MBFM_SEQSHOW_GATE));
  for(i=0; i<4; i++){
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISMEASURE, i, (i == seq_submeas));
  }
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISRECORDING, 0, seq_recording);
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISPLAYING, 0, seq_playing);
  MBFM_SEQ_RedrawTrack();
  MBNG_LCD_CursorSet(0,0,0);
  MBNG_LCD_PrintFormattedString(" Tmpo Swng Meas Beat Enab");
  MBNG_LCD_CursorSet(0,0,1);
  MBNG_LCD_PrintFormattedString(" %3d  %3d  %3d  %3d   |  ", 
      MBFM_SEQ_GetTempoBPM(), seq_swing, seq_measurelen, seq_beatlen);
  MBNG_LCD_CursorSet(0,5*screenmodeidx,0);
  MBNG_LCD_PrintFormattedString("~");
}

void MBFM_SEQ_RedrawTrack(){
  if(mainmode != MBFM_MAINMODE_SEQ) return;
  s8 substep;
  u8 stateG, stateR = 0;
  mbfm_drumstep_t* drumstep;
  u8 index;
  for(substep=-1; substep<seq_measurelen+1; substep++){
    index = MBFM_SEQ_GetSequenceIndexFromSubmeas(seq_selsubmeas, substep);
    drumstep = &sequence[index].drums[seq_seldrum];
    stateG = ((seq_selsubmeas == seq_submeas) && 
                  ((substep == seq_measstep) || (substep==-1 && seq_measstep==0)))
             || (seq_submeas == seq_selsubmeas+1 && seq_measstep==0 && substep==seq_measurelen);
    switch(seq_show){
    case MBFM_SEQSHOW_ON:
      stateR = drumstep->on;
      break;
    case MBFM_SEQSHOW_VEL:
      stateR = drumstep->vel;
      break;
    case MBFM_SEQSHOW_VARI:
      stateR = drumstep->vari;
      break;
    case MBFM_SEQSHOW_GATE:
      stateR = drumstep->gate & 1;
      stateG = (drumstep->gate & 2) > 0;
      break;
    }
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, substep+1, stateR);
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN, substep+1, stateG);
  }
}


void MBFM_SEQ_Init(){
  //TODO load from SD card instead of re-init
  mididrummapping[0] = 36; //C3
  mididrummapping[1] = 38; //D3
  mididrummapping[2] = 42; //G#3
  mididrummapping[3] = 46; //A#3
  mididrummapping[4] = 43; //G3
  mididrummapping[5] = 49; //C#4
  mididrummapping[6] = 35; //B2
  mididrummapping[7] = 40; //E3
  mididrummapping[8] = 51; //D#4
  mididrummapping[9] = 59; //A#4
  mididrummapping[10] = 47; //B3
  mididrummapping[11] = 57; //A4
  
  seq_measurelen = 16;
  seq_beatlen = 4;
  MBFM_SEQ_SetTempoBPM(120);
  seq_swing = 50;
  seq_recording = 0;
  seq_playing = 0;
  seq_lastsubmeas = 3;
  
  u8 i, drum;
  mbfm_drumstep_t* drumstep;
  for(i=0; i<OPL3_COUNT*4*17; i++){
    for(drum=0; drum<6; drum++){
      drumstep = &sequence[i].drums[drum];
      drumstep->vel = 1;
      drumstep->gate = 2;
    }
  }
  for(i=0; i<4; i++){
    submeas_enabled[i] = 1;
  }
}

void MBFM_SEQ_Tick(u32 time){
  if(!seq_playing) return;
  u8 old, doredraw = 0;
  //Update timing
  u32 tickssinceplay = ((time - seq_starttime) << 8) / seq_tempo;
  seq_masterpos = tickssinceplay >> TICKS_PER_STEP_SHIFT;
  seq_tickinstep = tickssinceplay - (seq_masterpos << TICKS_PER_STEP_SHIFT);
  if(seq_masterpos >= seq_measurelen << 3){
    seq_masterpos %= (seq_measurelen << 3);
    seq_starttime += (seq_tempo / (seq_measurelen << (TICKS_PER_STEP_SHIFT + 3))) >> 8;
  }
  seq_mastermeas = seq_masterpos / seq_measurelen;
  old = seq_submeas;
  seq_submeas = MBFM_SEQ_GetSubmeas(seq_mastermeas);
  if(old != seq_submeas) doredraw = 1;
  old = seq_measstep;
  seq_measstep = seq_masterpos - (seq_mastermeas * seq_measurelen);
  if(old != seq_measstep && seq_selsubmeas == seq_submeas) doredraw = 1;
  //Figure out index in sequence
  u8 seq_subpos = MBFM_SEQ_GetSequenceIndex(seq_mastermeas, seq_measstep);
  u8 seq_subpos_override = seq_subpos;
  if(seq_measstep == 0){
    seq_subpos_override = MBFM_SEQ_GetSequenceIndex(seq_mastermeas, -1);
  }
  //Update states of all drums
  mbfm_drumstep_t* drumstep;
  mbfm_drumstep_t* drumstep_override;
  u8 state, gate, chip, drum, drumidx;
  for(chip=0; chip<OPL3_COUNT; chip++){
    for(drum=0; drum<6; drum++){
      drumidx = drum+(6*chip);
      drumstep = &sequence[seq_subpos+chip].drums[drum];
      drumstep_override = &sequence[seq_subpos_override+chip].drums[drum];
      if(drumstep_override->on){
        //Use data from override step
        MBFM_Drum_SetVelocity(drumidx, drumstep_override->vel ? 127 : 64);
        MBFM_Drum_SetVariation(drumidx, drumstep_override->vari ? 127 : 0);
        state = 1;
        gate = drumstep_override->gate;
      }else{
        //Use data from original step
        MBFM_Drum_SetVelocity(drumidx, drumstep->vel ? 127 : 64);
        MBFM_Drum_SetVariation(drumidx, drumstep->vari ? 127 : 0);
        state = drumstep->on;
        gate = drumstep->gate;
      }
      //Determine actual state
      if(state && seq_tickinstep >= tps_gate_lengths[gate]) state = 0;
      if(!drumstates[drumidx].soloed && (drumstates[drumidx].muted || drums_soloed >= 1)) state = 0;
      if(drumstates[drumidx].state != state){
        MBFM_Drum_ReceiveGenericTrigger(drumidx, state, 0);
      }
    }
  }
  if(doredraw){
    MBFM_SEQ_RedrawTrack();
  }
}
void MBFM_SEQ_PostTick(u32 time){
  u8 i, d, subdrum, state, chip;
  for(i=0; i<6*OPL3_COUNT; i++){
    d = drumstates[i].update;
    if(d){
      drumstates[i].update = 0;
      if(d == 2){
        state = drumstates[i].state;
        chip = i/6;
        subdrum = i%6;
        switch(subdrum){
        case 0: OPL3_TriggerBD(chip, state); if(state){StartModulatorsNow((chip*18) + 15, time);} break;
        case 1: OPL3_TriggerSD(chip, state); if(state){StartModulatorsNow((chip*18) + 16, time);} break;
        case 2:
        case 3: OPL3_TriggerHH(chip, state); break;
        case 4: OPL3_TriggerTT(chip, state); if(state){StartModulatorsNow((chip*18) + 17, time);} break;
        case 5: OPL3_TriggerCY(chip, state); break;
        }
      }
    }
  }
}

void MBFM_Drum_RecordNow(u8 drum, u8 state){
  if(drum >= 6*OPL3_COUNT) return;
  if(state){
    u8 index;
    u8 submeas;
    mbfm_drumstep_t* drumstep;
    u8 measstep = seq_measstep;
    //Quantize
    if(seq_tickinstep >= (TICKS_PER_STEP / 2) && measstep < 15){
      measstep++;
    }
    for(submeas = seq_submeas; submeas<=3; submeas++){
      index = MBFM_SEQ_GetSequenceIndexFromSubmeas(submeas, seq_measstep);
      drumstep = &sequence[index].drums[drum];
      drumstep->on = 1;
    }
  }else{
    //TODO
  }
}

///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnSeqRec(u8 value){
  if(!value) return;
  seq_recording = !seq_recording;
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISRECORDING, 0, seq_recording);
  if(seq_recording && !seq_playing){
    MBFM_BtnSeqPlay(1); //Start playing
  }
  MBFM_SEQ_RedrawTrack();
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnSeqPlay(u8 value){
  if(!value) return;
  seq_playing = !seq_playing;
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISPLAYING, 0, seq_playing);
  if(!seq_playing){
    MBFM_Drum_OffAll();
    if(seq_recording){
      MBFM_BtnSeqRec(1); //Stop recording
    }
  }else{
    MBFM_SEQ_Restart();
  }
  MBFM_SEQ_RedrawTrack();
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnSeqBack(u8 value){
  if(!value) return;
  MBFM_Drum_OffAll();
  MBFM_SEQ_Restart();
  MBFM_SEQ_RedrawTrack();
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnSeqMute(u8 value){
  u8 i;
  if(value){
    if(holdmode != MBFM_HOLD_NONE) return;
    holdmode = MBFM_HOLD_MUTE;
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISMUTE, 0, 1);
    for(i=0; i<6*OPL3_COUNT; i++){
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED, i, drumstates[i].muted);
    }
  }else{
    if(holdmode != MBFM_HOLD_MUTE) return;
    holdmode = MBFM_HOLD_NONE;
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISMUTE, 0, 0);
    for(i=0; i<6*OPL3_COUNT; i++){
      if(drumactivity[i] >= 1){
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED, i, 1);
        drumactivity[i] = 2;
      }else{
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED, i, 0);
        drumactivity[i] = -1;
      }
    }
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnSeqSolo(u8 value){
  u8 i;
  if(value){
    if(holdmode != MBFM_HOLD_NONE) return;
    holdmode = MBFM_HOLD_SOLO;
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISSOLO, 0, 1);
    for(i=0; i<6*OPL3_COUNT; i++){
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED, i, drumstates[i].soloed);
    }
  }else{
    if(holdmode != MBFM_HOLD_SOLO) return;
    holdmode = MBFM_HOLD_NONE;
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISSOLO, 0, 0);
    for(i=0; i<6*OPL3_COUNT; i++){
      if(drumactivity[i] >= 1){
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED, i, 1);
        drumactivity[i] = 2;
      }else{
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED, i, 0);
        drumactivity[i] = -1;
      }
    }
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnSeqVel(u8 value){
  if(!value) return;
  if(seq_show == MBFM_SEQSHOW_VEL){
    seq_show = MBFM_SEQSHOW_ON;
  }else{
    seq_show = MBFM_SEQSHOW_VEL;
  }
  MBFM_SEQ_RedrawAll();
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnSeqVari(u8 value){
  if(!value) return;
  if(seq_show == MBFM_SEQSHOW_VARI){
    seq_show = MBFM_SEQSHOW_ON;
  }else{
    seq_show = MBFM_SEQSHOW_VARI;
  }
  MBFM_SEQ_RedrawAll();
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnSeqGate(u8 value){
  if(!value) return;
  if(seq_show == MBFM_SEQSHOW_GATE){
    seq_show = MBFM_SEQSHOW_ON;
  }else{
    seq_show = MBFM_SEQSHOW_GATE;
  }
  MBFM_SEQ_RedrawAll();
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnSeqTrigger(u8 instrument, u8 value){
  //DEBUG_MSG("MBFM_BtnSeqTrigger %d, value %d", instrument, value);
  if(instrument >= 6*OPL3_COUNT) return;
  MBFM_Drum_ReceivePanelTrigger(instrument, value);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnSeqMeasure(u8 measure, u8 value){
  if(!value) return;
  if(measure == seq_selsubmeas) return;
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISMEASURE, seq_selsubmeas, 0);
  seq_selsubmeas = measure;
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISMEASURE, seq_selsubmeas, 1);
  MBFM_SEQ_RedrawTrack();
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnSeqSection(u8 value){
  u8 i;
  if(holdmode == MBFM_HOLD_MUTE){
    for(i=0; i<6*OPL3_COUNT; i++){
      drumstates[i].muted = 0;
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED, i, 0);
    }
    return;
  }else if(holdmode == MBFM_HOLD_SOLO){
    for(i=0; i<6*OPL3_COUNT; i++){
      drumstates[i].soloed = 0;
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED, i, 0);
    }
    drums_soloed = 0;
    return;
  }
}
///////////////////////////////////////////////////////////////////////////////


void MBFM_SEQ_TrackButton(u8 button, u8 value){
  if(!button) return;
  if(!value) return;
  u8 index, submeas, val = 0, flag = 0;
  mbfm_drumstep_t* drumstep;
  u8 measstep = button - 1;
  for(submeas = seq_selsubmeas; submeas<=3; submeas++){
    index = MBFM_SEQ_GetSequenceIndexFromSubmeas(submeas, measstep);
    drumstep = &sequence[index].drums[seq_seldrum];
    switch(seq_show){
    case MBFM_SEQSHOW_ON:
      if(!flag) val = !drumstep->on;
      drumstep->on = val;
      break;
    case MBFM_SEQSHOW_VEL:
      if(!flag) val = !drumstep->vel;
      drumstep->vel = val;
      break;
    case MBFM_SEQSHOW_VARI:
      if(!flag) val = !drumstep->vari;
      drumstep->vari = val;
      break;
    case MBFM_SEQSHOW_GATE:
      if(!flag){
        val = drumstep->gate;
        val++;
        if(val > 3) val = 0;
      }
      drumstep->gate = val;
      break;
    }
    flag = 1;
  }
  MBFM_SEQ_RedrawTrack();
}
void MBFM_SEQ_Softkey(u8 button, u8 value){
  button -= 1;
  if(button == 4){
    //TODO submeasure enable
  }else{
    if(!value) return;
    if(screenmodeidx != button){
      MBNG_LCD_CursorSet(0,5*screenmodeidx,0);
      MBNG_LCD_PrintFormattedString(" ");
      screenmodeidx = button;
      MBNG_LCD_CursorSet(0,5*screenmodeidx,0);
      MBNG_LCD_PrintFormattedString("~");
    }
  }
}
void MBFM_SEQ_Datawheel(s16 realinc){
  s32 val;
  switch(screenmodeidx){
  case 0:
    //Tempo
    val = MBFM_SEQ_GetTempoBPM();
    val += realinc;
    MBNG_LCD_CursorSet(0,1,1);
    MBNG_LCD_PrintFormattedString("%3d", val);
    MBFM_SEQ_SetTempoBPM(val);
    break;
  case 1:
    //Swing
    
    break;
  case 2:
    //Measure length
    
    break;
  case 3:
    //Beat length
    
    break;
  }
}
